#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../core/mbox_audio_fifo.h"
static int32_t sample(size_t n,unsigned channel) {
 return channel?(int32_t)(0x80000000u|((uint32_t)((n*7)%0x7fffff)<<8)):
                 (int32_t)((uint32_t)(n%0x7fffff)<<8);
}
int main(void) {
 for(uint32_t rate=44100;rate<=48000;rate+=3900) {
  int32_t txStorage[2048],rxStorage[2048],l[256],r[256];
  mbox_audio_fifo tx,rx;mbox_packet_clock clock;size_t generated=0,verified=0;
  assert(mbox_fifo_init(&tx,txStorage,1024)&&mbox_fifo_init(&rx,rxStorage,1024));
  assert(mbox_clock_init(&clock,rate));
  for(unsigned ms=0;ms<10000;ms++) {
   mbox_packet packet;uint32_t bytes;uint8_t wire[304];
   assert(mbox_plan(&clock,&packet,1,sizeof(wire),&bytes));
   size_t needed=bytes/6;
   while(tx.count<needed) {
    for(size_t n=0;n<256;n++){l[n]=sample(generated+n,0);r[n]=sample(generated+n,1);}
    assert(mbox_fifo_write_planar(&tx,l,r,256));generated+=256;
   }
   assert(mbox_fifo_read_wire(&tx,wire,bytes)==needed);
   assert(mbox_fifo_write_wire(&rx,wire,0,bytes));
   if(rx.count>=256) {
    assert(mbox_fifo_read_planar(&rx,l,r,256)==256);
    for(size_t n=0;n<256;n++){assert(l[n]==sample(verified+n,0));assert(r[n]==sample(verified+n,1));}
    verified+=256;
   }
  }
  size_t remaining=rx.count;
  assert(mbox_fifo_read_planar(&rx,l,r,remaining)==remaining);
  for(size_t n=0;n<remaining;n++){assert(l[n]==sample(verified+n,0));assert(r[n]==sample(verified+n,1));}
  assert(verified+remaining==(size_t)rate*10);
 }
 int32_t storage[8],l[256],r[256];uint8_t wire[304];mbox_audio_fifo q;
 assert(mbox_fifo_init(&q,storage,4));
 for(unsigned n=0;n<4;n++){l[n]=sample(n,0);r[n]=sample(n,1);}
 assert(mbox_fifo_write_planar(&q,l,r,4));mbox_audio_fifo before=q;int32_t copy[8];memcpy(copy,storage,sizeof(copy));
 assert(!mbox_fifo_write_planar(&q,l,r,1));
 assert(!mbox_fifo_write_wire(&q,wire,0,300));
 assert(!mbox_fifo_write_wire(&q,wire,0,304));
 assert(memcmp(&q,&before,sizeof(q))==0&&memcmp(copy,storage,sizeof(copy))==0);
 memset(wire,0xff,sizeof(wire));assert(mbox_fifo_read_wire(&q,wire,288)==4);
 for(unsigned n=24;n<288;n++)assert(wire[n]==0);
 memset(l,0xff,sizeof(l));memset(r,0xff,sizeof(r));assert(mbox_fifo_read_planar(&q,l,r,256)==0);
 for(unsigned n=0;n<256;n++)assert(l[n]==0&&r[n]==0);
 before=q;assert(mbox_fifo_read_wire(&q,wire,304)==SIZE_MAX);assert(memcmp(&q,&before,sizeof(q))==0);
 puts("PASS: 256-frame host blocks round-trip through variable USB packets at both rates; wraparound, signed stereo order, overflow atomicity and zero-filled underruns.");
 return 0;
}
