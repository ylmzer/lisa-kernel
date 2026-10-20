/* SPDX-License-Identifier: GPL-2.0-only */
/* Actual device tests; no global cache drop, swapoff, reset or app data writes. */
#include <sys/mman.h>
#include <sys/resource.h>
#include <time.h>
struct iorap_rom_request { uint32_t uid,pid;char package[256]; };
struct iorap_rom_size { struct iorap_rom_request request;uint64_t bytes; };
_Static_assert(sizeof(struct iorap_rom_request)==264,"real JNI prefix");
_Static_assert(sizeof(struct iorap_rom_size)==272,"actual size query");
static int rom_open(void)
{
 int fd=open("/dev/iorap_dev",O_RDONLY|O_CLOEXEC);
 require(fd>=0,"open real ROM transport");return fd;
}
static void rom_key(struct iorap_rom_request *r)
{
 memset(r,0,sizeof(*r));r->uid=htole32(getuid());r->pid=htole32(getpid());
 memcpy(r->package,"com.test.iorapfixture",21);
}
static void rom_ioctl(unsigned nr,void *request)
{
 int fd=rom_open();require(!ioctl(fd,0xc1084c00U+nr,request),"ROM ioctl");
 require(!close(fd),"close ROM fd");
}
static uint32_t rom_get32(const unsigned char *p)
{uint32_t v;memcpy(&v,p,4);return le32toh(v);}
static uint64_t rom_get64(const unsigned char *p)
{uint64_t v;memcpy(&v,p,8);return le64toh(v);}
static void rom_capture_test(void)
{
 struct iorap_rom_request r;struct iorap_rom_size q;struct stat st;
 char path[]="/data/local/tmp/lisa-iorap-fixture-XXXXXX";
 unsigned char page[4096],*response;unsigned i,j,found=0;int fd,file,other;
 unsigned pages[]={0,63,64,129};size_t bytes,offset;uint32_t count;
 rom_key(&r);file=mkstemp(path);require(file>=0,"create owned ROM fixture");
 memset(page,0x6d,sizeof(page));
 for(i=0;i<130;i++)require(pwrite(file,page,sizeof(page),i*4096)==sizeof(page),"write fixture");
 require(!fstat(file,&st),"fixture stat");
 rom_ioctl(1,&r); /* This fd is closed before any file reads, just like JNI. */
 other=open("/dev/lisa_iorap_test",O_RDONLY|O_CLOEXEC);
 require(other<0&&errno==EBUSY,"legacy/diagnostic ownership");
 for(i=0;i<sizeof(pages)/sizeof(pages[0]);i++)
  require(pread(file,page,sizeof(page),pages[i]*4096)==sizeof(page),"capture selected pages");
 rom_ioctl(5,&r);
 memset(&q,0,sizeof(q));q.request=r;q.request.pid=0;
 rom_ioctl(7,&q);bytes=(size_t)le64toh(q.bytes);
 require(bytes>=280&&bytes<=10*1024*1024,"ROM size response");
 response=calloc(1,bytes);require(response!=NULL,"allocate ROM response");
 memcpy(response,&q.request,sizeof(q.request));
 {uint64_t capacity=htole64(bytes);memcpy(response+272,&capacity,8);}
 rom_ioctl(8,response);
 count=rom_get32(response+264);require(count<=64&&rom_get64(response+272)==bytes,"ROM info header");
 offset=280;
 for(i=0;i<count;i++){
  unsigned words,bits;const unsigned char *rec;
  require(offset<=bytes&&bytes-offset>=296,"ROM record header");rec=response+offset;
  words=rom_get32(rec+288);bits=(unsigned)rom_get64(rec+24);
  require(bits<=524288&&words==(bits+63)/64&&memchr(rec+32,0,256),"ROM record bounds");
  require(words<=(bytes-offset-296)/8,"ROM record bitmap bounds");
  if(!strcmp((const char *)rec+32,path)){
   require(!found++&&bits==130&&rom_get64(rec)==(uint64_t)st.st_mtim.tv_sec&&
    rom_get64(rec+8)==(uint64_t)st.st_mtim.tv_nsec&&rom_get64(rec+16)==st.st_ino,"actual legacy metadata");
   for(j=0;j<sizeof(pages)/sizeof(pages[0]);j++)
    require((rom_get64(rec+296+(pages[j]/64)*8)>>(pages[j]%64))&1,"legacy captured page");
  }
  offset+=296+(size_t)words*8;
 }
 require(offset==bytes&&found==1,"complete legacy fixture response");
 rom_ioctl(3,&r);fd=rom_open();
 expect_error(fd,0xc1084c07U,(unsigned long)&q,ENOENT);
 expect_error(fd,0x80044c14U,(unsigned long)&q,ENOTTY);
 close(fd);close(file);free(response);
 puts("PASS real legacy cross-fd capture, bitmap, metadata, zero-PID lookup, cleanup and v2 fallback");
 printf("Fixture retained: %s\n",path);
}
static unsigned rom_resident(void *memory,size_t bytes,unsigned char *vec)
{
 size_t i;unsigned count=0;require(!mincore(memory,bytes,vec),"own fixture mincore");
 for(i=0;i<bytes/4096;i++)count+=!!(vec[i]&1);
 return count;
}
static uint64_t rom_checksum(const unsigned char *data,size_t bytes)
{
 size_t i;uint64_t h=1469598103934665603ULL;
 for(i=0;i<bytes;i++)h=(h^data[i])*1099511628211ULL;
 return h;
}
static void rom_pte_test(void)
{
 const size_t bytes=32*1024*1024;size_t i;uint32_t seed=0x746589ab;
 unsigned before,swapped,after,attempt;unsigned char *memory,*vec;
 uint64_t checksum;struct iorap_rom_request r,wrong;int fd;
 struct timespec start,end;
 memory=mmap(NULL,bytes,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
 require(memory!=MAP_FAILED,"allocate private anonymous fixture");
 if(madvise(memory,bytes,MADV_NOHUGEPAGE)){
  require(errno==EINVAL,"optional NOHUGEPAGE hint");
  puts("NOHUGEPAGE hint unavailable; verified test config has THP disabled");
 }
 vec=calloc(bytes/4096,1);require(vec!=NULL,"allocate own residency vector");
 for(i=0;i<bytes;i++){
  seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;memory[i]=(unsigned char)seed;
 }
 checksum=rom_checksum(memory,bytes);before=rom_resident(memory,bytes,vec);
 require(before==bytes/4096,"fixture initially resident");
 rom_key(&r);fd=rom_open();wrong=r;wrong.uid=htole32(getuid()+1);
 expect_error(fd,0xc1084c10U,(unsigned long)&wrong,EPERM);
 wrong=r;wrong.pid=0;expect_error(fd,0xc1084c10U,(unsigned long)&wrong,EINVAL);
 close(fd);rom_ioctl(16,&r);
 require(!madvise(memory,bytes,MADV_PAGEOUT),"page out only owned 32MiB fixture");
 swapped=before;
 for(attempt=0;attempt<50;attempt++){
  swapped=rom_resident(memory,bytes,vec);if(swapped<before)break;usleep(100000);
 }
 require(swapped<before,"fixture must actually leave RAM; no false preload PASS");
 require(!clock_gettime(CLOCK_MONOTONIC,&start),"prefetch clock");
 rom_ioctl(17,&r);
 after=rom_resident(memory,bytes,vec);
 for(attempt=0;attempt<50&&after<before;attempt++){
  usleep(100000);after=rom_resident(memory,bytes,vec);
 }
 require(!clock_gettime(CLOCK_MONOTONIC,&end),"prefetch completion clock");
 require(after>swapped,"real swap-in residency increase before fixture read");
 require(rom_checksum(memory,bytes)==checksum,"anonymous data integrity after swap-in");
 rom_ioctl(18,&r);rom_ioctl(3,&r);
 require(!munmap(memory,bytes),"release own mapping");free(vec);
 printf("PASS real ROM PTE snapshot/swap-in/data integrity: resident=%u pageout=%u preread=%u elapsed_us=%lld\n",
  before,swapped,after,(long long)(end.tv_sec-start.tv_sec)*1000000+(end.tv_nsec-start.tv_nsec)/1000);
}
static int rom_test_mode(const char *mode)
{
 if(!strcmp(mode,"legacy")){rom_capture_test();return 1;}
 if(!strcmp(mode,"pte")){rom_pte_test();return 1;}
 if(!strcmp(mode,"native-enable")||!strcmp(mode,"native-disable")){
  unsigned enabled=!strcmp(mode,"native-enable");int fd=rom_open();
  require(!ioctl(fd,0x40044cfeU,&enabled),"explicit root native-mode switch");close(fd);
  printf("PASS native-mode switch = %u\n",enabled);return 1;
 }
 return 0;
}
