// SPDX-License-Identifier: GPL-3.0-only
#include <sysvad.h>
#include <wdmsec.h>
#include "simple.h"
#include "minwavert.h"
#include "minwavertstream.h"
#include "NimBridge.h"
#include "EndpointCore.h"

static PDEVICE_OBJECT device;
static PDRIVER_DISPATCH original[IRP_MJ_MAXIMUM_FUNCTION + 1];
static KSPIN_LOCK streamsLock;
static CMiniportWaveRTStream* streams[2];
static ULONGLONG sequence;
static const GUID securityClass = {0x874a48dd,0x2577,0x4db6,{0x9d,0xf6,0xf9,0x01,0x72,0x14,0x84,0xe5}};

NTSTATUS NimBridgeRegister(CMiniportWaveRTStream* stream, BOOLEAN capture)
{
    KIRQL irql; KeAcquireSpinLock(&streamsLock, &irql);
    const unsigned index = capture ? 1u : 0u;
    NTSTATUS status = streams[index] ? STATUS_DEVICE_BUSY : STATUS_SUCCESS;
    if (NT_SUCCESS(status)) streams[index] = stream;
    KeReleaseSpinLock(&streamsLock, irql);
    return status;
}
void NimBridgeUnregister(CMiniportWaveRTStream* stream)
{
    KIRQL irql; KeAcquireSpinLock(&streamsLock, &irql);
    for (unsigned i=0;i<2;i++) if (streams[i]==stream) streams[i]=nullptr;
    KeReleaseSpinLock(&streamsLock, irql);
}

static DRIVER_DISPATCH Dispatch;
_Use_decl_annotations_
static NTSTATUS Dispatch(PDEVICE_OBJECT object, PIRP irp)
{
    auto stack=IoGetCurrentIrpStackLocation(irp);
    if (object != device) return original[stack->MajorFunction](object,irp);
    NTSTATUS status=STATUS_INVALID_DEVICE_REQUEST;
    ULONG_PTR bytes=0;
    switch(stack->MajorFunction) {
    case IRP_MJ_CREATE:
        sequence=0; status=STATUS_SUCCESS; break;
    case IRP_MJ_CLEANUP:
    case IRP_MJ_CLOSE:
        status=STATUS_SUCCESS; break;
    case IRP_MJ_DEVICE_CONTROL:
        if (stack->Parameters.DeviceIoControl.IoControlCode==IOCTL_NIM_EXCHANGE) {
            if (stack->Parameters.DeviceIoControl.InputBufferLength!=sizeof(NimEndpointExchange) ||
                stack->Parameters.DeviceIoControl.OutputBufferLength!=sizeof(NimEndpointExchange)) {
                status=STATUS_INFO_LENGTH_MISMATCH; break;
            }
            auto block=static_cast<NimEndpointExchange*>(irp->AssociatedIrp.SystemBuffer);
            // Registry lock also protects stream lifetime: the destructor unregisters first.
            // Global -> position lock is the only permitted lock order.
            KIRQL irql; KeAcquireSpinLock(&streamsLock,&irql);
            if (!nimValidExchange(block,stack->Parameters.DeviceIoControl.InputBufferLength,
                stack->Parameters.DeviceIoControl.OutputBufferLength,sequence)) {
                KeReleaseSpinLock(&streamsLock,irql);status=STATUS_INVALID_PARAMETER;break;
            }
            RtlZeroMemory(block->Render,sizeof(block->Render));
            for(unsigned i=0;i<2;i++) if(streams[i] && streams[i]->NimTransfer(block)) block->Flags|=1u<<i;
            sequence++;
            KeReleaseSpinLock(&streamsLock,irql);
            bytes=sizeof(*block); status=STATUS_SUCCESS;
        }
        break;
    }
    irp->IoStatus.Status=status; irp->IoStatus.Information=bytes;
    IoCompleteRequest(irp,IO_NO_INCREMENT); return status;
}

NTSTATUS NimBridgeInitialize(PDRIVER_OBJECT driver)
{
    KeInitializeSpinLock(&streamsLock);
    UNICODE_STRING name=RTL_CONSTANT_STRING(L"\\Device\\NimBoxEndpointBridge");
    UNICODE_STRING link=RTL_CONSTANT_STRING(L"\\DosDevices\\NimBoxEndpointBridge");
    UNICODE_STRING sddl=RTL_CONSTANT_STRING(L"D:P(A;;GA;;;SY)(A;;GA;;;BA)");
    NTSTATUS status=IoCreateDeviceSecure(driver,0,&name,FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,TRUE,&sddl,&securityClass,&device);
    if(!NT_SUCCESS(status))return status;
    status=IoCreateSymbolicLink(&link,&name);
    if(!NT_SUCCESS(status)){IoDeleteDevice(device);device=nullptr;return status;}
    for(unsigned i=0;i<=IRP_MJ_MAXIMUM_FUNCTION;i++) {
        original[i]=driver->MajorFunction[i]; driver->MajorFunction[i]=Dispatch;
    }
    device->Flags&=~DO_DEVICE_INITIALIZING;
    return STATUS_SUCCESS;
}
void NimBridgeShutdown()
{
    if(!device)return;
    UNICODE_STRING link=RTL_CONSTANT_STRING(L"\\DosDevices\\NimBoxEndpointBridge");
    IoDeleteSymbolicLink(&link);IoDeleteDevice(device);device=nullptr;
}

