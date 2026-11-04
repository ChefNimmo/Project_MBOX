#pragma once
#include <windows.h>
#include <stdint.h>
#include "NimBoxIds.h"
// Called only by the transport worker; arrays are valid for this call only.
typedef void (*NimAudioCallback)(void*,const int32_t*,const int32_t*,int32_t*,int32_t*);
typedef void (*NimErrorCallback)(void*);
struct NimEngine;
NimEngine *nimCreate(const wchar_t *helper);
void nimDestroy(NimEngine*);
bool nimStart(NimEngine*,unsigned rate,NimAudioCallback,void*,NimErrorCallback=nullptr);
void nimStop(NimEngine*,bool releaseDevice=false);
bool nimOnWorker(NimEngine*);
void nimStats(NimEngine*,NimDiagnostics*);
