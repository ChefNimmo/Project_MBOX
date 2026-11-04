// SPDX-License-Identifier: GPL-3.0-only
#pragma once
// Fixed-size, pointer-free boundary. Both sides include Windows/WDK types first.
#define NIM_ENDPOINT_VERSION 1u
#define NIM_ENDPOINT_FRAMES 256u
#define NIM_ENDPOINT_RATE 48000u
#define IOCTL_NIM_EXCHANGE CTL_CODE(FILE_DEVICE_UNKNOWN,0x800,METHOD_BUFFERED,FILE_READ_DATA|FILE_WRITE_DATA)
struct NimEndpointExchange {
    unsigned Version;
    unsigned Size;
    unsigned long long Sequence;
    unsigned Flags; // Input must be zero. Output: bit 0 render RUN, bit 1 capture RUN.
    unsigned Reserved;
    int Capture[NIM_ENDPOINT_FRAMES * 2]; // PCM signed 24 valid bits, MSB aligned in 32.
    int Render[NIM_ENDPOINT_FRAMES * 2];
};
static_assert(sizeof(NimEndpointExchange) == 4120, "Bridge ABI layout changed");
