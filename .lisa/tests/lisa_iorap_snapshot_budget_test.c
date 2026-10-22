/* SPDX-License-Identifier: GPL-2.0-only */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../mm/lisa_iorap_legacy_wire.h"
int main(void) {
 unsigned long i,bytes=0,size;
 unsigned long long map=1;
 struct lisa_iorap_wire_view *r=calloc(LISA_IORAP_SNAPSHOT_MAX_FILES,sizeof(*r));
 assert(r);
 for(i=0;i<LISA_IORAP_SNAPSHOT_MAX_FILES;i++) {
  assert(!lisa_iorap_snapshot_reserve(i,&bytes,1));
  r[i]=(struct lisa_iorap_wire_view){{1,2,3,64},(const unsigned char*)"/data/fixture",13,&map,1,64,1};
 }
 assert(bytes==8192);
 assert(lisa_iorap_snapshot_reserve(1024,&bytes,1)==-1 && bytes==8192);
 assert(lisa_iorap_snapshot_reserve(0,&bytes,ULONG_MAX)==-1 && bytes==8192);
 assert(!lisa_iorap_wire_size(r,1024,&size) && size==280+1024*304);
 assert(lisa_iorap_wire_size(r,1025,&size)==-1);
 bytes=0;
 for(i=0;i<128;i++)assert(!lisa_iorap_snapshot_reserve(i,&bytes,8192));
 assert(bytes==LISA_IORAP_SNAPSHOT_BITMAP_BYTES);
 assert(lisa_iorap_snapshot_reserve(128,&bytes,1)==-1 && bytes==LISA_IORAP_SNAPSHOT_BITMAP_BYTES);
 assert(!lisa_iorap_snapshot_reserve(128,&bytes,0));
 assert(lisa_iorap_snapshot_reserve(0,NULL,1)==-1);
 for(i=0;i<129;i++){r[i].words=8192;r[i].bits=524288;r[i].metadata[3]=524288;}
 assert(!lisa_iorap_wire_size(r,128,&size));
 assert(size==280+128*(296+65536));
 assert(lisa_iorap_wire_size(r,129,&size)==-2);
 free(r);puts("PASS 1024-file transport, 8MiB bitmap budget, exact bounds and arithmetic overflow");return 0;
}
