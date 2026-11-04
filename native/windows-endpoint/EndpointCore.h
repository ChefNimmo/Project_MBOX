// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "BridgeProtocol.h"
inline bool nimValidExchange(const NimEndpointExchange* block, unsigned inputBytes,
    unsigned outputBytes, unsigned long long sequence)
{
    return block && inputBytes==sizeof(*block) && outputBytes==sizeof(*block) &&
        block->Size==sizeof(*block) && block->Version==NIM_ENDPOINT_VERSION &&
        !block->Flags && !block->Reserved && block->Sequence==sequence;
}
// Same bounded copy routine used by the kernel and the guard-buffer tests.
// Caller validates lifetime/locking. Amount can wrap more than once.
inline unsigned nimRingTransfer(unsigned char* ring,unsigned size,unsigned offset,
    unsigned char* audio,unsigned amount,bool capture)
{
    if(!ring||!audio||!size||size%8||offset>=size||offset%8||amount%8)return offset;
    for(unsigned done=0;done<amount;) {
        unsigned count=amount-done;
        if(count>size-offset)count=size-offset;
        if(capture)memcpy(ring+offset,audio+done,count);
        else memcpy(audio+done,ring+offset,count);
        done+=count;offset=(offset+count)%size;
    }
    return offset;
}
