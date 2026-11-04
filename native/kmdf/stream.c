/* Silent pipeline diagnostic and deterministic backend tests.
 * No ASIO/WASAPI registration, recordings, driver install or security changes. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winioctl.h>
#include <setupapi.h>
#include <initguid.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mbox_ioctl.h"
#include "../core/mbox_stream.h"
#include "../core/mbox_gate.h"
DEFINE_GUID(GUID_DEVINTERFACE_MBOX_STREAM,
 0x65ec12b8,0xe2a2,0x41b0,0x96,0x5e,0x78,0xcf,0x2a,0x39,0x83,0xc1);
enum { SLOT_COUNT=2*MBOX_PIPE_DEPTH };
typedef struct {
 unsigned Index,Direction;int Active;
 OVERLAPPED Ov;
 MBOX_TRANSFER_INPUT Input;
 MBOX_TRANSFER_OUTPUT Output;
} SLOT;
typedef struct {
 uint64_t Submitted[2],Completed[2],Frames[2];
 unsigned Pending,MaxPending,Cancelled,Failures;
 uint32_t Peak[2];
} STATS;
typedef struct BACKEND {
 void *Context;
 int (*Submit)(struct BACKEND *,SLOT *);
 int (*Wait)(struct BACKEND *,SLOT *); /* slot index, -2 timeout, -1 failure */
 int (*Finish)(struct BACKEND *,SLOT *,DWORD *);
 void (*Cancel)(struct BACKEND *,SLOT *);
 int (*Stop)(struct BACKEND *);
} BACKEND;

/* This scheduler is shared by the actual DeviceIoControl backend and simulator.
 * All slots stay alive until their completion has been drained, including after
 * submit/completion failure or cancellation. Only the control thread owns slots. */
static int run(BACKEND *backend,ULONG rate,unsigned seconds,STATS *stats) {
 SLOT *slots=(SLOT*)calloc(SLOT_COUNT,sizeof(SLOT));
 HANDLE events[SLOT_COUNT]={0}; /* owned by scheduler, never by a backend */
 mbox_packet_clock clock;uint64_t target=(uint64_t)seconds*125;
 int result=0;unsigned idle=0,eventCount=0;
 memset(stats,0,sizeof(*stats));
 if(!slots||!target||!mbox_clock_init(&clock,rate)){free(slots);return 1;}
 for(unsigned i=0;i<SLOT_COUNT;i++) {
  slots[i].Index=i;slots[i].Direction=i/MBOX_PIPE_DEPTH;
  events[i]=CreateEventW(NULL,TRUE,FALSE,NULL);
  slots[i].Ov.hEvent=events[i];
  if(!events[i]){result=1;goto drain;}
  eventCount++;
 }
 for(;;) {
  if(backend->Stop(backend)){result=2;break;}
  for(unsigned i=0;i<SLOT_COUNT;i++) {
   SLOT *s=&slots[i];unsigned dir=s->Direction;
   if(s->Active||stats->Submitted[dir]>=target)continue;
   memset(&s->Input,0,sizeof(s->Input));memset(&s->Output,0,sizeof(s->Output));
   s->Input.Version=MBOX_ABI_VERSION;s->Input.Direction=dir;
   if(!dir) {
    mbox_packet packets[MBOX_PACKET_COUNT];uint32_t bytes;
    if(!mbox_plan(&clock,packets,MBOX_PACKET_COUNT,MBOX_BUFFER_BYTES,&bytes)){result=1;goto drain;}
    for(unsigned p=0;p<MBOX_PACKET_COUNT;p++)s->Input.Length[p]=packets[p].length;
   }
   if(!backend->Submit(backend,s)){stats->Failures++;result=1;goto drain;}
   s->Active=1;stats->Submitted[dir]++;stats->Pending++;
   if(stats->Pending>stats->MaxPending)stats->MaxPending=stats->Pending;
  }
  if(!stats->Pending)break;
  int index=backend->Wait(backend,slots);
  if(index==-2) {
   if(++idle<50)continue;
   stats->Failures++;result=1;break;
  }
  idle=0;
  if(index<0||index>=SLOT_COUNT||!slots[index].Active){stats->Failures++;result=1;break;}
  SLOT *s=&slots[index];DWORD bytes=0;unsigned dir=s->Direction;
  int finished=backend->Finish(backend,s,&bytes);
  s->Active=0;stats->Pending--;
  if(!finished||bytes!=sizeof(s->Output)||s->Output.Version!=MBOX_ABI_VERSION||
     s->Output.UsbStatus||s->Output.ErrorCount){stats->Failures++;result=1;break;}
  for(unsigned p=0;p<MBOX_PACKET_COUNT;p++) {
   ULONG length=dir?s->Output.Length[p]:s->Input.Length[p];
   ULONG offset=s->Output.Offset[p];
   if(s->Output.Status[p]||
      (dir&&(offset!=p*304u||!mbox_capture_packet_valid(s->Output.Status[p],length)))) {
    stats->Failures++;result=1;goto drain;
   }
   stats->Frames[dir]+=length/6;
   if(dir)for(ULONG frame=0;frame<length/6;frame++) {
    int32_t l,r;uint32_t peaks[2];
    mbox_unpack_frame(&s->Output.Data[offset+frame*6],&l,&r);
    peaks[0]=l<0?0u-(uint32_t)l:(uint32_t)l;
    peaks[1]=r<0?0u-(uint32_t)r:(uint32_t)r;
    for(unsigned c=0;c<2;c++)if(peaks[c]>stats->Peak[c])stats->Peak[c]=peaks[c];
   }
  }
  stats->Completed[dir]++;
 }
drain:
 /* Cancel everything first so draining one endpoint cannot delay cancelling
  * the other. Finish must wait for ownership to return, even on cancellation. */
 for(unsigned i=0;i<SLOT_COUNT;i++)if(slots[i].Active)backend->Cancel(backend,&slots[i]);
 for(unsigned i=0;i<SLOT_COUNT;i++) {
  if(slots[i].Active) {
   DWORD bytes;backend->Finish(backend,&slots[i],&bytes);
   slots[i].Active=0;stats->Pending--;stats->Cancelled++;
  }
 }
 while(eventCount)CloseHandle(events[--eventCount]);
 free(slots);return result;
}

typedef struct {
 uint32_t Gate,Random;unsigned Submitted,Finished,Cancelled,CancelAt,FailSubmit,FailComplete,BadLength,ShortReply,UsbError,BadOffset,Stall;
 unsigned Live[SLOT_COUNT];mbox_packet_clock Clock[2];
} MOCK;
static int mock_submit(BACKEND *b,SLOT *s) {
 MOCK *m=(MOCK*)b->Context;uint32_t next,bytes;mbox_packet packets[MBOX_PACKET_COUNT];
 m->Submitted++;
 if(m->Submitted==m->FailSubmit)return 0;
 if(m->Live[s->Index]||!mbox_gate_acquire(m->Gate,s->Direction,&next))return 0;
 if(!mbox_plan(&m->Clock[s->Direction],packets,MBOX_PACKET_COUNT,MBOX_BUFFER_BYTES,&bytes))return 0;
 s->Output.Version=MBOX_ABI_VERSION;
 for(unsigned p=0;p<MBOX_PACKET_COUNT;p++) {
  ULONG length=packets[p].length;
  if(!s->Direction) {
   if(s->Input.Length[p]!=length)return 0;
  } else {
   s->Output.Offset[p]=p*304;s->Output.Length[p]=length;
   for(ULONG frame=0;frame<length/6;frame++)
    mbox_pack_frame(&s->Output.Data[p*304+frame*6],0x12345600,-0x23456700);
  }
 }
 if(!s->Direction)for(unsigned i=0;i<bytes;i++)if(s->Input.Data[i])return 0;
 m->Gate=next;m->Live[s->Index]=1;return 1;
}
static int mock_wait(BACKEND *b,SLOT *slots) {
 MOCK *m=(MOCK*)b->Context;
 if(m->Stall)return -2;
 /* Deliberately vary completion order to exercise buffer ownership/reuse. */
 m->Random=m->Random*1664525u+1013904223u;
 unsigned first=(m->Random>>16)%SLOT_COUNT;
 for(unsigned n=0;n<SLOT_COUNT;n++) {
  unsigned i=(first+n)%SLOT_COUNT;if(slots[i].Active)return (int)i;
 }
 return -1;
}
static int mock_finish(BACKEND *b,SLOT *s,DWORD *bytes) {
 MOCK *m=(MOCK*)b->Context;uint32_t next;
 if(!m->Live[s->Index]||!mbox_gate_release(m->Gate,s->Direction,&next))return 0;
 m->Gate=next;unsigned cancelled=m->Live[s->Index]==2;m->Live[s->Index]=0;m->Finished++;
 *bytes=sizeof(s->Output);
 if(cancelled){m->Cancelled++;return 0;}
 if(m->Finished==m->BadLength)s->Output.Length[0]=305;
 if(m->Finished==m->BadOffset)s->Output.Offset[0]=1;
 if(m->Finished==m->UsbError)s->Output.UsbStatus=0xc0000001u;
 if(m->Finished==m->ShortReply)*bytes=sizeof(s->Output)-1;
 return m->Finished!=m->FailComplete;
}
static void mock_cancel(BACKEND *b,SLOT *s) {
 MOCK *m=(MOCK*)b->Context;if(m->Live[s->Index])m->Live[s->Index]=2;
}
static int mock_stop(BACKEND *b) {
 MOCK *m=(MOCK*)b->Context;return m->CancelAt&&m->Finished>=m->CancelAt;
}
static int self_test(void) {
 for(unsigned rate=44100;rate<=48000;rate+=3900)for(unsigned fault=0;fault<9;fault++) {
  MOCK m={0};STATS stats;BACKEND backend={&m,mock_submit,mock_wait,mock_finish,mock_cancel,mock_stop};
  mbox_clock_init(&m.Clock[0],rate);mbox_clock_init(&m.Clock[1],rate);m.Random=123;
  if(fault==1)m.FailSubmit=3;
  if(fault==2)m.FailComplete=17;
  if(fault==3)m.CancelAt=20;
  /* Seed0 makes the initial synthetic completion a capture slot. */
  if(fault==4){m.BadLength=1;m.Random=0;}
  if(fault==5)m.ShortReply=17;
  if(fault==6)m.UsbError=17;
  if(fault==7){m.BadOffset=1;m.Random=0;}
  if(fault==8)m.Stall=1;
  int result=run(&backend,rate,600,&stats);
  if(m.Gate||stats.Pending||stats.MaxPending>8||m.Cancelled!=stats.Cancelled)return 1;
  if(!fault) {
   if(result||stats.Completed[0]!=75000||stats.Completed[1]!=75000||
      stats.Frames[0]!=(uint64_t)rate*600||stats.Frames[1]!=(uint64_t)rate*600||
      stats.Peak[0]!=0x12345600u||stats.Peak[1]!=0x23456700u||stats.MaxPending!=8)return 1;
  } else if(result!=(fault==3?2:1))return 1;
  printf("PASS simulated %lu Hz case %u: result=%d, completed=%llu/%llu, drained=%u\n",
   (ULONG)rate,fault,result,(unsigned long long)stats.Completed[0],(unsigned long long)stats.Completed[1],stats.Cancelled);
 }
 puts("PASS: two ten-minute virtual duplex streams; eight failure/cancellation cases at each rate drained all owned requests.");
 return 0;
}

static volatile LONG stop_requested;
static BOOL WINAPI on_console(DWORD type) {
 if(type==CTRL_C_EVENT||type==CTRL_BREAK_EVENT){InterlockedExchange(&stop_requested,1);return TRUE;}
 return FALSE;
}
static int real_submit(BACKEND *b,SLOT *s) {
 HANDLE device=(HANDLE)b->Context;HANDLE event=s->Ov.hEvent;DWORD ignored;
 memset(&s->Ov,0,sizeof(s->Ov));s->Ov.hEvent=event;ResetEvent(event);
 if(DeviceIoControl(device,IOCTL_MBOX_TRANSFER,&s->Input,sizeof(s->Input),&s->Output,sizeof(s->Output),&ignored,&s->Ov))return 1;
 return GetLastError()==ERROR_IO_PENDING;
}
static int real_wait(BACKEND *b,SLOT *slots) {
 HANDLE events[SLOT_COUNT];unsigned map[SLOT_COUNT],count=0;UNREFERENCED_PARAMETER(b);
 for(unsigned i=0;i<SLOT_COUNT;i++)if(slots[i].Active){events[count]=slots[i].Ov.hEvent;map[count++]=i;}
 DWORD result=WaitForMultipleObjects(count,events,FALSE,100);
 if(result==WAIT_TIMEOUT)return -2;
 return result<WAIT_OBJECT_0+count?(int)map[result-WAIT_OBJECT_0]:-1;
}
static int real_finish(BACKEND *b,SLOT *s,DWORD *bytes) {
 if(GetOverlappedResult((HANDLE)b->Context,&s->Ov,bytes,TRUE))return 1;
 printf("Transfer returned Windows error %lu\n",GetLastError());return 0;
}
static void real_cancel(BACKEND *b,SLOT *s){CancelIoEx((HANDLE)b->Context,&s->Ov);}
static int real_stop(BACKEND *b){UNREFERENCED_PARAMETER(b);return InterlockedCompareExchange(&stop_requested,0,0)!=0;}
static HANDLE open_device(void) {
 HDEVINFO set=SetupDiGetClassDevsW(&GUID_DEVINTERFACE_MBOX_STREAM,NULL,NULL,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
 SP_DEVICE_INTERFACE_DATA iface={0};PSP_DEVICE_INTERFACE_DETAIL_DATA_W detail=NULL;
 DWORD needed=0;HANDLE device=INVALID_HANDLE_VALUE;
 if(set==INVALID_HANDLE_VALUE)return device;
 iface.cbSize=sizeof(iface);
 if(SetupDiEnumDeviceInterfaces(set,NULL,&GUID_DEVINTERFACE_MBOX_STREAM,0,&iface)) {
  SetupDiGetDeviceInterfaceDetailW(set,&iface,NULL,0,&needed,NULL);
  detail=(PSP_DEVICE_INTERFACE_DETAIL_DATA_W)calloc(1,needed);
  if(detail) {
   detail->cbSize=sizeof(*detail);
   if(SetupDiGetDeviceInterfaceDetailW(set,&iface,detail,needed,NULL,NULL))
    device=CreateFileW(detail->DevicePath,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,NULL);
  }
 }
 free(detail);SetupDiDestroyDeviceInfoList(set);return device;
}
int main(int argc,char **argv) {
 if(argc==2&&strcmp(argv[1],"--self-test")==0)return self_test();
 if(argc!=4||strcmp(argv[1],"silent")!=0||
    (strcmp(argv[2],"44100")&&strcmp(argv[2],"48000"))) {
  puts("Usage: mbox_stream.exe --self-test | silent 44100|48000 seconds(1..60)");return 2;
 }
 char *end;unsigned long duration=strtoul(argv[3],&end,10);
 if(!*argv[3]||*end||duration<1||duration>60)return 2;
 ULONG rate=strcmp(argv[2],"44100")==0?44100:48000;
 HANDLE device=open_device();
 if(device==INVALID_HANDLE_VALUE){puts("Custom research transport unavailable. No hardware operation performed.");return 1;}
 OVERLAPPED ov={0};DWORD bytes;int result=1;
 ov.hEvent=CreateEventW(NULL,TRUE,FALSE,NULL);
 if(!ov.hEvent){CloseHandle(device);return 1;}
 BOOL submitted=DeviceIoControl(device,IOCTL_MBOX_SET_RATE,&rate,sizeof(rate),NULL,0,&bytes,&ov);
 if(submitted||GetLastError()==ERROR_IO_PENDING) {
  if(GetOverlappedResult(device,&ov,&bytes,TRUE)) {
   BACKEND backend={device,real_submit,real_wait,real_finish,real_cancel,real_stop};STATS stats;
   SetConsoleCtrlHandler(on_console,TRUE);
   result=run(&backend,rate,(unsigned)duration,&stats);
   printf("result=%d completed OUT/IN=%llu/%llu frames=%llu/%llu peak=%lu/%lu errors=%u cancelled=%u\n",
    result,(unsigned long long)stats.Completed[0],(unsigned long long)stats.Completed[1],
    (unsigned long long)stats.Frames[0],(unsigned long long)stats.Frames[1],stats.Peak[0],stats.Peak[1],stats.Failures,stats.Cancelled);
   SetConsoleCtrlHandler(on_console,FALSE);
  }
 }
 CloseHandle(ov.hEvent);CloseHandle(device);return result;
}
