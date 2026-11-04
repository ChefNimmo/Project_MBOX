#pragma once
#include <windows.h>
// This identity belongs to NimBox and never replaces another vendor's COM key.
static const GUID CLSID_NimBox={0x90ab4c67,0x2788,0x4148,{0xa3,0xd0,0x69,0xa2,0x17,0xc2,0x3e,0xa5}};
#define NIMBOX_CLSID_TEXT L"{90AB4C67-2788-4148-A3D0-69A217C23EA5}"
#define NIMBOX_NAME L"NimBox 1 USB audio device"
struct NimDiagnostics {
 unsigned Version,Running,Error,Rate;
 unsigned long long InputFrames,OutputFrames,Callbacks,Underruns,Overflows,MutedFrames;
};
