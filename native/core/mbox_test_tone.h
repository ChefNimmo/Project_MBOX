#ifndef MBOX_TEST_TONE_H
#define MBOX_TEST_TONE_H
#include <math.h>
#include <stdint.h>
#include <stddef.h>
/* Explicit diagnostic only: 750 Hz, 1% full scale (-40 dBFS peak), stereo,
 * twenty-millisecond fades. No gain or persisted audio settings are changed. */
static inline int mbox_test_tone(int32_t *left,int32_t *right,size_t frames,
                                uint32_t rate,uint64_t *position,uint64_t total) {
 if(!left||!right||!position||(rate!=44100&&rate!=48000))return 0;
 uint64_t n=*position,fade=rate/50;
 for(size_t i=0;i<frames;i++) {
  int32_t sample=0;
  if(n<total) {
   double gain=1.0;
   if(n<fade)gain=(double)n/(double)fade;
   uint64_t remaining=total-1-n;
   if(remaining<fade&&((double)remaining/(double)fade)<gain)gain=(double)remaining/(double)fade;
   double phase=6.2831853071795864769*750.0*(double)(n%rate)/(double)rate;
   sample=(int32_t)(sin(phase)*2147483647.0*0.01*gain);n++;
  }
  left[i]=right[i]=sample;
 }
 *position=n;return 1;
}
#endif
