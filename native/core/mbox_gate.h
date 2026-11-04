#ifndef MBOX_GATE_H
#define MBOX_GATE_H
#include <stdint.h>
/* Pure admission transitions. Caller supplies atomic compare/exchange or a
 * single-threaded owner. Four pending eight-frame URBs per direction. */
enum { MBOX_PIPE_DEPTH=4, MBOX_CONTROL_GATE=0x100 };
static inline int mbox_gate_acquire(uint32_t state,unsigned direction,uint32_t *next) {
 unsigned shift=direction?4u:0u;
 if(!next||direction>1||state&MBOX_CONTROL_GATE||state&~0x1ffu||
    (state&15u)>MBOX_PIPE_DEPTH||((state>>4)&15u)>MBOX_PIPE_DEPTH||
    ((state>>shift)&15u)>=MBOX_PIPE_DEPTH)return 0;
 *next=state+(1u<<shift);return 1;
}
static inline int mbox_gate_release(uint32_t state,unsigned direction,uint32_t *next) {
 unsigned shift=direction?4u:0u;
 if(!next||direction>1||state&MBOX_CONTROL_GATE||state&~0x1ffu||
    (state&15u)>MBOX_PIPE_DEPTH||((state>>4)&15u)>MBOX_PIPE_DEPTH||
    ((state>>shift)&15u)==0)return 0;
 *next=state-(1u<<shift);return 1;
}
#endif
