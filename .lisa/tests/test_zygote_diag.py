"""Exercise the actual diagnostic filter and quota with a host C harness."""
from pathlib import Path
import subprocess, tempfile
root = Path(__file__).resolve().parents[2]
src = (root / 'arch/arm64/kernel/syscall.c').read_text()
start = src.index('static atomic_t lisa_zygote_diag_records')
end = src.index('\n#else', start)
helper = src[start:end]
harness = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#define TASK_COMM_LEN 16
#define ATOMIC_INIT(x) (x)
typedef int atomic_t;
static bool compat;
static char task_comm[TASK_COMM_LEN];
static unsigned int emitted;
static unsigned int last_nr;
static long last_ret;
#define current ((void *)1)
#define is_compat_task() (compat)
#define get_task_comm(dst, task) memcpy(dst, task_comm, TASK_COMM_LEN)
#define task_pid_nr(task) 42
static int atomic_add_unless(atomic_t *a, int add, int unless) {
    if (*a == unless) return 0;
    *a += add; return 1;
}
#define pr_info(fmt, pid, nr, ret) do { emitted++; last_nr = nr; last_ret = ret; } while (0)
#define __NR_prctl 167
#define __NR_capset 91
#define __NR_setresuid 147
#define __NR_setresgid 149
#define __NR_setgroups 159
#define __NR_seccomp 277
#define __NR_sched_setattr 274
#define __NR_sched_setscheduler 119
#define __NR_sched_setparam 118
#define __NR_setrlimit 164
#define __NR_prlimit64 261
'''
tests = r'''
int main(void) {
    strcpy(task_comm, "zygote-child-x");
    lisa_zygote_syscall_diag(__NR_capset, -1);
    assert(emitted == 0);
    strcpy(task_comm, "zygote-child");
    compat = true;
    lisa_zygote_syscall_diag(__NR_capset, -1);
    assert(emitted == 0);
    compat = false;
    lisa_zygote_syscall_diag(64, -1); /* write is outside selected init syscalls */
    lisa_zygote_syscall_diag(UINT32_MAX, -38); /* NO_SYSCALL */
    assert(emitted == 0);
    lisa_zygote_syscall_diag(__NR_capset, -1);
    assert(emitted == 1 && last_nr == __NR_capset && last_ret == -1);
    lisa_zygote_syscall_diag(__NR_prctl, 0);
    assert(emitted == 2 && last_ret == 0);
    lisa_zygote_syscall_diag(__NR_prlimit64, -22);
    assert(emitted == 3 && last_ret == -22);
    for (int i = 0; i < 1000; i++) lisa_zygote_syscall_diag(__NR_setresuid, 0);
    assert(emitted == 256 && lisa_zygote_diag_records == 256);
    lisa_zygote_syscall_diag(__NR_seccomp, -13);
    assert(emitted == 256 && last_ret == 0);
    puts("PASS: exact task name, compat exclusion, syscall filter, errno/success and hard quota");
}
'''
with tempfile.TemporaryDirectory() as td:
    path = Path(td)
    (path / 'test.c').write_text(harness + helper + tests)
    subprocess.run(['gcc', '-std=gnu11', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(path / 'test.c'),
                    '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
