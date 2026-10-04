"""Compile the actual uname helper with mocked credentials/task identity."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'kernel/sys.c').read_text()
start = source.index('static void lisa_netbpfload_compat(')
helper = source[start:source.index('\n}\n', start) + 3]
harness = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define TASK_COMM_LEN 16
#define GLOBAL_ROOT_UID 0
#define UTS_RELEASE "5.4.302-lisa-bootdiag+"
struct new_utsname { char sysname[65], nodename[65], release[65], version[65], machine[65], domainname[65]; };
static unsigned test_uid;
static char test_comm[TASK_COMM_LEN];
static unsigned messages;
#define current 0
#define current_uid() test_uid
#define uid_eq(a,b) ((a)==(b))
#define get_task_comm(out,task) memcpy(out,test_comm,TASK_COMM_LEN)
#define strscpy(out,in,len) snprintf(out,len,"%s",in)
#define pr_info_once(...) (++messages)
'''
harness += helper
harness += r'''
static void check(unsigned uid, const char *comm, int match) {
    struct new_utsname actual, expected;
    memset(&actual, 0x5a, sizeof(actual));
    strcpy(actual.release, UTS_RELEASE);
    expected = actual;
    test_uid = uid;
    memset(test_comm, 0, sizeof(test_comm));
    snprintf(test_comm, sizeof(test_comm), "%s", comm);
    messages = 0;
    if (match) snprintf(expected.release, sizeof(expected.release), "%s", "5.10.248");
    lisa_netbpfload_compat(&actual);
    assert(memcmp(&actual, &expected, sizeof(actual)) == 0);
    assert(messages == (unsigned)match);
}
int main(void) {
    check(0, "netbpfload", 1);
    check(1000, "netbpfload", 0);
    check(10000, "netbpfload", 0);
    check(0, "netbpfload-test", 0);
    check(0, "netbpfloa", 0);
    check(0, "bpfloader", 0);
    check(0, "netd", 0);
    check(0, "uname", 0);
    check(0, "", 0);
    puts("uname compatibility: root/exact-name guards and all other UTS fields passed");
}
'''
with tempfile.TemporaryDirectory() as folder:
    path = Path(folder)
    (path / 'test.c').write_text(harness)
    binary = path / ('test.exe' if os.name == 'nt' else 'test')
    subprocess.run([os.environ.get('CC', 'gcc'), '-std=gnu11', '-Wall', '-Wextra', '-Werror',
                    str(path / 'test.c'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
