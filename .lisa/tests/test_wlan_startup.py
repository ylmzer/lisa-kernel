"""Exercise the actual built-in WLAN init and deferred load error paths."""
from pathlib import Path
import re, subprocess, tempfile
root = Path(__file__).resolve().parents[2]
source = (root / 'drivers/staging/qcacld-3.0/core/hdd/src/wlan_hdd_main.c').read_text()

def function(signature):
    begin = source.index(signature)
    pos = source.index('{', begin)
    end, depth = pos + 1, 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[begin:end]

prelude = r'''
#include <assert.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#define __init
#define hdd_err(...) ((void)0)
static bool hdd_loaded;
static int wlan_driver_load_lock, locked;
static int sysfs_result, state_result, load_result;
static int sysfs_calls, state_calls, cleanup_calls, load_calls;
static void mutex_lock(int *p) {(void)p; assert(!locked); locked=1;}
static void mutex_unlock(int *p) {(void)p; assert(locked); locked=0;}
static int wlan_init_sysfs(void) {sysfs_calls++; return sysfs_result;}
static int wlan_hdd_state_ctrl_param_create(void) {state_calls++; return state_result;}
static int wlan_deinit_sysfs(void) {cleanup_calls++; return 0;}
static int hdd_driver_load(void) {
    assert(locked); load_calls++;
    if (!load_result) hdd_loaded=true;
    return load_result;
}
'''
main = r'''
int main(void) {
    sysfs_result=-ENOMEM;
    assert(hdd_module_init()==-ENOMEM);
    assert(sysfs_calls==1 && state_calls==0 && cleanup_calls==0);
    sysfs_result=0; state_result=-ENODEV;
    assert(hdd_module_init()==-ENODEV);
    assert(state_calls==1 && cleanup_calls==1);
    state_result=0;
    assert(hdd_module_init()==0 && state_calls==2);
    assert(load_calls==0); /* Firmware I/O stays deferred to the HAL. */
    load_result=-EIO;
    assert(wlan_hdd_request_driver_load()==-EIO && !hdd_loaded && !locked);
    load_result=0;
    assert(wlan_hdd_request_driver_load()==0 && hdd_loaded && !locked);
    assert(wlan_hdd_request_driver_load()==0 && load_calls==2);
    puts("Built-in WLAN HAL entry, deferred load retry and init cleanup passed");
}
'''
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory)
    test = path / 'wlan-startup.c'
    test.write_text(prelude + function('static int wlan_hdd_request_driver_load(void)') +
                    function('static int __init hdd_module_init(void)') + main)
    subprocess.run(['gcc', '-std=gnu11', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(test), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
