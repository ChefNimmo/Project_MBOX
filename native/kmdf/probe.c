/* Diagnostic controller. Default action only lists our custom interface.
 * 'probe 44100' or 'probe 48000' sets rate and sends 8ms of silence while
 * capturing 8 USB frames. No tone, recordings, driver installation or fallback
 * to the proprietary driver. Buildable before the custom driver is signed. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winioctl.h>
#include <setupapi.h>
#include <initguid.h>
#include <stdio.h>
#include <stdlib.h>
#include "mbox_ioctl.h"
#include "../core/mbox_stream.h"
DEFINE_GUID(GUID_DEVINTERFACE_MBOX_RESEARCH,
 0x65ec12b8,0xe2a2,0x41b0,0x96,0x5e,0x78,0xcf,0x2a,0x39,0x83,0xc1);

static HANDLE open_mbox(BOOL probe) {
 HDEVINFO set=SetupDiGetClassDevsW(&GUID_DEVINTERFACE_MBOX_RESEARCH,NULL,NULL,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
 SP_DEVICE_INTERFACE_DATA iface={0};DWORD needed=0;
 PSP_DEVICE_INTERFACE_DETAIL_DATA_W detail;HANDLE device=INVALID_HANDLE_VALUE;
 if(set==INVALID_HANDLE_VALUE)return device;
 iface.cbSize=sizeof(iface);
 if(!SetupDiEnumDeviceInterfaces(set,NULL,&GUID_DEVINTERFACE_MBOX_RESEARCH,0,&iface)) {
  puts("No independent Mbox research transport is currently installed/present.");
  SetupDiDestroyDeviceInfoList(set);return device;
 }
 SetupDiGetDeviceInterfaceDetailW(set,&iface,NULL,0,&needed,NULL);
 detail=(PSP_DEVICE_INTERFACE_DETAIL_DATA_W)calloc(1,needed);
 if(detail) {
  detail->cbSize=sizeof(*detail);
  if(SetupDiGetDeviceInterfaceDetailW(set,&iface,detail,needed,NULL,NULL)) {
   puts("Independent Mbox research interface found.");
   if(probe)device=CreateFileW(detail->DevicePath,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,NULL);
  }
  free(detail);
 }
 SetupDiDestroyDeviceInfoList(set);return device;
}
static BOOL submit(HANDLE device,DWORD code,void *input,DWORD inBytes,
                   void *output,DWORD outBytes,OVERLAPPED *ov) {
 DWORD ignored;
 if(DeviceIoControl(device,code,input,inBytes,output,outBytes,&ignored,ov))return TRUE;
 if(GetLastError()==ERROR_IO_PENDING)return TRUE;
 printf("DeviceIoControl failed: %lu\n",GetLastError());return FALSE;
}
static BOOL finish(HANDLE device,OVERLAPPED *ov,DWORD *bytes) {
 if(WaitForSingleObject(ov->hEvent,7000)!=WAIT_OBJECT_0) {
  puts("Transfer timed out; requesting cancellation.");CancelIoEx(device,ov);
 }
 /* Drain completion before freeing the caller's overlapped storage/buffers. */
 if(!GetOverlappedResult(device,ov,bytes,TRUE)) {
  printf("Request completed with Windows error %lu\n",GetLastError());return FALSE;
 }
 return TRUE;
}
int main(int argc,char **argv) {
 BOOL probe=(argc==3 && strcmp(argv[1],"probe")==0);
 ULONG rate=0;HANDLE device;OVERLAPPED ov[2]={{0}};
 MBOX_TRANSFER_INPUT input[2]={{0}};
 MBOX_TRANSFER_OUTPUT output[2]={{0}};
 mbox_packet_clock clock;mbox_packet plan[MBOX_PACKET_COUNT];uint32_t used;
 DWORD bytes=0;BOOL sent[2]={FALSE,FALSE},ok=TRUE;
 if(argc!=1&&!probe){puts("Usage: mbox_probe.exe [probe 44100|48000]");return 2;}
 if(probe) {
  if(strcmp(argv[2],"44100")==0)rate=44100;
  else if(strcmp(argv[2],"48000")==0)rate=48000;
  else {puts("Unsupported sample rate.");return 2;}
 }
 device=open_mbox(probe);
 if(!probe)return 0;
 if(device==INVALID_HANDLE_VALUE){printf("Cannot open transport (administrator access required): %lu\n",GetLastError());return 1;}
 for(unsigned i=0;i<2;i++) {
  ov[i].hEvent=CreateEventW(NULL,TRUE,FALSE,NULL);
  if(!ov[i].hEvent){ok=FALSE;goto cleanup;}
 }
 if(!submit(device,IOCTL_MBOX_SET_RATE,&rate,sizeof(rate),NULL,0,&ov[0])||!finish(device,&ov[0],&bytes)) {
  ok=FALSE;goto cleanup;
 }
 ResetEvent(ov[0].hEvent);
 if(!mbox_clock_init(&clock,rate)||!mbox_plan(&clock,plan,MBOX_PACKET_COUNT,MBOX_BUFFER_BYTES,&used)) {
  ok=FALSE;goto cleanup;
 }
 for(unsigned i=0;i<2;i++) {
  input[i].Version=MBOX_ABI_VERSION;input[i].Direction=i;
  for(unsigned p=0;p<MBOX_PACKET_COUNT;p++)input[i].Length[p]=plan[p].length;
  sent[i]=submit(device,IOCTL_MBOX_TRANSFER,&input[i],sizeof(input[i]),&output[i],sizeof(output[i]),&ov[i]);
  if(!sent[i])ok=FALSE;
 }
 for(unsigned i=0;i<2;i++)if(sent[i]) {
  if(!finish(device,&ov[i],&bytes)){ok=FALSE;continue;}
  if(bytes!=sizeof(output[i])||output[i].Version!=MBOX_ABI_VERSION){ok=FALSE;continue;}
  printf("%s USB status=0x%08lx errors=%lu start-frame=%lu\n",i?"IN":"OUT",output[i].UsbStatus,output[i].ErrorCount,output[i].StartFrame);
  if(output[i].UsbStatus||output[i].ErrorCount)ok=FALSE;
  for(unsigned p=0;p<MBOX_PACKET_COUNT;p++) {
   printf("  packet %u offset=%lu length=%lu status=0x%08lx\n",p,output[i].Offset[p],output[i].Length[p],output[i].Status[p]);
   if(output[i].Status[p])ok=FALSE;
   if(i&&!mbox_capture_packet_valid(output[i].Status[p],output[i].Length[p]))ok=FALSE;
  }
 }
cleanup:
 for(unsigned i=0;i<2;i++)if(ov[i].hEvent)CloseHandle(ov[i].hEvent);
 CloseHandle(device);
 return ok?0:1;
}
