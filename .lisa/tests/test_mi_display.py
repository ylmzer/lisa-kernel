"""Check pinned MIUI UAPI and the actual asynchronous payload ownership code."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'techpack/display/msm/mi_disp/mi_disp_file.c').read_text()

def function(name):
    match = re.search(r'static (?:void|int) ' + name + r'\s*\(', source)
    assert match
    start = source.index('{', match.start())
    depth = 1
    end = start + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]

prelude = r'''
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define __user
#include "drm/mi_disp.h"
_Static_assert(sizeof(struct disp_feature_req) == 48, "feature ABI");
_Static_assert(offsetof(struct disp_feature_req, tx_ptr) == 24, "tx ABI");
_Static_assert(offsetof(struct disp_feature_req, rx_ptr) == 40, "rx ABI");
_Static_assert(MI_DISP_IOCTL_SET_FEATURE == 0xc0304401, "ioctl ABI");
_Static_assert(DISP_FEATURE_DIMMING == 0 && DISP_FEATURE_GIR == 20, "feature IDs");
typedef uint32_t u32;
typedef unsigned char u8;
#define GFP_ATOMIC 0
#define GFP_KERNEL 0
#define container_of(p,t,m) ((t *)((char *)(p)-offsetof(t,m)))
struct kthread_work { void (*fn)(struct kthread_work *); };
struct disp_display { void *display; int pending_wq; struct {int worker;} d_thread; };
struct disp_work { struct kthread_work work; struct disp_display *dd_ptr; void *wq; void *data; };
struct disp_feature_ctl {u32 feature_id; int feature_val; u32 tx_len; u8 *tx_ptr; u32 rx_len; u8 *rx_ptr;};
static int allocations, calls, fail_at, delivered;
static struct kthread_work *queued;
static void *kzalloc(size_t n, int flags) {
    (void)flags;
    if (++calls == fail_at) return NULL;
    void *p = calloc(1,n); if(p) allocations++; return p;
}
static void *kmemdup(const void *src, size_t n, int flags) {
    void *p=kzalloc(n,flags); if(p) memcpy(p,src,n); return p;
}
static void kfree(void *p) {if(p) {allocations--; free(p);}}
static void kthread_init_work(struct kthread_work *w, void (*fn)(struct kthread_work *)) {w->fn=fn;}
static void kthread_queue_work(int *worker, struct kthread_work *w) {(void)worker; queued=w;}
static void mi_dsi_display_set_disp_param(void *display, struct disp_feature_ctl *ctl) {
    (void)display; assert(ctl->tx_len==4); assert(!memcmp(ctl->tx_ptr,"MIUI",4));
    assert(ctl->rx_len==4 && ctl->rx_ptr[0]==0); delivered++;
}
'''
main = r'''
int main(void) {
    struct disp_display dd={0};
    for(int failure=0; failure<=3; failure++) {
        calls=0; fail_at=failure; queued=NULL;
        struct disp_feature_ctl ctl={.tx_len=4,.rx_len=4};
        ctl.tx_ptr=malloc(4); memcpy(ctl.tx_ptr,"MIUI",4); ctl.rx_ptr=malloc(4);
        int ret=mi_disp_set_feature_queue_work(&dd,&ctl,sizeof(ctl));
        free(ctl.tx_ptr); free(ctl.rx_ptr);
        if(failure) {assert(ret==-ENOMEM); assert(!queued);}
        else {assert(!ret && queued); queued->fn(queued); assert(delivered==1);}
        assert(allocations==0);
    }
    puts("MIUI ABI, delayed payload lifetime and allocation-failure cleanup passed");
}
'''
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory)
    test = path / 'mi-display.c'
    test.write_text(prelude + function('mi_disp_set_feature_work_handler') + '\n' +
                    function('mi_disp_set_feature_queue_work') + main)
    subprocess.run(['gcc', '-std=gnu11', '-O1', '-g', '-fsanitize=address,undefined',
                    '-I' + str(root / 'techpack/display/include/uapi/display'),
                    str(test), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
