#ifndef MBOX_STREAM_H
#define MBOX_STREAM_H
#include <stddef.h>
#include <stdint.h>

/* Independent transport primitives. No Ploytec code or licensing logic.
 * Planner assumes synchronous one-millisecond USB frames, as in the historical
 * ALSA playback quirk. This timing assumption still needs live validation. */
typedef struct { uint32_t rate, phase; } mbox_packet_clock;
typedef struct { uint32_t offset, length; } mbox_packet;
enum { MBOX_FRAME_BYTES=6, MBOX_ENDPOINT_MAX=304, MBOX_MAX_BATCH=32 };

static int mbox_clock_init(mbox_packet_clock *clock,uint32_t rate) {
 if(!clock || (rate!=44100 && rate!=48000)) return 0;
 clock->rate=rate;clock->phase=0;return 1;
}
/* All-or-nothing: no caller state/output changes when capacity is insufficient. */
static int mbox_plan(mbox_packet_clock *clock,mbox_packet *packets,size_t count,
                    uint32_t capacity,uint32_t *used) {
 mbox_packet_clock next;mbox_packet temporary[MBOX_MAX_BATCH];uint32_t total=0;
 if(!clock||!packets||!used||count==0||count>MBOX_MAX_BATCH||clock->phase>=1000||
    (clock->rate!=44100&&clock->rate!=48000))return 0;
 next=*clock;
 for(size_t i=0;i<count;i++) {
  uint32_t sum=next.phase+next.rate;
  uint32_t bytes=(sum/1000)*MBOX_FRAME_BYTES;next.phase=sum%1000;
  if(bytes>MBOX_ENDPOINT_MAX||total>capacity||bytes>capacity-total)return 0;
  temporary[i].offset=total;temporary[i].length=bytes;total+=bytes;
 }
 for(size_t i=0;i<count;i++)packets[i]=temporary[i];
 *clock=next;*used=total;return 1;
}
static int mbox_capture_packet_valid(uint32_t status,uint32_t bytes) {
 return status==0 && bytes<=MBOX_ENDPOINT_MAX && bytes%MBOX_FRAME_BYTES==0;
}
/* Buffers must not overlap. Host samples are signed PCM32, MSB-aligned 24-bit.
 * Shift uses unsigned arithmetic; the low 8 host bits are discarded. */
static void mbox_pack_frame(uint8_t wire[6],int32_t left,int32_t right) {
 uint32_t l=(uint32_t)left,r=(uint32_t)right;
 wire[0]=(uint8_t)(l>>24);wire[1]=(uint8_t)(l>>16);wire[2]=(uint8_t)(l>>8);
 wire[3]=(uint8_t)(r>>24);wire[4]=(uint8_t)(r>>16);wire[5]=(uint8_t)(r>>8);
}
static void mbox_unpack_frame(const uint8_t wire[6],int32_t *left,int32_t *right) {
 uint32_t l=((uint32_t)wire[0]<<24)|((uint32_t)wire[1]<<16)|((uint32_t)wire[2]<<8);
 uint32_t r=((uint32_t)wire[3]<<24)|((uint32_t)wire[4]<<16)|((uint32_t)wire[5]<<8);
 *left=(int32_t)l;*right=(int32_t)r;
}
#endif
