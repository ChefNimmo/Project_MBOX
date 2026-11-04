/* Independent experimental transport. Not yet hardware validated or installable.
 * No firmware download, license modification, or Windows audio endpoint. */
#include <ntddk.h>
#include <wdf.h>
#include <usb.h>
#include <usbdlib.h>
#include <wdfusb.h>
#include <initguid.h>
#include "mbox_ioctl.h"

DEFINE_GUID(GUID_DEVINTERFACE_MBOX_RESEARCH,
 0x65ec12b8,0xe2a2,0x41b0,0x96,0x5e,0x78,0xcf,0x2a,0x39,0x83,0xc1);
typedef struct {
 WDFUSBDEVICE Usb;
 WDFUSBPIPE In,Out;
 volatile LONG Busy; /* bit0 OUT, bit1 IN, bit2 control; CAS serialises rate changes */
 ULONG Rate;
} DEVICE_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT,DeviceContext);
typedef struct {
 DEVICE_CONTEXT *Device;
 PURB Urb;
 PUCHAR Data;
 MBOX_TRANSFER_OUTPUT *Output;
 ULONG Direction,Bytes;
 LONG Bit;
} REQUEST_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(REQUEST_CONTEXT,RequestContext);

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD MboxAdd;
EVT_WDF_DEVICE_PREPARE_HARDWARE MboxPrepare;
EVT_WDF_DEVICE_RELEASE_HARDWARE MboxRelease;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL MboxControl;
EVT_WDF_REQUEST_COMPLETION_ROUTINE MboxTransferComplete;

NTSTATUS DriverEntry(PDRIVER_OBJECT object,PUNICODE_STRING path) {
 WDF_DRIVER_CONFIG config;
 WDF_DRIVER_CONFIG_INIT(&config,MboxAdd);
 return WdfDriverCreate(object,path,WDF_NO_OBJECT_ATTRIBUTES,&config,WDF_NO_HANDLE);
}
NTSTATUS MboxAdd(WDFDRIVER driver,PWDFDEVICE_INIT init) {
 WDF_PNPPOWER_EVENT_CALLBACKS pnp;
 WDF_OBJECT_ATTRIBUTES attrs;
 WDF_IO_QUEUE_CONFIG queue;
 WDFDEVICE device;
 NTSTATUS status;
 DECLARE_CONST_UNICODE_STRING(sddl,L"D:P(A;;GA;;;SY)(A;;GA;;;BA)");
 UNREFERENCED_PARAMETER(driver);
 WdfDeviceInitSetDeviceType(init,FILE_DEVICE_UNKNOWN);
 WdfDeviceInitSetExclusive(init,TRUE);
 status=WdfDeviceInitAssignSDDLString(init,&sddl);
 if(!NT_SUCCESS(status))return status;
 WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnp);
 pnp.EvtDevicePrepareHardware=MboxPrepare;pnp.EvtDeviceReleaseHardware=MboxRelease;
 WdfDeviceInitSetPnpPowerEventCallbacks(init,&pnp);
 WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attrs,REQUEST_CONTEXT);
 WdfDeviceInitSetRequestAttributes(init,&attrs);
 WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attrs,DEVICE_CONTEXT);
 attrs.ExecutionLevel=WdfExecutionLevelPassive;
 status=WdfDeviceCreate(&init,&attrs,&device);
 if(!NT_SUCCESS(status))return status;
 status=WdfDeviceCreateDeviceInterface(device,&GUID_DEVINTERFACE_MBOX_RESEARCH,NULL);
 if(!NT_SUCCESS(status))return status;
 WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queue,WdfIoQueueDispatchParallel);
 queue.EvtIoDeviceControl=MboxControl;
 /* Sent requests are owned/cancelled by the USB I/O target; unsent requests
  * are completed in this callback. No driver-owned pending request list. */
 return WdfIoQueueCreate(device,&queue,WDF_NO_OBJECT_ATTRIBUTES,WDF_NO_HANDLE);
}
NTSTATUS MboxPrepare(WDFDEVICE device,WDFCMRESLIST raw,WDFCMRESLIST translated) {
 DEVICE_CONTEXT *ctx=DeviceContext(device);
 WDF_USB_DEVICE_CREATE_CONFIG create;
 WDF_USB_DEVICE_SELECT_CONFIG_PARAMS config;
 WDF_USB_INTERFACE_SETTING_PAIR pairs[2];
 USB_DEVICE_DESCRIPTOR descriptor;
 NTSTATUS status;
 UNREFERENCED_PARAMETER(raw);UNREFERENCED_PARAMETER(translated);
 if(ctx->Usb==NULL) {
  WDF_USB_DEVICE_CREATE_CONFIG_INIT(&create,USBD_CLIENT_CONTRACT_VERSION_602);
  status=WdfUsbTargetDeviceCreateWithParameters(device,&create,WDF_NO_OBJECT_ATTRIBUTES,&ctx->Usb);
  if(!NT_SUCCESS(status))return status;
 }
 WdfUsbTargetDeviceGetDeviceDescriptor(ctx->Usb,&descriptor);
 if(descriptor.idVendor!=0x0dba||descriptor.idProduct!=0x1000||descriptor.bcdUSB!=0x0110)
  return STATUS_DEVICE_CONFIGURATION_ERROR;
 if(WdfUsbTargetDeviceGetNumInterfaces(ctx->Usb)!=2)return STATUS_DEVICE_CONFIGURATION_ERROR;
 pairs[0].UsbInterface=WdfUsbTargetDeviceGetInterface(ctx->Usb,0);pairs[0].SettingIndex=0;
 pairs[1].UsbInterface=WdfUsbTargetDeviceGetInterface(ctx->Usb,1);pairs[1].SettingIndex=1;
 if(WdfUsbInterfaceGetInterfaceNumber(pairs[0].UsbInterface)!=0||
    WdfUsbInterfaceGetInterfaceNumber(pairs[1].UsbInterface)!=1)
  return STATUS_DEVICE_CONFIGURATION_ERROR;
 WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_MULTIPLE_INTERFACES(&config,2,pairs);
 status=WdfUsbTargetDeviceSelectConfig(ctx->Usb,WDF_NO_OBJECT_ATTRIBUTES,&config);
 if(!NT_SUCCESS(status))return status;
 ctx->In=NULL;ctx->Out=NULL;ctx->Rate=0;
 if(WdfUsbInterfaceGetNumConfiguredPipes(pairs[1].UsbInterface)!=2)
  return STATUS_DEVICE_CONFIGURATION_ERROR;
 for(UCHAR i=0;i<2;i++) {
  WDF_USB_PIPE_INFORMATION info;
  WDFUSBPIPE pipe;
  WDF_USB_PIPE_INFORMATION_INIT(&info);
  pipe=WdfUsbInterfaceGetConfiguredPipe(pairs[1].UsbInterface,i,&info);
  if(info.PipeType!=WdfUsbPipeTypeIsochronous||info.MaximumPacketSize!=304||info.Interval!=1)
   return STATUS_DEVICE_CONFIGURATION_ERROR;
  if(info.EndpointAddress==0x81)ctx->In=pipe;
  else if(info.EndpointAddress==0x02)ctx->Out=pipe;
  else return STATUS_DEVICE_CONFIGURATION_ERROR;
 }
 return ctx->In&&ctx->Out?STATUS_SUCCESS:STATUS_DEVICE_CONFIGURATION_ERROR;
}
NTSTATUS MboxRelease(WDFDEVICE device,WDFCMRESLIST translated) {
 DEVICE_CONTEXT *ctx=DeviceContext(device);
 UNREFERENCED_PARAMETER(translated);
 /* Framework stops USB targets before ReleaseHardware. */
 ctx->In=NULL;ctx->Out=NULL;ctx->Rate=0;
 return STATUS_SUCCESS;
}
/* Endpoint class sampling-frequency control, independently recovered from
 * original driver and corroborated by ALSA. Readback required on both pipes. */
static NTSTATUS MboxRateEndpoint(DEVICE_CONTEXT *ctx,UCHAR endpoint,ULONG rate) {
 WDF_USB_CONTROL_SETUP_PACKET setup;
 WDF_MEMORY_DESCRIPTOR memory;
 WDF_REQUEST_SEND_OPTIONS options;
 UCHAR bytes[3];ULONG transferred;NTSTATUS status;
 bytes[0]=(UCHAR)rate;bytes[1]=(UCHAR)(rate>>8);bytes[2]=(UCHAR)(rate>>16);
 WDF_REQUEST_SEND_OPTIONS_INIT(&options,WDF_REQUEST_SEND_OPTION_TIMEOUT);
 WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&options,WDF_REL_TIMEOUT_IN_SEC(5));
 WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memory,bytes,sizeof(bytes));
 WDF_USB_CONTROL_SETUP_PACKET_INIT_CLASS(&setup,BmRequestHostToDevice,BmRequestToEndpoint,
  0x01,0x0100,endpoint);
 status=WdfUsbTargetDeviceSendControlTransferSynchronously(ctx->Usb,NULL,&options,&setup,&memory,&transferred);
 if(!NT_SUCCESS(status))return status;
 if(transferred!=3)return STATUS_DEVICE_DATA_ERROR;
 RtlZeroMemory(bytes,sizeof(bytes));
 WDF_USB_CONTROL_SETUP_PACKET_INIT_CLASS(&setup,BmRequestDeviceToHost,BmRequestToEndpoint,
  0x81,0x0100,endpoint);
 status=WdfUsbTargetDeviceSendControlTransferSynchronously(ctx->Usb,NULL,&options,&setup,&memory,&transferred);
 if(!NT_SUCCESS(status))return status;
 if(transferred!=3||((ULONG)bytes[0]|((ULONG)bytes[1]<<8)|((ULONG)bytes[2]<<16))!=rate)
  return STATUS_DEVICE_DATA_ERROR;
 return STATUS_SUCCESS;
}
static BOOLEAN MboxAcquire(DEVICE_CONTEXT *ctx,LONG bit) {
 LONG old=InterlockedCompareExchange(&ctx->Busy,0,0);
 for(;;) {
  LONG actual;
  if(old&(bit|4))return FALSE;
  actual=InterlockedCompareExchange(&ctx->Busy,old|bit,old);
  if(actual==old)return TRUE;
  old=actual;
 }
}
VOID MboxTransferComplete(WDFREQUEST request,WDFIOTARGET target,
 PWDF_REQUEST_COMPLETION_PARAMS params,WDFCONTEXT unused) {
 REQUEST_CONTEXT *r=RequestContext(request);
 MBOX_TRANSFER_OUTPUT *out=r->Output;
 struct _URB_ISOCH_TRANSFER *iso=&r->Urb->UrbIsochronousTransfer;
 NTSTATUS status=params->IoStatus.Status;
 UNREFERENCED_PARAMETER(target);UNREFERENCED_PARAMETER(unused);
 if(NT_SUCCESS(status)) {
  RtlZeroMemory(out,sizeof(*out));out->Version=MBOX_ABI_VERSION;
  out->UsbStatus=iso->Hdr.Status;out->ErrorCount=iso->ErrorCount;out->StartFrame=iso->StartFrame;
  for(ULONG i=0;i<MBOX_PACKET_COUNT;i++) {
   out->Offset[i]=iso->IsoPacket[i].Offset;out->Length[i]=iso->IsoPacket[i].Length;
   out->Status[i]=iso->IsoPacket[i].Status;
   /* For OUT, USB reports status; Length is meaningful only for IN. */
   if(r->Direction && (out->Offset[i]>r->Bytes || out->Length[i]>r->Bytes-out->Offset[i]))
    status=STATUS_DEVICE_DATA_ERROR;
  }
  if(r->Direction&&NT_SUCCESS(status))RtlCopyMemory(out->Data,r->Data,r->Bytes);
 }
 InterlockedAnd(&r->Device->Busy,~r->Bit);
 WdfRequestCompleteWithInformation(request,status,NT_SUCCESS(status)?sizeof(*out):0);
}
VOID MboxControl(WDFQUEUE queue,WDFREQUEST request,size_t outputLength,size_t inputLength,ULONG code) {
 DEVICE_CONTEXT *ctx=DeviceContext(WdfIoQueueGetDevice(queue));
 NTSTATUS status=STATUS_INVALID_DEVICE_REQUEST;
 UNREFERENCED_PARAMETER(outputLength);UNREFERENCED_PARAMETER(inputLength);
 if(code==IOCTL_MBOX_SET_RATE) {
  ULONG *rate,value;
  status=WdfRequestRetrieveInputBuffer(request,sizeof(*rate),(PVOID*)&rate,NULL);
  if(!NT_SUCCESS(status))goto done;
  value=*rate;
  if(value!=44100&&value!=48000){status=STATUS_INVALID_PARAMETER;goto done;}
  if(InterlockedCompareExchange(&ctx->Busy,4,0)!=0){status=STATUS_DEVICE_BUSY;goto done;}
  ctx->Rate=0;
  status=MboxRateEndpoint(ctx,0x02,value);
  if(NT_SUCCESS(status))status=MboxRateEndpoint(ctx,0x81,value);
  if(NT_SUCCESS(status))ctx->Rate=value;
  InterlockedExchange(&ctx->Busy,0);
 } else if(code==IOCTL_MBOX_TRANSFER) {
  MBOX_TRANSFER_INPUT *input;
  MBOX_TRANSFER_OUTPUT *output;
  REQUEST_CONTEXT *r=RequestContext(request);
  WDF_OBJECT_ATTRIBUTES attrs;
  WDFMEMORY urbMemory,dataMemory;
  WDFUSBPIPE pipe;
  WDF_REQUEST_SEND_OPTIONS options;
  ULONG bytes=0,offsets[MBOX_PACKET_COUNT];
  status=WdfRequestRetrieveInputBuffer(request,sizeof(*input),(PVOID*)&input,NULL);
  if(!NT_SUCCESS(status))goto done;
  status=WdfRequestRetrieveOutputBuffer(request,sizeof(*output),(PVOID*)&output,NULL);
  if(!NT_SUCCESS(status))goto done;
  if(input->Version!=MBOX_ABI_VERSION||input->Direction>1){status=STATUS_INVALID_PARAMETER;goto done;}
  for(ULONG i=0;i<MBOX_PACKET_COUNT;i++) {
   ULONG length=input->Direction?304:input->Length[i];
   if(length>304||(!input->Direction&&(length==0||length%6))){status=STATUS_INVALID_PARAMETER;goto done;}
   offsets[i]=bytes;bytes+=length;
  }
  r->Device=ctx;r->Direction=input->Direction;r->Bit=input->Direction?2:1;r->Bytes=bytes;r->Output=output;
  if(!MboxAcquire(ctx,r->Bit)){status=STATUS_DEVICE_BUSY;goto done;}
  if(!ctx->Rate||!ctx->In||!ctx->Out){status=STATUS_INVALID_DEVICE_STATE;goto release;}
  pipe=r->Direction?ctx->In:ctx->Out;
  WDF_OBJECT_ATTRIBUTES_INIT(&attrs);attrs.ParentObject=request;
  status=WdfMemoryCreate(&attrs,NonPagedPoolNx,'xbmM',bytes,&dataMemory,(PVOID*)&r->Data);
  if(!NT_SUCCESS(status))goto release;
  RtlZeroMemory(r->Data,bytes);
  if(!r->Direction)RtlCopyMemory(r->Data,input->Data,bytes);
  status=WdfUsbTargetDeviceCreateIsochUrb(ctx->Usb,&attrs,MBOX_PACKET_COUNT,&urbMemory,&r->Urb);
  if(!NT_SUCCESS(status))goto release;
  r->Urb->UrbIsochronousTransfer.Hdr.Length=(USHORT)GET_ISO_URB_SIZE(MBOX_PACKET_COUNT);
  r->Urb->UrbIsochronousTransfer.Hdr.Function=URB_FUNCTION_ISOCH_TRANSFER;
  r->Urb->UrbIsochronousTransfer.PipeHandle=WdfUsbTargetPipeWdmGetPipeHandle(pipe);
  r->Urb->UrbIsochronousTransfer.TransferFlags=USBD_START_ISO_TRANSFER_ASAP|
   (r->Direction?USBD_TRANSFER_DIRECTION_IN:USBD_TRANSFER_DIRECTION_OUT);
  r->Urb->UrbIsochronousTransfer.TransferBufferLength=bytes;
  r->Urb->UrbIsochronousTransfer.TransferBuffer=r->Data;
  r->Urb->UrbIsochronousTransfer.NumberOfPackets=MBOX_PACKET_COUNT;
  for(ULONG i=0;i<MBOX_PACKET_COUNT;i++)r->Urb->UrbIsochronousTransfer.IsoPacket[i].Offset=offsets[i];
  status=WdfUsbTargetPipeFormatRequestForUrb(pipe,request,urbMemory,NULL);
  if(!NT_SUCCESS(status))goto release;
  WdfRequestSetCompletionRoutine(request,MboxTransferComplete,NULL);
  WDF_REQUEST_SEND_OPTIONS_INIT(&options,WDF_REQUEST_SEND_OPTION_TIMEOUT);
  WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&options,WDF_REL_TIMEOUT_IN_SEC(2));
  if(WdfRequestSend(request,WdfUsbTargetPipeGetIoTarget(pipe),&options))return;
  status=WdfRequestGetStatus(request);
release:
  InterlockedAnd(&ctx->Busy,~r->Bit);
 }
done:
 WdfRequestComplete(request,status);
}
