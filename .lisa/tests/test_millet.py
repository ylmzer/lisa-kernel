"""Compile and exercise the actual Millet ABI and frame validators."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[2]
harness=r'''#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "uapi/millet.h"
#include "millet_validate.h"
int main(void) {
 struct millet_userconf p={0};
 (void)NAME_ARRAY;
 assert(sizeof(p)==80 && sizeof(struct millet_data)==144);
 assert(offsetof(struct millet_userconf,mod)==72);
 assert(NETLINK_MILLET==31);
 p.owner=SIG_TYPE;p.msg_type=LOOPBACK_MSG;
 p.src_port=MILLET_USER_ID;p.dst_port=MILLET_KERNEL_ID;
 assert(millet_payload_valid(&p));
 assert(millet_frame_valid(0,96,96,1234,1234));
 assert(millet_frame_valid(1000,96,96,1234,1234));
 assert(!millet_frame_valid(10000,96,96,1234,1234));
 assert(!millet_frame_valid(1001,96,96,1234,1234));
 assert(!millet_frame_valid(0,95,96,1234,1234));
 assert(!millet_frame_valid(0,96,97,1234,1234));
 assert(!millet_frame_valid(0,96,95,1234,1234));
 assert(!millet_frame_valid(0,96,96,0,0));
 assert(!millet_frame_valid(0,96,96,1234,1235));
 p.src_port++;assert(!millet_payload_valid(&p));p.src_port--;
 p.dst_port++;assert(!millet_payload_valid(&p));p.dst_port--;
 for(int i=-16;i<32;i++) {p.owner=i;assert(millet_payload_valid(&p)==(i>0&&i<7));}
 p.owner=SIG_TYPE;
 for(int i=-16;i<32;i++) {p.msg_type=i;assert(millet_payload_valid(&p)==(i==1||i==3));}
 puts("Actual Millet ARM64 ABI, credentials, framing, sender and payload rejection tests passed");
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.c').write_text(harness)
 subprocess.run(['gcc','-std=gnu11','-Wall','-Werror','-fsanitize=address,undefined','-I',str(root/'drivers/misc/millet'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
