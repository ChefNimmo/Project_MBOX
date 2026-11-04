// SPDX-License-Identifier: GPL-3.0-only
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <avrt.h>
#include <atomic>
#include <process.h>
#include <new>
#include <stdio.h>
#include <string.h>
#include "Engine.h"
#include "../transport/types.h"
#include "../core/mbox_stream.h"
#include "../core/mbox_audio_fifo.h"
#include "../kmdf/usbdk_backend.h"
struct NimEngine {
 UDK Usb{};HANDLE Thread=nullptr,Stop=nullptr,Ready=nullptr;unsigned ThreadId=0;
 SLOT Slots[SLOT_COUNT]{};HANDLE Events[SLOT_COUNT]{};
 int32_t TxData[16384]{},RxData[16384]{},Left[256]{},Right[256]{},OutL[256]{},OutR[256]{};
 mbox_audio_fifo Tx{},Rx{};mbox_packet_clock Clock{};
 ULONG Feedback[64][MBOX_PACKET_COUNT]{};unsigned FHead=0,FTail=0,FCount=0;
 bool SeenCapture=false;unsigned Rate=48000;NimAudioCallback Callback=nullptr;NimErrorCallback OnError=nullptr;void *User=nullptr;
 std::atomic<unsigned> Running{0},Error{0};
 std::atomic<uint64_t> In{0},Out{0},Calls{0},Under{0},Over{0},Muted{0};
};
static void drain(NimEngine *e) {
 BACKEND backend{};backend.Context=&e->Usb;
 for(auto &s:e->Slots)if(s.Active)udk_cancel(&backend,&s);
 for(auto &s:e->Slots)if(s.Active){DWORD bytes;udk_finish(&backend,&s,&bytes);s.Active=0;}
}
static bool submit(NimEngine *e,SLOT &s) {
 memset(&s.Input,0,sizeof(s.Input));memset(&s.Output,0,sizeof(s.Output));
 s.Input.Version=MBOX_ABI_VERSION;s.Input.Direction=s.Direction;
 if(!s.Direction) {
  ULONG lengths[MBOX_PACKET_COUNT];
  if(e->FCount) {
   memcpy(lengths,e->Feedback[e->FHead],sizeof(lengths));
   e->FHead=(e->FHead+1)%64;e->FCount--;
  } else {
   mbox_packet packets[MBOX_PACKET_COUNT];uint32_t used;
   if(!mbox_plan(&e->Clock,packets,MBOX_PACKET_COUNT,MBOX_BUFFER_BYTES,&used))return false;
   for(unsigned p=0;p<MBOX_PACKET_COUNT;p++)lengths[p]=packets[p].length;
  }
  ULONG offset=0;
  for(unsigned p=0;p<MBOX_PACKET_COUNT;p++) {
   s.Input.Length[p]=lengths[p];
   size_t used=mbox_fifo_read_wire(&e->Tx,s.Input.Data+offset,lengths[p]);
   if(used==SIZE_MAX)return false;
   if(used<lengths[p]/6)e->Under.fetch_add(lengths[p]/6-used);
   offset+=lengths[p];
  }
 }
 BACKEND backend{};backend.Context=&e->Usb;
 if(!udk_submit(&backend,&s))return false;
 s.Active=1;return true;
}
static bool complete(NimEngine *e,SLOT &s) {
 DWORD bytes=0;BACKEND backend{};backend.Context=&e->Usb;
 bool ok=udk_finish(&backend,&s,&bytes)!=0;s.Active=0;
 if(!ok||bytes!=sizeof(s.Output)||s.Output.UsbStatus||s.Output.ErrorCount)return false;
 for(unsigned p=0;p<MBOX_PACKET_COUNT;p++) {
  if(s.Output.Status[p])return false;
  if(!s.Direction){e->Out.fetch_add(s.Input.Length[p]/6);continue;}
  ULONG length=s.Output.Length[p];
  if(s.Output.Offset[p]!=p*304||!length||!mbox_capture_packet_valid(0,length))return false;
  if(!mbox_fifo_write_wire(&e->Rx,s.Output.Data+p*304,0,length)){e->Over++;return false;}
  e->In.fetch_add(length/6);
  while(e->Rx.count>=256) {
   if(mbox_fifo_read_planar(&e->Rx,e->Left,e->Right,256)!=256)return false;
   // Startup transient is intentionally muted, never corrected by a noise filter.
   if(e->Calls.load()*256<(uint64_t)e->Rate*10) {
    memset(e->Left,0,sizeof(e->Left));memset(e->Right,0,sizeof(e->Right));e->Muted.fetch_add(256);
   }
   memset(e->OutL,0,sizeof(e->OutL));memset(e->OutR,0,sizeof(e->OutR));
   e->Callback(e->User,e->Left,e->Right,e->OutL,e->OutR);e->Calls++;
   if(!mbox_fifo_write_planar(&e->Tx,e->OutL,e->OutR,256)){e->Over++;return false;}
  }
 }
 if(s.Direction) {
  if(e->FCount==64){e->Over++;return false;}
  for(unsigned p=0;p<MBOX_PACKET_COUNT;p++)e->Feedback[e->FTail][p]=s.Output.Length[p];
  e->FTail=(e->FTail+1)%64;e->FCount++;e->SeenCapture=true;
 }
 return true;
}
static unsigned __stdcall worker(void *context) {
 auto e=static_cast<NimEngine*>(context);DWORD task=0;
 HANDLE mmcss=AvSetMmThreadCharacteristicsW(L"Pro Audio",&task);
 // A bounded silent reserve covers the initial OUT queue plus callback jitter.
 for(unsigned n=0;n<10;n++)mbox_fifo_write_planar(&e->Tx,e->OutL,e->OutR,256);
 bool ok=true;
 for(unsigned j=0;j<SLOT_COUNT;j++) {
  auto &s=e->Slots[(j+MBOX_PIPE_DEPTH)%SLOT_COUNT];
  if(!submit(e,s)){ok=false;break;}
 }
 e->Running=ok?1:0;if(!ok)e->Error=1;SetEvent(e->Ready);
 unsigned idle=0;
 while(ok&&WaitForSingleObject(e->Stop,0)!=WAIT_OBJECT_0) {
  HANDLE handles[SLOT_COUNT+1];unsigned map[SLOT_COUNT+1],count=1;
  handles[0]=e->Stop;map[0]=SLOT_COUNT;
  for(unsigned j=0;j<SLOT_COUNT;j++) {
   unsigned i=(j+MBOX_PIPE_DEPTH)%SLOT_COUNT;
   auto &s=e->Slots[i];
   if(!s.Active&&(s.Direction||!e->SeenCapture||e->FCount))if(!submit(e,s)){ok=false;break;}
   if(s.Active){handles[count]=s.Ov.hEvent;map[count++]=i;}
  }
  if(!ok)break;
  DWORD wait=WaitForMultipleObjects(count,handles,FALSE,100);
  if(wait==WAIT_TIMEOUT){if(++idle>=50)ok=false;continue;}
  idle=0;if(wait==WAIT_OBJECT_0)break;
  if(wait<WAIT_OBJECT_0+count)ok=complete(e,e->Slots[map[wait-WAIT_OBJECT_0]]);
  else ok=false;
 }
 if(!ok)e->Error=1;
 drain(e);e->Running=0;
 if(!ok&&e->OnError)e->OnError(e->User);
 if(mmcss)AvRevertMmThreadCharacteristics(mmcss);return 0;
}
NimEngine *nimCreate(const wchar_t *helper) {
 auto e=new(std::nothrow) NimEngine;if(!e)return nullptr;
 char path[2048];
 if(!WideCharToMultiByte(CP_ACP,WC_NO_BEST_FIT_CHARS,helper,-1,path,sizeof(path),nullptr,nullptr)||
    !udk_load(&e->Usb,path)||!udk_find(&e->Usb,0)){nimDestroy(e);return nullptr;}
 e->Stop=CreateEventW(nullptr,TRUE,FALSE,nullptr);e->Ready=CreateEventW(nullptr,TRUE,FALSE,nullptr);
 if(!e->Stop||!e->Ready){nimDestroy(e);return nullptr;}
 for(unsigned i=0;i<SLOT_COUNT;i++) {
  e->Slots[i].Index=i;e->Slots[i].Direction=i/MBOX_PIPE_DEPTH;
  e->Events[i]=CreateEventW(nullptr,TRUE,FALSE,nullptr);e->Slots[i].Ov.hEvent=e->Events[i];
  if(!e->Events[i]){nimDestroy(e);return nullptr;}
 }
 return e;
}
bool nimOnWorker(NimEngine *e){return e&&e->Thread&&GetCurrentThreadId()==e->ThreadId;}
void nimStop(NimEngine *e,bool releaseDevice) {
 if(!e)return;
 if(e->Thread) {
  SetEvent(e->Stop);if(nimOnWorker(e))return;
  WaitForSingleObject(e->Thread,INFINITE);CloseHandle(e->Thread);e->Thread=nullptr;e->ThreadId=0;
 }
 if(releaseDevice&&e->Usb.Device&&e->Usb.Device!=INVALID_HANDLE_VALUE){e->Usb.Stop(e->Usb.Device);e->Usb.Device=INVALID_HANDLE_VALUE;}
}
void nimDestroy(NimEngine *e) {
 if(!e)return;nimStop(e,true);
 for(HANDLE event:e->Events)if(event)CloseHandle(event);
 if(e->Stop)CloseHandle(e->Stop);if(e->Ready)CloseHandle(e->Ready);udk_close(&e->Usb);delete e;
}
bool nimStart(NimEngine *e,unsigned rate,NimAudioCallback callback,void *user,NimErrorCallback onError) {
 if(!e||e->Thread||!callback)return false;
 if(!e->Usb.Device||e->Usb.Device==INVALID_HANDLE_VALUE) {
  if(!udk_find(&e->Usb,1)||!udk_rate(&e->Usb,rate)){nimStop(e,true);return false;}
 } else if(e->Rate!=rate&&!udk_rate(&e->Usb,rate)){nimStop(e,true);return false;}
 e->Rate=rate;e->Callback=callback;e->OnError=onError;e->User=user;e->SeenCapture=false;e->FHead=e->FTail=e->FCount=0;
 e->In=0;e->Out=0;e->Calls=0;e->Under=0;e->Over=0;e->Muted=0;e->Error=0;
 memset(e->OutL,0,sizeof(e->OutL));memset(e->OutR,0,sizeof(e->OutR));
 mbox_fifo_init(&e->Tx,e->TxData,8192);mbox_fifo_init(&e->Rx,e->RxData,8192);mbox_clock_init(&e->Clock,rate);
 ResetEvent(e->Stop);ResetEvent(e->Ready);
 e->Thread=(HANDLE)_beginthreadex(nullptr,0,worker,e,0,&e->ThreadId);
 if(!e->Thread){nimStop(e,true);return false;}
 if(WaitForSingleObject(e->Ready,5000)!=WAIT_OBJECT_0||e->Error){nimStop(e,true);return false;}
 return true;
}
void nimStats(NimEngine *e,NimDiagnostics *d) {
 *d={};d->Version=1;if(!e)return;
 d->Running=e->Running.load();d->Error=e->Error.load();d->Rate=e->Rate;
 d->InputFrames=e->In.load();d->OutputFrames=e->Out.load();d->Callbacks=e->Calls.load();
 d->Underruns=e->Under.load();d->Overflows=e->Over.load();d->MutedFrames=e->Muted.load();
}
