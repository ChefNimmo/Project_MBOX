#ifndef MBOX_AUDIO_FIFO_H
#define MBOX_AUDIO_FIFO_H
#include "mbox_stream.h"
/* Stereo frame adapter between planar PCM32 host blocks and BE24 USB packets.
 * Single owner only: the eventual ASIO/USB thread boundary requires explicit
 * synchronization. No allocation, locks or sample-rate conversion occurs here.
 * Caller-provided storage and buffers must not overlap. */
typedef struct {
 int32_t *data;size_t capacity,read,write,count;
} mbox_audio_fifo;
static inline int mbox_fifo_init(mbox_audio_fifo *q,int32_t *storage,size_t frames) {
 if(!q||!storage||!frames||frames>SIZE_MAX/(2*sizeof(int32_t)))return 0;
 q->data=storage;q->capacity=frames;q->read=q->write=q->count=0;return 1;
}
/* Writes are all-or-nothing, including capture overflow/malformed packets. */
static inline int mbox_fifo_write_planar(mbox_audio_fifo *q,const int32_t *l,const int32_t *r,size_t frames) {
 if(!q||!l||!r||frames>q->capacity-q->count)return 0;
 for(size_t i=0;i<frames;i++) {
  q->data[2*q->write]=l[i];q->data[2*q->write+1]=r[i];
  if(++q->write==q->capacity)q->write=0;
 }
 q->count+=frames;return 1;
}
static inline int mbox_fifo_write_wire(mbox_audio_fifo *q,const uint8_t *wire,uint32_t status,uint32_t bytes) {
 size_t frames=bytes/6;
 if(!q||!wire||!mbox_capture_packet_valid(status,bytes)||frames>q->capacity-q->count)return 0;
 for(size_t i=0;i<frames;i++) {
  mbox_unpack_frame(wire+i*6,&q->data[2*q->write],&q->data[2*q->write+1]);
  if(++q->write==q->capacity)q->write=0;
 }
 q->count+=frames;return 1;
}
/* Reads zero-fill shortages so stale data is never replayed. The return value
 * is the actual number of consumed frames, which the caller uses for xrun stats. */
static inline size_t mbox_fifo_read_planar(mbox_audio_fifo *q,int32_t *l,int32_t *r,size_t frames) {
 if(!q||!l||!r)return SIZE_MAX;
 size_t used=frames<q->count?frames:q->count;
 for(size_t i=0;i<used;i++) {
  l[i]=q->data[2*q->read];r[i]=q->data[2*q->read+1];
  if(++q->read==q->capacity)q->read=0;
 }
 for(size_t i=used;i<frames;i++)l[i]=r[i]=0;
 q->count-=used;return used;
}
static inline size_t mbox_fifo_read_wire(mbox_audio_fifo *q,uint8_t *wire,uint32_t bytes) {
 if(!q||!wire||!bytes||!mbox_capture_packet_valid(0,bytes))return SIZE_MAX;
 size_t frames=bytes/6,used=frames<q->count?frames:q->count;
 for(size_t i=0;i<used;i++) {
  mbox_pack_frame(wire+i*6,q->data[2*q->read],q->data[2*q->read+1]);
  if(++q->read==q->capacity)q->read=0;
 }
 for(size_t i=used*6;i<bytes;i++)wire[i]=0;
 q->count-=used;return used;
}
#endif
