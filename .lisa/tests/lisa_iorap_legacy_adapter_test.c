/* SPDX-License-Identifier: GPL-2.0-only */
/* Execute the production adapter with explicit allocation/VFS/capture doubles.
 * Does not emulate a donor driver or replace device runtime acceptance.
 */
#define _GNU_SOURCE
#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <linux/types.h>
#define CONFIG_LISA_IORAP_LEGACY 1
#define CONFIG_LISA_IORAP_TEST 1
#define LISA_IORAP_LEGACY_HOST_TEST 1
#include "../../mm/lisa_iorap_snapshot_budget.h"
#define LISA_IORAP_MAX_FILES LISA_IORAP_SNAPSHOT_MAX_FILES
#define __user
#define __init
#define THIS_MODULE NULL
#define GLOBAL_ROOT_UID 0
#define CAP_SYS_ADMIN 21
#define GFP_KERNEL 0
#define MISC_DYNAMIC_MINOR 255
#define BUILD_BUG_ON(c) _Static_assert(!(c), "layout")
#define device_initcall(f)
#define DEFINE_MUTEX(n) int n
#define WARN_ON(x) (!!(x))
#define ERR_PTR(e) ((void *)(intptr_t)(e))
#define PTR_ERR(p) ((int)(intptr_t)(p))
#define IS_ERR(p) ((uintptr_t)(p)>=(uintptr_t)-4095)
#define cpu_to_le64(x) htole64(x)
#define time_before(a,b) ((long)((a)-(b))<0)
#define READ_ONCE(x) (x)
#define WRITE_ONCE(x,v) ((x)=(v))
typedef _Atomic int atomic_t;
static atomic_t capture_transport_owner;
static unsigned long jiffies=1;
static int init_user_ns, other_namespace, euid, capability=1, namespace_other;
static int fail_alloc, fail_end, fail_copy, active, end_timeout_probe;
static unsigned int mock_files=1;
static unsigned long mock_bits=130;
static int atomic_cmpxchg(atomic_t *p,int old,int value)
{ atomic_compare_exchange_strong(p,&old,value);return old; }
static int atomic_read(atomic_t *p) { return atomic_load(p); }
static void mutex_lock(int *m) { assert(!*m);*m=1; }
static void mutex_unlock(int *m) { assert(*m);*m=0; }
static void *kvzalloc(size_t n,int flags)
{ (void)flags;if(fail_alloc)return NULL;return calloc(1,n); }
static void *kcalloc(size_t n,size_t bytes,int flags)
{ (void)flags;if(fail_alloc)return NULL;return calloc(n,bytes); }
static void kvfree(void *p) { free(p); }
static void kfree(void *p) { free(p); }
static void *current_user_ns(void) { return namespace_other?&other_namespace:&init_user_ns; }
static int current_euid(void) { return euid; }
static unsigned int __kuid_val(int uid) { return (unsigned int)uid; }
static int uid_eq(int a,int b) { return a==b; }
static int ns_capable(void *ns,int cap) { (void)ns;(void)cap;return capability; }
static int make_kuid(void *ns,unsigned int uid) { (void)ns;return (int)uid; }
static int capture_control;
static unsigned long capture_deadline;
static struct {unsigned int uid; int active;} capture;
struct task_struct {unsigned int pid,uid; unsigned long long start_time; struct task_struct *group_leader; int refs;};
static struct task_struct mock_leader={.pid=4455,.uid=10001,.start_time=12345},mock_render={.pid=4456,.uid=10001,.start_time=12346};
static int mock_tasks_alive=1;
static struct task_struct *find_get_task_by_vpid(unsigned int pid) {
 struct task_struct *t=NULL;
 if(mock_tasks_alive) {if(pid==4455)t=&mock_leader;else if(pid==4456)t=&mock_render;}
 if(t)t->refs++;
 return t;
}
static unsigned int task_pid_nr(struct task_struct *t){return t->pid;}
static unsigned int task_tgid_nr(struct task_struct *t){return t->group_leader->pid;}
static int task_uid(struct task_struct *t){return t->uid;}
static void put_task_struct(struct task_struct *t){assert(t->refs>0);t->refs--;}
#define rcu_read_lock() ((void)0)
#define rcu_read_unlock() ((void)0)
#define pr_info_ratelimited(...) ((void)0)
struct inode {int unused;};struct file {int unused;};
static int nonseekable_open(struct inode *i,struct file *f) {(void)i;(void)f;return 0;}
static unsigned long copy_from_user(void *to,const void *from,unsigned long bytes)
{ if(fail_copy||!from)return bytes;memcpy(to,from,bytes);return 0; }
static unsigned long copy_to_user(void *to,const void *from,unsigned long bytes)
{ if(fail_copy||!to)return bytes;memcpy(to,from,bytes);return 0; }
struct file_operations {void *owner;int (*open)(struct inode *,struct file *);
 long (*unlocked_ioctl)(struct file *,unsigned int,unsigned long);void *llseek;};
#define no_llseek NULL
struct miscdevice {int minor;const char *name;const struct file_operations *fops;int mode;};
static int misc_register(struct miscdevice *d) {assert(d->mode==0600);return 0;}
struct lisa_iorap_snapshot {int unused;};
struct lisa_iorap_file_bitmap {struct {long tv_sec,tv_nsec;} mtime;
 unsigned long long inode_number,*bitmap;unsigned long bits,words;char path[256];};
struct lisa_iorap_bitmap_snapshot {unsigned long files;
 struct lisa_iorap_file_bitmap records[LISA_IORAP_MAX_FILES];};
static int lisa_iorap_capture_begin(int uid)
{assert(uid>=0);if(active)return -EBUSY;active=1;capture.active=1;capture.uid=uid;capture_deadline=jiffies+10000;return 0;}
static int lisa_iorap_capture_abort(void) {active=0;return 0;}
static struct lisa_iorap_snapshot *lisa_iorap_capture_end(void)
{
 if(end_timeout_probe){
  /* The real timeout only releases a collecting owner (2). Finish must
   * already hold owner 3, preventing diagnostic open during serialization.
   */
  assert(atomic_cmpxchg(&capture_transport_owner,2,0)==3);
  assert(atomic_cmpxchg(&capture_transport_owner,0,1)==3);
 }
 if(fail_end)return ERR_PTR(-ENOMEM);
 if(!active)return ERR_PTR(-ENOENT);
 active=0;return calloc(1,sizeof(struct lisa_iorap_snapshot));
}
static void lisa_iorap_snapshot_put(struct lisa_iorap_snapshot *p){free(p);}
static struct lisa_iorap_bitmap_snapshot *lisa_iorap_snapshot_bitmap(struct lisa_iorap_snapshot *raw)
{
 unsigned int i;struct lisa_iorap_bitmap_snapshot *s;
 (void)raw;if(fail_alloc)return ERR_PTR(-ENOMEM);
 s=calloc(1,sizeof(*s));assert(s);s->files=mock_files;
 for(i=0;i<s->files;i++){
  struct lisa_iorap_file_bitmap *r=&s->records[i];
  r->mtime.tv_sec=1700000000;r->mtime.tv_nsec=123;r->inode_number=17+i;
  r->bits=mock_bits;r->words=(mock_bits+63)/64;
  r->bitmap=calloc(r->words,8);assert(r->bitmap);r->bitmap[0]=1;
  snprintf(r->path,sizeof(r->path),"/data/test-fixture-%u",i);
 }
 return s;
}
static void lisa_iorap_bitmap_put(struct lisa_iorap_bitmap_snapshot *s)
{unsigned long i;for(i=0;i<s->files;i++)free(s->records[i].bitmap);free(s);}
#define pr_warn_ratelimited(...) ((void)0)
#include "../../mm/lisa_iorap_legacy_device.h"
static void request(unsigned char *data,unsigned int uid,unsigned int pid)
{memset(data,0,280);lisa_iorap_wire_u32(data,uid);lisa_iorap_wire_u32(data+4,pid);memcpy(data+8,"com.test.fixture",17);}
static long call(unsigned int nr,void *data)
{jiffies++;return legacy_ioctl(NULL,0xc1084c00U+nr,(unsigned long)data);}
int main(void)
{
 unsigned char input[280],size_query[280],small[280];unsigned char *info;
 unsigned int i,enabled;unsigned long bytes;int slot;
 mock_leader.group_leader=&mock_leader;mock_render.group_leader=&mock_leader;
 assert(!lisa_iorap_legacy_init());assert(!legacy_open(NULL,NULL));
 /* Zygote UID at generation sample does not authorize attachment. */
 request(input,10001,4455);mock_leader.uid=0;euid=0;
 assert(!call(1,input));request(small,10001,4456);
 assert(call(1,small)==-EBUSY);mock_leader.uid=10001;
 assert(!call(1,small));assert(!call(6,input));
 request(input,10001,4455);euid=2000;assert(call(1,input)==-EPERM);
 euid=0;capability=0;assert(call(1,input)==-EPERM);
 euid=1000;assert(legacy_open(NULL,NULL)==-EAGAIN);
 enabled=1;assert(legacy_ioctl(NULL,LISA_IORAP_NATIVE_SWITCH,(unsigned long)&enabled)==-EPERM);
 euid=0;capability=1;enabled=2;
 assert(legacy_ioctl(NULL,LISA_IORAP_NATIVE_SWITCH,(unsigned long)&enabled)==-EINVAL);
 enabled=1;assert(!legacy_ioctl(NULL,LISA_IORAP_NATIVE_SWITCH,(unsigned long)&enabled));
 euid=1000;assert(!legacy_open(NULL,NULL));namespace_other=1;
 assert(call(1,input)==-EPERM);namespace_other=0;euid=0;capability=1;
 fail_copy=1;assert(call(1,input)==-EFAULT);fail_copy=0;
 atomic_store(&capture_transport_owner,1);assert(call(1,input)==-EBUSY);
 atomic_store(&capture_transport_owner,0);assert(!call(1,input));
 assert(active && atomic_read(&capture_transport_owner)==2);
 assert(call(1,input)==-EBUSY);
 /* Real adapter: render attaches without restarting collection or deadline. */
 request(small,10001,4456);{unsigned long deadline=capture_deadline;
 assert(!call(1,small));assert(active && capture_deadline==deadline && legacy_active.pid==4455);
 mock_render.uid=10002;assert(call(1,small)==-EBUSY);mock_render.uid=10001;
 mock_render.group_leader=&mock_render;assert(call(1,small)==-EBUSY);mock_render.group_leader=&mock_leader;
 mock_leader.start_time++;assert(call(1,small)==-EBUSY);mock_leader.start_time--;
 mock_tasks_alive=0;assert(call(1,small)==-EBUSY);mock_tasks_alive=1;
 request(small,10002,4456);assert(call(1,small)==-EBUSY);
 request(small,10001,4456);small[8]='x';assert(call(1,small)==-EBUSY);
 request(small,10001,4456);capture_deadline=jiffies;assert(call(1,small)==-EBUSY);capture_deadline=deadline;
 atomic_store(&capture_transport_owner,3);assert(call(1,small)==-EBUSY);atomic_store(&capture_transport_owner,2);
 assert(!mock_leader.refs && !mock_render.refs);
 }
 /* No release callback: the actual JNI closes its fd after every ioctl. */
 request(size_query,10002,0);assert(call(5,size_query)==-ENOENT);
 assert(active);end_timeout_probe=1;assert(!call(5,input));end_timeout_probe=0;
 assert(!active && atomic_read(&capture_transport_owner)==0);
 request(size_query,10001,0);assert(!call(7,size_query));
 bytes=(unsigned long)le64toh(*(__le64 *)(size_query+264));assert(bytes==280+296+24);
 request(small,10001,0);lisa_iorap_wire_u64(small+272,280);
 assert(call(8,small)==-ENOSPC);
 info=malloc(bytes);assert(info);memcpy(info,size_query,264);
 memset(info+264,0xa5,bytes-264);lisa_iorap_wire_u64(info+272,bytes);
 assert(!call(8,info));assert(lisa_iorap_legacy_u32(info+264)==1);
 assert(!strcmp((char *)info+280+32,"/data/test-fixture-0"));
 assert(le64toh(*(__le64 *)(info+280+296))==1);free(info);
 assert(!call(3,size_query));assert(call(7,size_query)==-ENOENT);
 assert(!call(1,input));fail_end=1;assert(call(5,input)==-ENOMEM);fail_end=0;
 assert(!active && atomic_read(&capture_transport_owner)==0);
 assert(!call(1,input));fail_alloc=1;assert(call(5,input)==-ENOMEM);fail_alloc=0;
 assert(!active && atomic_read(&capture_transport_owner)==0);
 assert(!call(1,input));assert(!call(6,input));assert(!active);
 assert(!call(1,input));enabled=0;
 assert(!legacy_ioctl(NULL,LISA_IORAP_NATIVE_SWITCH,(unsigned long)&enabled));
 assert(!active && atomic_read(&capture_transport_owner)==0);
 euid=1000;assert(legacy_open(NULL,NULL)==-EAGAIN);euid=0;
 enabled=1;assert(!legacy_ioctl(NULL,LISA_IORAP_NATIVE_SWITCH,(unsigned long)&enabled));
 assert(legacy_ioctl(NULL,0x80044c14U,0)==-ENOTTY);
 assert(call(17,input)==-ENOTTY && call(18,input)==-ENOTTY);
 /* Real transport accepts more than 64 records, with a bounded payload. */
 for (i=0;i<4;i++) {
  unsigned long records=((unsigned long[]){65,128,256,1024})[i];
  mock_files=records;mock_bits=64;
  request(input,12000+records,77);assert(!call(1,input));assert(!call(5,input));
  request(size_query,12000+records,0);assert(!call(7,size_query));
  bytes=(unsigned long)le64toh(*(__le64 *)(size_query+264));
  assert(bytes==280+records*(296+8));
  info=malloc(bytes);assert(info);memcpy(info,size_query,264);
  lisa_iorap_wire_u64(info+272,bytes);assert(!call(8,info));
  assert(lisa_iorap_legacy_u32(info+264)==records);free(info);
 }
 /* Exercise byte-budget eviction while empty slots still exist. */
 mock_files=64;mock_bits=524288;
 for(i=0;i<7;i++){
  request(input,11000+i,77);assert(!call(1,input));assert(!call(5,input));
  assert(legacy_cache_bytes<=LISA_IORAP_LEGACY_CACHE_BYTES);
 }
 request(size_query,11006,0);assert(!call(7,size_query));
 request(size_query,11000,0);assert(call(7,size_query)==-ENOENT);
 for(i=0;i<LISA_IORAP_LEGACY_SLOTS;i++)legacy_evict(i);
 assert(!legacy_cache_bytes);
 request(size_query,11006,0);slot=legacy_find((void *)size_query);assert(slot==-ENOENT);
 puts("PASS production legacy adapter: UID isolation, cross-fd lifecycle, real bitmap serialization, capacity bounds, timeout ownership, allocation failures and bounded-cache eviction");
 return 0;
}
