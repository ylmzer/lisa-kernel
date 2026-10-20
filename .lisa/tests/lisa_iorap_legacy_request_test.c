/* SPDX-License-Identifier: GPL-2.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../mm/lisa_iorap_legacy_request.h"
static void put64(unsigned char *p, unsigned long long value)
{
	unsigned i;
	for (i=0;i<8;i++)p[i]=(unsigned char)(value>>(i*8));
}
int main(void)
{
	unsigned char storage[282], *data=storage+1;
	struct lisa_iorap_legacy_request r,before,consumer;
	unsigned long capacity=123,i;
	const unsigned commands[]={1,3,5,6,7,8,9,16,17,18};
	const unsigned rejected[]={0xc1084c02U,0xc1084c04U,0xc1084c0fU,
		0xc1084c13U,0x80044c14U};
	memset(data,0,280);data[0]=0x39;data[1]=0x30;data[4]=0x78;data[5]=0x56;
	memcpy(data+8,"com.example.launch",19);
	for(i=0;i<264;i++){
		memset(&r,0xa5,sizeof(r));before=r;
		assert(lisa_iorap_legacy_decode(data,i,&r)==-1);
		assert(!memcmp(&r,&before,sizeof(r)));
	}
	assert(!lisa_iorap_legacy_decode(data,264,&r));
	assert(r.uid==12345 && r.pid==22136);
	consumer=r;consumer.pid=0;assert(lisa_iorap_legacy_same_app(&r,&consumer));
	consumer.uid++;assert(!lisa_iorap_legacy_same_app(&r,&consumer));
	consumer=r;consumer.package[0]='x';assert(!lisa_iorap_legacy_same_app(&r,&consumer));
	memset(data+8,'a',255);data[263]=0;assert(!lisa_iorap_legacy_decode(data,264,&r));
	data[263]='a';assert(lisa_iorap_legacy_decode(data,264,&r)==-1);
	data[8]=0;assert(lisa_iorap_legacy_decode(data,264,&r)==-1);
	memcpy(data+8,"../escape",10);assert(lisa_iorap_legacy_decode(data,264,&r)==-1);
	memcpy(data+8,"valid.package",14);data[3]=0x80;
	assert(lisa_iorap_legacy_decode(data,264,&r)==-1);data[3]=0;
	data[7]=0x80;assert(lisa_iorap_legacy_decode(data,264,&r)==-1);data[7]=0;
	for(i=0;i<256;i++){
		data[8]=(unsigned char)i;
		int valid=(i>='a'&&i<='z')||(i>='A'&&i<='Z')||(i>='0'&&i<='9')||i=='.'||i=='_';
		assert((lisa_iorap_legacy_decode(data,264,&r)==0)==valid);
	}
	memset(data+264,0xa5,8); /* Real malloc output fields are not inputs. */
	for(i=0;i<280;i++)assert(lisa_iorap_legacy_capacity(data,i,&capacity)==-1);
	for(i=0;i<280;i++){
		put64(data+272,i);capacity=123;
		assert(lisa_iorap_legacy_capacity(data,280,&capacity)==-1 && capacity==123);
	}
	put64(data+272,280);assert(!lisa_iorap_legacy_capacity(data,280,&capacity)&&capacity==280);
	put64(data+272,LISA_IORAP_LEGACY_LIMIT);
	assert(!lisa_iorap_legacy_capacity(data,280,&capacity)&&capacity==LISA_IORAP_LEGACY_LIMIT);
	put64(data+272,LISA_IORAP_LEGACY_LIMIT+1);assert(lisa_iorap_legacy_capacity(data,280,&capacity)==-1);
	put64(data+272,~0ULL);assert(lisa_iorap_legacy_capacity(data,280,&capacity)==-1);
	for(i=0;i<sizeof(commands)/sizeof(commands[0]);i++){
		unsigned cmd=0xc1084c00U+commands[i],bit;
		assert(lisa_iorap_legacy_operation(cmd)!=LISA_IORAP_LEGACY_UNSUPPORTED);
		for(bit=8;bit<32;bit++)assert(lisa_iorap_legacy_operation(cmd^(1U<<bit))==LISA_IORAP_LEGACY_UNSUPPORTED);
	}
	for(i=0;i<sizeof(rejected)/sizeof(rejected[0]);i++)
		assert(lisa_iorap_legacy_operation(rejected[i])==LISA_IORAP_LEGACY_UNSUPPORTED);
	assert(lisa_iorap_legacy_decode(NULL,264,&r)==-1);
	puts("PASS legacy UID/PID, package bounds, unaligned input, actual capacity, exact ioctl dispatch and cache identity");
	return 0;
}
