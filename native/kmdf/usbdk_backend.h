/* NimBox user-mode backend for the official, separately installed UsbDk.
 * Requires the scheduler's SLOT/BACKEND types and Windows helper functions.
 * Does not install drivers, set hider rules or change boot configuration. */
#include <cfgmgr32.h>
#include <usb.h>
#include "../third_party/usbdk/UsbDkHelper.h"
typedef struct {
 HMODULE Library;HANDLE Device,System;
 BOOL (*GetList)(PUSB_DK_DEVICE_INFO*,PULONG);
 void (*FreeList)(PUSB_DK_DEVICE_INFO);
 BOOL (*GetConfig)(PUSB_DK_CONFIG_DESCRIPTOR_REQUEST,PUSB_CONFIGURATION_DESCRIPTOR*,PULONG);
 void (*FreeConfig)(PUSB_CONFIGURATION_DESCRIPTOR);
 HANDLE (*Start)(PUSB_DK_DEVICE_ID);
 BOOL (*Stop)(HANDLE);
 HANDLE (*GetSystem)(HANDLE);
 BOOL (*SetAlt)(HANDLE,ULONG64,ULONG64);
 TransferResult (*Read)(HANDLE,PUSB_DK_TRANSFER_REQUEST,LPOVERLAPPED);
 TransferResult (*Write)(HANDLE,PUSB_DK_TRANSFER_REQUEST,LPOVERLAPPED);
 USB_DK_TRANSFER_REQUEST Requests[SLOT_COUNT];
 ULONG64 Lengths[SLOT_COUNT][MBOX_PACKET_COUNT];
 USB_DK_ISO_TRANSFER_RESULT Results[SLOT_COUNT][MBOX_PACKET_COUNT];
} UDK;
static void udk_close(UDK *u) {
 if(u->Device&&u->Device!=INVALID_HANDLE_VALUE) {
  if(!u->Stop(u->Device))printf("UsbDk restore returned Windows error %lu; check device state.\n",GetLastError());
  u->Device=INVALID_HANDLE_VALUE;
 }
 if(u->Library){FreeLibrary(u->Library);u->Library=NULL;}
}
static int udk_load(UDK *u,const char *file) {
 wchar_t path[1024];
 memset(u,0,sizeof(*u));u->Device=INVALID_HANDLE_VALUE;
 if(!MultiByteToWideChar(CP_ACP,0,file,-1,path,1024)||wcslen(path)<3||path[1]!=L':'||path[2]!=L'\\') {
  puts("Provide an absolute path to the verified x64 UsbDkHelper.dll.");return 0;
 }
 u->Library=LoadLibraryExW(path,NULL,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
 if(!u->Library){printf("Cannot load UsbDk helper, Windows error %lu\n",GetLastError());return 0;}
#define UDK_LOAD(member,name) do { FARPROC p=GetProcAddress(u->Library,name); if(!p){udk_close(u);return 0;} memcpy(&u->member,&p,sizeof(p)); } while(0)
 UDK_LOAD(GetList,"UsbDk_GetDevicesList");UDK_LOAD(FreeList,"UsbDk_ReleaseDevicesList");
 UDK_LOAD(GetConfig,"UsbDk_GetConfigurationDescriptor");UDK_LOAD(FreeConfig,"UsbDk_ReleaseConfigurationDescriptor");
 UDK_LOAD(Start,"UsbDk_StartRedirect");UDK_LOAD(Stop,"UsbDk_StopRedirect");
 UDK_LOAD(GetSystem,"UsbDk_GetRedirectorSystemHandle");UDK_LOAD(SetAlt,"UsbDk_SetAltsetting");
 UDK_LOAD(Read,"UsbDk_ReadPipe");UDK_LOAD(Write,"UsbDk_WritePipe");
#undef UDK_LOAD
 return 1;
}
static int udk_find(UDK *u,int redirect) {
 PUSB_DK_DEVICE_INFO list=NULL;ULONG count=0,index=0,matches=0;
 if(!u->GetList(&list,&count)){printf("UsbDk enumeration unavailable: Windows error %lu\n",GetLastError());return 0;}
 for(ULONG i=0;i<count;i++)if(list[i].DeviceDescriptor.idVendor==0x0dba&&list[i].DeviceDescriptor.idProduct==0x1000) {
  index=i;matches++;
  printf("Original Mbox found: revision %04x, speed %llu\n",list[i].DeviceDescriptor.bcdDevice,(unsigned long long)list[i].Speed);
 }
 if(matches!=1){printf("Expected exactly one target; found %lu. No device redirected.\n",matches);u->FreeList(list);return 0;}
 if(!redirect){u->FreeList(list);puts("Enumeration only: no device detached or configured.");return 1;}
 USB_DK_CONFIG_DESCRIPTOR_REQUEST request={0};PUSB_CONFIGURATION_DESCRIPTOR descriptor=NULL;ULONG length=0;
 const unsigned char expected[]={0x09,0x02,0x36,0x00,0x02,0x01,0x00,0x80,0xf0,0x09,0x04,0x00,0x00,0x00,0xff,0x00,0x00,0x00,
  0x09,0x04,0x01,0x00,0x00,0xff,0x00,0x00,0x00,0x09,0x04,0x01,0x01,0x02,0xff,0x00,0x00,0x00,
  0x09,0x05,0x81,0x05,0x30,0x01,0x01,0x00,0x00,0x09,0x05,0x02,0x05,0x30,0x01,0x01,0x00,0x00};
 request.ID=list[index].ID;
 int valid=list[index].Speed==FullSpeed&&u->GetConfig(&request,&descriptor,&length);
 if(valid)valid=descriptor&&length==sizeof(expected)&&memcmp(descriptor,expected,sizeof(expected))==0;
 if(descriptor)u->FreeConfig(descriptor);
 if(valid)u->Device=u->Start(&list[index].ID);
 u->FreeList(list);
 if(!valid||!u->Device||u->Device==INVALID_HANDLE_VALUE){puts("Identity/configuration or redirection failed. No transfers submitted.");return 0;}
 u->System=u->GetSystem(u->Device);
 if(!u->System||u->System==INVALID_HANDLE_VALUE||!u->SetAlt(u->Device,1,1))return 0;
 return 1;
}
static int udk_control(UDK *u,unsigned char endpoint,ULONG rate,int input) {
 UCHAR data[11]={0};USB_DK_TRANSFER_REQUEST request={0};OVERLAPPED ov={0};DWORD bytes;
 data[0]=(UCHAR)(input?0xa2:0x22);data[1]=(UCHAR)(input?0x81:0x01);
 data[3]=1;data[4]=endpoint;data[6]=3;
 if(!input){data[8]=(UCHAR)rate;data[9]=(UCHAR)(rate>>8);data[10]=(UCHAR)(rate>>16);}
 request.Buffer=data;request.BufferLength=sizeof(data);request.TransferType=ControlTransferType;
 ov.hEvent=CreateEventW(NULL,TRUE,FALSE,NULL);if(!ov.hEvent)return 0;
 TransferResult result=input?u->Read(u->Device,&request,&ov):u->Write(u->Device,&request,&ov);
 int ok=result!=TransferFailure;
 if(ok) {
  if(result==TransferSuccessAsync&&WaitForSingleObject(ov.hEvent,5000)!=WAIT_OBJECT_0)CancelIoEx(u->System,&ov);
  ok=GetOverlappedResult(u->System,&ov,&bytes,TRUE)!=FALSE;
 }
 CloseHandle(ov.hEvent);
 if(ok)ok=request.Result.GenResult.UsbdStatus==0&&request.Result.GenResult.BytesTransferred==3;
 if(ok&&input)ok=((ULONG)data[8]|((ULONG)data[9]<<8)|((ULONG)data[10]<<16))==rate;
 printf("UsbDk %s rate EP %02x: %s\n",input?"GET":"SET",endpoint,ok?"OK":"FAILED");
 return ok;
}
static int udk_rate(UDK *u,ULONG rate) {
 if(rate!=44100&&rate!=48000)return 0;
 return udk_control(u,0x02,rate,0)&&udk_control(u,0x02,rate,1)&&
        udk_control(u,0x81,rate,0)&&udk_control(u,0x81,rate,1);
}
static int udk_submit(BACKEND *b,SLOT *s) {
 UDK *u=(UDK*)b->Context;USB_DK_TRANSFER_REQUEST *r=&u->Requests[s->Index];
 HANDLE event=s->Ov.hEvent;ULONG bytes=0;
 memset(&s->Ov,0,sizeof(s->Ov));s->Ov.hEvent=event;ResetEvent(event);
 memset(r,0,sizeof(*r));memset(u->Results[s->Index],0,sizeof(u->Results[s->Index]));
 for(unsigned p=0;p<MBOX_PACKET_COUNT;p++) {
  u->Lengths[s->Index][p]=s->Direction?304:s->Input.Length[p];
  bytes+=(ULONG)u->Lengths[s->Index][p];
 }
 r->EndpointAddress=s->Direction?0x81:0x02;r->BufferLength=bytes;
 r->Buffer=s->Direction?s->Output.Data:s->Input.Data;r->TransferType=IsochronousTransferType;
 r->IsochronousPacketsArraySize=MBOX_PACKET_COUNT;r->IsochronousPacketsArray=u->Lengths[s->Index];
 r->Result.IsochronousResultsArray=u->Results[s->Index];
 TransferResult result=s->Direction?u->Read(u->Device,r,&s->Ov):u->Write(u->Device,r,&s->Ov);
 if(result==TransferSuccess)SetEvent(event);
 return result!=TransferFailure;
}
static int udk_finish(BACKEND *b,SLOT *s,DWORD *bytes) {
 UDK *u=(UDK*)b->Context;USB_DK_TRANSFER_REQUEST *r=&u->Requests[s->Index];DWORD returned;
 if(!GetOverlappedResult(u->System,&s->Ov,&returned,TRUE)){printf("UsbDk completion error %lu\n",GetLastError());return 0;}
 if(returned!=sizeof(USB_DK_GEN_TRANSFER_RESULT)||r->Result.GenResult.UsbdStatus>UINT32_MAX)return 0;
 s->Output.Version=MBOX_ABI_VERSION;s->Output.UsbStatus=(ULONG)r->Result.GenResult.UsbdStatus;
 ULONG offset=0;
 for(unsigned p=0;p<MBOX_PACKET_COUNT;p++) {
  USB_DK_ISO_TRANSFER_RESULT *item=&u->Results[s->Index][p];
  if(item->ActualLength>u->Lengths[s->Index][p]||item->TransferResult>UINT32_MAX)return 0;
  s->Output.Offset[p]=offset;s->Output.Length[p]=(ULONG)item->ActualLength;s->Output.Status[p]=(ULONG)item->TransferResult;
  if(item->TransferResult)s->Output.ErrorCount++;
  offset+=(ULONG)u->Lengths[s->Index][p];
 }
 *bytes=sizeof(s->Output);return 1;
}
static void udk_cancel(BACKEND *b,SLOT *s){UDK *u=(UDK*)b->Context;CancelIoEx(u->System,&s->Ov);}

