// SPDX-License-Identifier: GPL-3.0-only
#include <windows.h>
#include <winioctl.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include "EndpointCore.h"
#define CHECK(x) do { if(!(x)){printf("FAIL line %d\n",__LINE__);return 1;} } while(0)
int main()
{
    NimEndpointExchange block{};block.Version=NIM_ENDPOINT_VERSION;block.Size=sizeof(block);
    for(unsigned size=0;size<sizeof(block)*2;size++) {
        CHECK(nimValidExchange(&block,size,sizeof(block),0)==(size==sizeof(block)));
        CHECK(nimValidExchange(&block,sizeof(block),size,0)==(size==sizeof(block)));
    }
    block.Sequence=0xffffffffffffffffull;CHECK(nimValidExchange(&block,sizeof(block),sizeof(block),block.Sequence));
    CHECK(!nimValidExchange(&block,sizeof(block),sizeof(block),0));
    block.Sequence=0;
    for(unsigned bit=0;bit<32;bit++) {
        block.Flags=1u<<bit;CHECK(!nimValidExchange(&block,sizeof(block),sizeof(block),0));block.Flags=0;
        block.Reserved=1u<<bit;CHECK(!nimValidExchange(&block,sizeof(block),sizeof(block),0));block.Reserved=0;
    }
    block.Version++;CHECK(!nimValidExchange(&block,sizeof(block),sizeof(block),0));block.Version--;
    block.Size--;CHECK(!nimValidExchange(&block,sizeof(block),sizeof(block),0));block.Size++;
    CHECK(!nimValidExchange(nullptr,sizeof(block),sizeof(block),0));
    unsigned cases=0;
    for(unsigned frames: {1u,47u,128u,240u,256u,480u,960u}) {
        unsigned size=frames*8;
        for(unsigned offset=0;offset<size;offset+=8)for(bool capture:{false,true}) {
            std::vector<unsigned char> ring(size+32,0xa5),audio(NIM_ENDPOINT_FRAMES*8+32,0x5a);
            for(unsigned n=0;n<size;n++)ring[16+n]=static_cast<unsigned char>((n*13+3)%251);
            for(unsigned n=0;n<NIM_ENDPOINT_FRAMES*8;n++)audio[16+n]=static_cast<unsigned char>((n*7+5)%253);
            auto expected=ring;auto expectedAudio=audio;
            for(unsigned n=0;n<NIM_ENDPOINT_FRAMES*8;n++) {
                if(capture)expected[16+(offset+n)%size]=audio[16+n];
                else expectedAudio[16+n]=ring[16+(offset+n)%size];
            }
            unsigned end=nimRingTransfer(ring.data()+16,size,offset,audio.data()+16,NIM_ENDPOINT_FRAMES*8,capture);
            CHECK(end==(offset+NIM_ENDPOINT_FRAMES*8)%size);
            CHECK(ring==expected);CHECK(audio==expectedAudio);cases++;
        }
    }
    printf("Endpoint ABI validation and %u guarded DMA wrap cases passed. No kernel/hardware execution.\n",cases);
    return 0;
}
