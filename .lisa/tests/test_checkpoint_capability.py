"""Exercise the actual bounding-set code and namespace capability helper."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
uapi = (root / 'include/uapi/linux/capability.h').read_text()
header = (root / 'include/linux/capability.h').read_text()
common = (root / 'security/commoncap.c').read_text()
defines = '\n'.join(re.findall(
    r'^#define (?:CAP_SETPCAP|CAP_SYS_ADMIN|CAP_CHECKPOINT_RESTORE|CAP_LAST_CAP|cap_valid).*$',
    uapi, re.M))
helper = re.search(r'static inline bool checkpoint_restore_ns_capable\([^}]+\}', header).group()
drop = re.search(r'static int cap_prctl_drop\([^}]+\}', common).group()
read = common.split('case PR_CAPBSET_READ:', 1)[1].split('case PR_CAPBSET_DROP:', 1)[0]

# Both real feature gates must accompany the UAPI/SELinux update.
assert 'checkpoint_restore_ns_capable(pid_ns->user_ns)' in (root / 'kernel/pid_namespace.c').read_text()
assert 'checkpoint_restore_ns_capable(&init_user_ns)' in (root / 'fs/proc/base.c').read_text()
classmap = (root / 'security/selinux/include/classmap.h').read_text()
perms = classmap.split('#define COMMON_CAP2_PERMS', 1)[1].split('#if', 1)[0]
assert re.findall(r'"([a-z_]+)"', perms)[8] == 'checkpoint_restore'
assert '#if CAP_LAST_CAP > CAP_CHECKPOINT_RESTORE' in classmap

harness = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
struct user_namespace { uint64_t grants; };
struct cred { uint64_t cap_bset; };
static struct user_namespace init_ns, child_ns;
static struct user_namespace *caller_ns;
static struct cred live, prepared;
static bool fail_alloc;
static unsigned allocations, commits;
static bool ns_capable(struct user_namespace *ns, int cap) {
    return !!(ns->grants & (UINT64_C(1) << cap));
}
static struct user_namespace *current_user_ns(void) { return caller_ns; }
static struct cred *prepare_creds(void) {
    allocations++; prepared = live; return fail_alloc ? NULL : &prepared;
}
static int commit_creds(struct cred *c) { commits++; live = *c; return 0; }
#define cap_lower(set, cap) ((set) &= ~(UINT64_C(1) << (cap)))
#define cap_raised(set, cap) ((set) & (UINT64_C(1) << (cap)))
'''
tests = r'''
int main(void) {
    assert(CAP_CHECKPOINT_RESTORE == 40 && CAP_LAST_CAP == 40);
    assert(!cap_valid(-1) && cap_valid(39) && cap_valid(40) && !cap_valid(41));
    caller_ns = &child_ns;
    assert(!checkpoint_restore_ns_capable(&child_ns));
    child_ns.grants = UINT64_C(1) << CAP_CHECKPOINT_RESTORE;
    assert(checkpoint_restore_ns_capable(&child_ns));
    assert(!checkpoint_restore_ns_capable(&init_ns));
    child_ns.grants = UINT64_C(1) << CAP_SYS_ADMIN;
    assert(checkpoint_restore_ns_capable(&child_ns));
    assert(!checkpoint_restore_ns_capable(&init_ns));
    init_ns.grants = UINT64_C(1) << CAP_CHECKPOINT_RESTORE;
    assert(checkpoint_restore_ns_capable(&init_ns));
    live.cap_bset = (UINT64_C(1) << 41) - 1;
    assert(bound_read(40) == 1 && bound_read(41) == -EINVAL);
    assert(bound_read((unsigned long)-1) == -EINVAL);
    child_ns.grants = 0;
    assert(cap_prctl_drop(40) == -EPERM && allocations == 0 && commits == 0);
    child_ns.grants = UINT64_C(1) << CAP_SETPCAP;
    assert(cap_prctl_drop(41) == -EINVAL && allocations == 0);
    fail_alloc = true;
    assert(cap_prctl_drop(40) == -ENOMEM && bound_read(40) == 1 && commits == 0);
    fail_alloc = false;
    assert(cap_prctl_drop(40) == 0 && bound_read(40) == 0);
    assert(bound_read(39) == 1 && commits == 1);
    assert(cap_prctl_drop(40) == 0 && bound_read(40) == 0);
    puts("Actual capability query/drop, permission denial, allocation failure and namespace boundaries passed");
}
'''
with tempfile.TemporaryDirectory() as tmp:
    source = Path(tmp) / 'capability.c'
    binary = Path(tmp) / 'capability'
    source.write_text(harness + '\n' + defines + '\n' + helper + '\n' + drop +
                      '\nstatic int bound_read(unsigned long arg2) { const struct cred *old = &live;\n' +
                      read + '\n}\n' + tests)
    subprocess.run(['gcc', '-std=gnu11', '-O2', '-g', '-Wall', '-Werror',
                    '-fsanitize=address,undefined', str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
