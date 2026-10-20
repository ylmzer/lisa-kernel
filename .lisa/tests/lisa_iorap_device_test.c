/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include <errno.h>
#include <endian.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../../include/uapi/linux/lisa_iorap_test.h"
_Static_assert(sizeof(struct lisa_iorap_test_header) == 24, "header ABI");
_Static_assert(sizeof(struct lisa_iorap_test_record) == 304, "record ABI");
static void require(int ok, const char *what)
{
 if (!ok) { perror(what); exit(1); }
}
static int open_device(void)
{
 int fd=open("/dev/lisa_iorap_test", O_RDWR|O_CLOEXEC);
 require(fd>=0,"open diagnostic device (root + enforcing policy required)");
 return fd;
}
static void expect_error(int fd, unsigned long cmd, unsigned long arg, int error)
{
 errno=0;
 require(ioctl(fd,cmd,arg)==-1 && errno==error,"expected ioctl rejection");
}
#include "lisa_iorap_legacy_device_test.h"
static void decode(const unsigned char *data, size_t bytes,
                   const char *fixture, const struct stat *st)
{
 const struct lisa_iorap_test_header *h=(const void *)data;
 size_t offset=sizeof(*h), i, j;
 unsigned found=0;
 const unsigned pages[]={0,63,64,129};
 require(bytes>=sizeof(*h),"short response");
 require(le32toh(h->magic)==LISA_IORAP_TEST_MAGIC && le32toh(h->version)==1 &&
         le64toh(h->bytes)==bytes && le32toh(h->files)<=64,"response header");
 for(i=0;i<le32toh(h->files);i++) {
  const struct lisa_iorap_test_record *r;
  uint32_t words,bits;
  require(bytes-offset>=sizeof(*r),"short record");
  r=(const void *)(data+offset); words=le32toh(r->words); bits=le32toh(r->bits);
  require(bits<=524288 && words==(bits+63)/64 &&
          memchr(r->path,0,sizeof(r->path)) && !r->reserved,"record bounds");
  offset+=sizeof(*r);
  require(words<=(bytes-offset)/8,"short bitmap");
  if(fixture && !strcmp(r->path,fixture)) {
   require(!found++ && bits==130 && le64toh(r->file_bytes)==130*4096 &&
           le64toh(r->inode)==(uint64_t)st->st_ino &&
           le64toh(r->mtime_sec)==(uint64_t)st->st_mtim.tv_sec &&
           le32toh(r->mtime_nsec)==(uint32_t)st->st_mtim.tv_nsec,"fixture identity");
   for(j=0;j<sizeof(pages)/sizeof(pages[0]);j++) {
    uint64_t word;
    memcpy(&word,data+offset+(pages[j]/64)*8,8);
    require((le64toh(word)>>(pages[j]%64))&1,"missing captured page");
   }
  }
  offset+=(size_t)words*8;
 }
 require(offset==bytes && (!fixture || found==1),"complete response/fixture");
 puts("PASS diagnostic response structure and requested fixture pages");
}
int main(int argc,char **argv)
{
 int fd,other,fixture,status;
 unsigned i,j;
 unsigned char page[4096],*data;
 const unsigned pages[]={0,63,64,129};
 char path[]="/data/local/tmp/lisa-iorap-fixture-XXXXXX";
 struct stat st;
 size_t bytes=0;
 ssize_t got;
 pid_t child;
 if(argc!=2) { fprintf(stderr,"Usage: %s layout|capture|close|crash|timeout\n",argv[0]); return 2; }
 if(rom_test_mode(argv[1])) return 0;
 if(!strcmp(argv[1],"layout")) { puts("PASS diagnostic UAPI 24/304-byte layout"); return 0; }
 if(strcmp(argv[1],"capture") && strcmp(argv[1],"close") &&
    strcmp(argv[1],"crash") && strcmp(argv[1],"timeout")) return 2;
 if(!strcmp(argv[1],"crash")) {
  child=fork(); require(child>=0,"fork");
  if(!child) { fd=open_device(); require(!ioctl(fd,LISA_IORAP_TEST_START,0),"child start"); _exit(0); }
  require(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status),"child exit");
  fd=open_device(); require(!ioctl(fd,LISA_IORAP_TEST_START,0),"restart after process exit");
  require(!close(fd),"close"); puts("PASS process-exit release and restart"); return 0;
 }
 fd=open_device();
 other=open("/dev/lisa_iorap_test",O_RDWR|O_CLOEXEC);
 require(other<0 && errno==EBUSY,"exclusive open");
 expect_error(fd,_IO('Q',99),0,ENOTTY);
 expect_error(fd,LISA_IORAP_TEST_START,1,EINVAL);
 if(strcmp(argv[1],"capture")) {
  require(!ioctl(fd,LISA_IORAP_TEST_START,0),"start");
  if(!strcmp(argv[1],"timeout")) {
   sleep(32); expect_error(fd,LISA_IORAP_TEST_STOP,0,ENOENT);
   require(!ioctl(fd,LISA_IORAP_TEST_START,0),"restart after timeout");
  }
  require(!close(fd),"release active capture");
  fd=open_device(); require(!ioctl(fd,LISA_IORAP_TEST_START,0),"restart after close");
  require(!ioctl(fd,LISA_IORAP_TEST_ABORT,0),"abort");
  expect_error(fd,LISA_IORAP_TEST_STOP,0,ENOENT);
  close(fd); puts("PASS close/abort/timeout as selected"); return 0;
 }
 fixture=mkstemp(path); require(fixture>=0,"create unique fixture");
 memset(page,0x5a,sizeof(page));
 for(i=0;i<130;i++) require(write(fixture,page,sizeof(page))==sizeof(page),"write fixture");
 require(!fsync(fixture) && !fstat(fixture,&st),"fixture metadata");
 require(!ioctl(fd,LISA_IORAP_TEST_START,0),"start capture");
 expect_error(fd,LISA_IORAP_TEST_START,0,EBUSY);
 for(j=0;j<2;j++) for(i=0;i<4;i++)
  require(pread(fixture,page,sizeof(page),(off_t)pages[i]*4096)==sizeof(page),"read fixture");
 require(!ioctl(fd,LISA_IORAP_TEST_STOP,0),"stop capture (overflow/stale is a real failure)");
 data=malloc(LISA_IORAP_TEST_MAX_BYTES); require(data!=NULL,"allocate response");
 while(bytes<LISA_IORAP_TEST_MAX_BYTES && (got=read(fd,data+bytes,LISA_IORAP_TEST_MAX_BYTES-bytes))>0)
  bytes+=(size_t)got;
 require(bytes<LISA_IORAP_TEST_MAX_BYTES && got==0,"read response");
 decode(data,bytes,path,&st); free(data);
 require(!ioctl(fd,LISA_IORAP_TEST_ABORT,0),"discard result");
 close(fixture); close(fd);
 /* Unique fixture is deliberately retained: no cleanup of user files. */
 printf("Fixture retained: %s\n",path);
 return 0;
}
