#include <assert.h>
#include <stdio.h>
#include "../core/mbox_test_tone.h"
int main(void) {
 for(uint32_t rate=44100;rate<=48000;rate+=3900) {
  int32_t l[256],r[256];uint64_t position=0,total=(uint64_t)rate*5;int32_t peak=0;
  while(position<total) {
   uint64_t before=position;
   assert(mbox_test_tone(l,r,256,rate,&position,total));
   for(unsigned i=0;i<256;i++) {
    assert(l[i]==r[i]&&l[i]>=-21474837&&l[i]<=21474837);
    if(before+i>=total)assert(l[i]==0);
    if(l[i]>peak)peak=l[i];
   }
  }
  assert(position==total&&peak>20000000);
  assert(mbox_test_tone(l,r,256,rate,&position,total));
  for(unsigned i=0;i<256;i++)assert(l[i]==0&&r[i]==0);
  assert(position==total);
 }
 /* Rendering in different host block sizes preserves phase and fade state. */
 int32_t a[64],b[64],r[64];uint64_t p=5000,q=5000;
 assert(mbox_test_tone(a,r,64,48000,&p,240000));
 for(unsigned i=0;i<64;i++)assert(mbox_test_tone(b+i,r+i,1,48000,&q,240000));
 for(unsigned i=0;i<64;i++)assert(a[i]==b[i]);
 puts("PASS: diagnostic tone stays below 1% full scale, ends in silence, matches stereo channels and preserves phase across blocks.");
 return 0;
}
