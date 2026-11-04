#include <assert.h>
#include <stdio.h>
#include "../core/mbox_gate.h"
int main(void) {
 /* Exhaust the legal state space: each direction has 0..4 active requests.
  * Verify capacity/exclusion/underflow and release independent of direction. */
 for(unsigned out=0;out<=4;out++)for(unsigned in=0;in<=4;in++) {
  uint32_t state=out+(in<<4),next=0xabcdefu;
  for(unsigned dir=0;dir<2;dir++) {
   unsigned count=dir?in:out;uint32_t step=dir?16u:1u;
   assert(mbox_gate_acquire(state,dir,&next)==(count<4));
   if(count<4)assert(next==state+step);
   assert(mbox_gate_release(state,dir,&next)==(count>0));
   if(count>0)assert(next==state-step);
  }
 }
 uint32_t next=123;
 assert(!mbox_gate_acquire(MBOX_CONTROL_GATE,0,&next)&&next==123);
 assert(!mbox_gate_acquire(MBOX_CONTROL_GATE,1,&next));
 assert(!mbox_gate_release(MBOX_CONTROL_GATE,0,&next));
 assert(!mbox_gate_acquire(0,2,&next));
 assert(!mbox_gate_acquire(0x200,0,&next));
 assert(!mbox_gate_acquire(5,1,&next));
 assert(!mbox_gate_release(0x50,0,&next));
 puts("PASS: all 25 duplex admission states, four-request bounds, control exclusion and underflow rejection.");
 return 0;
}
