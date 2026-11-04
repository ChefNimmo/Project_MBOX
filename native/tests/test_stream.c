#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../core/mbox_stream.h"
int main(void) {
 for(uint32_t rate=44100;rate<=48000;rate+=3900) {
  mbox_packet_clock c;mbox_packet p[5];uint32_t used,total=0;
  assert(mbox_clock_init(&c,rate));
  for(int n=0;n<2000;n++) {
   assert(mbox_plan(&c,p,5,5*MBOX_ENDPOINT_MAX,&used));
   for(int i=0;i<5;i++) {
    assert(p[i].offset==(i? p[i-1].offset+p[i-1].length:0));
    assert(p[i].length%6==0 && p[i].length<=304);
    assert(p[i].length==288 || (rate==44100 && (p[i].length==264||p[i].length==270)));
   }
   total+=used;
  }
  assert(total==rate*10*6 && c.phase==0);
 }
 mbox_packet_clock c;mbox_packet p={0};uint32_t used=123;
 assert(mbox_clock_init(&c,44100));p.offset=987;p.length=654;
 assert(!mbox_plan(&c,&p,1,1,&used));
 assert(c.phase==0 && used==123 && p.offset==987 && p.length==654);
 assert(!mbox_clock_init(&c,96000));
 assert(!mbox_plan(&c,&p,0,304,&used));
 assert(!mbox_capture_packet_valid(0,304));
 assert(mbox_capture_packet_valid(0,300));
 assert(!mbox_capture_packet_valid(1,288));
 assert(!mbox_capture_packet_valid(0,306));
 uint8_t wire[6],expect[]={0x7f,0xff,0xff,0x80,0,0};int32_t left,right;
 mbox_pack_frame(wire,0x7fffff00,(int32_t)0x80000000u);
 assert(memcmp(wire,expect,6)==0);mbox_unpack_frame(wire,&left,&right);
 assert(left==0x7fffff00 && (uint32_t)right==0x80000000u);
 puts("PASS: 10-second packet plans at both rates, atomic capacity rejection, invalid capture packets, signed PCM boundary conversion.");
 return 0;
}
