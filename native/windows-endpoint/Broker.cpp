// SPDX-License-Identifier: GPL-3.0-only
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <atomic>
#include "../asio/Engine.h"
#include "BridgeProtocol.h"

static HANDLE stopEvent, bridge=INVALID_HANDLE_VALUE;
static SERVICE_STATUS_HANDLE serviceHandle;
static SERVICE_STATUS serviceStatus{};
static std::atomic<DWORD> error{0};
static std::atomic<unsigned long long> callbacks{0};
static bool transportOnly;
static NimEndpointExchange block{};

static void status(DWORD state,DWORD code=NO_ERROR)
{
    if(!serviceHandle)return;
    serviceStatus.dwServiceType=SERVICE_WIN32_OWN_PROCESS;
    serviceStatus.dwCurrentState=state;
    serviceStatus.dwControlsAccepted=state==SERVICE_RUNNING?SERVICE_ACCEPT_STOP|SERVICE_ACCEPT_SHUTDOWN:0;
    serviceStatus.dwWin32ExitCode=code;
    serviceStatus.dwWaitHint=state==SERVICE_STOP_PENDING?10000:0;
    SetServiceStatus(serviceHandle,&serviceStatus);
}
static void failed(void*) { error=ERROR_DEVICE_NOT_CONNECTED;SetEvent(stopEvent); }
static void audio(void*,const int32_t* left,const int32_t* right,int32_t* outL,int32_t* outR)
{
    memset(outL,0,NIM_ENDPOINT_FRAMES*sizeof(*outL));
    memset(outR,0,NIM_ENDPOINT_FRAMES*sizeof(*outR));
    if(error.load())return;
    if(!transportOnly) {
        block.Version=NIM_ENDPOINT_VERSION;block.Size=sizeof(block);block.Flags=0;block.Reserved=0;
        for(unsigned i=0;i<NIM_ENDPOINT_FRAMES;i++) {block.Capture[2*i]=left[i];block.Capture[2*i+1]=right[i];}
        DWORD bytes=0;
        if(!DeviceIoControl(bridge,IOCTL_NIM_EXCHANGE,&block,sizeof(block),&block,sizeof(block),&bytes,nullptr)) {
            error=GetLastError();SetEvent(stopEvent);return;
        }
        if(bytes!=sizeof(block)||block.Version!=NIM_ENDPOINT_VERSION||block.Size!=sizeof(block)||
            block.Reserved||(block.Flags&~3u)) {
            error=ERROR_INVALID_DATA;SetEvent(stopEvent);return;
        }
        if(block.Flags&1u)for(unsigned i=0;i<NIM_ENDPOINT_FRAMES;i++) {outL[i]=block.Render[2*i];outR[i]=block.Render[2*i+1];}
        block.Sequence++;
    }
    callbacks++;
}
static DWORD run(DWORD milliseconds)
{
    // Fail closed: do not claim USB if the native endpoint driver isn't installed.
    if(!transportOnly) {
        bridge=CreateFileW(L"\\\\.\\NimBoxEndpointBridge",GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);
        if(bridge==INVALID_HANDLE_VALUE)return GetLastError();
    }
    // Use only the verified installed UsbDk runtime. No implicit DLL search.
    wchar_t helper[MAX_PATH],programFiles[MAX_PATH];
    DWORD count=GetEnvironmentVariableW(L"ProgramFiles",programFiles,MAX_PATH);
    DWORD result=ERROR_SUCCESS;
    NimEngine* engine=nullptr;
    if(!count||count>=MAX_PATH || swprintf_s(helper,L"%s\\UsbDk Runtime Library\\UsbDkHelper.dll",programFiles)<0)result=ERROR_BAD_PATHNAME;
    else {
        engine=nimCreate(helper);
        if(!engine)result=ERROR_DEVICE_NOT_CONNECTED;
        else if(!nimStart(engine,NIM_ENDPOINT_RATE,audio,nullptr,failed))result=ERROR_DEVICE_NOT_CONNECTED;
        else {
            status(SERVICE_RUNNING);
            WaitForSingleObject(stopEvent,milliseconds);
            status(SERVICE_STOP_PENDING);
            nimStop(engine,true);
            NimDiagnostics stats{};nimStats(engine,&stats);
            if(stats.Error||stats.Underruns||stats.Overflows||!callbacks.load())result=ERROR_IO_DEVICE;
            printf("callbacks=%llu IN=%llu OUT=%llu underruns=%llu overflows=%llu USB-error=%u bridge-error=%lu\n",
                callbacks.load(),stats.InputFrames,stats.OutputFrames,stats.Underruns,stats.Overflows,stats.Error,error.load());
        }
    }
    if(engine)nimDestroy(engine);
    if(bridge!=INVALID_HANDLE_VALUE){CloseHandle(bridge);bridge=INVALID_HANDLE_VALUE;}
    return error.load()?error.load():result;
}
static DWORD WINAPI control(DWORD code,DWORD,void*,void*)
{
    if(code==SERVICE_CONTROL_STOP||code==SERVICE_CONTROL_SHUTDOWN) {status(SERVICE_STOP_PENDING);SetEvent(stopEvent);return NO_ERROR;}
    if(code==SERVICE_CONTROL_INTERROGATE){SetServiceStatus(serviceHandle,&serviceStatus);return NO_ERROR;}
    return ERROR_CALL_NOT_IMPLEMENTED;
}
static void WINAPI serviceMain(DWORD,wchar_t**)
{
    serviceHandle=RegisterServiceCtrlHandlerExW(L"NimBoxAudio",control,nullptr);
    if(!serviceHandle)return;
    status(SERVICE_START_PENDING);
    stopEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    if(!stopEvent){status(SERVICE_STOPPED,GetLastError());return;}
    DWORD result=run(INFINITE);
    CloseHandle(stopEvent);status(SERVICE_STOPPED,result);
}
int wmain(int argc,wchar_t** argv)
{
    if(argc==3 && (!wcscmp(argv[1],L"--transport-check")||!wcscmp(argv[1],L"--console"))) {
        wchar_t* end=nullptr;unsigned long seconds=wcstoul(argv[2],&end,10);
        if(!*argv[2]||*end||seconds<1||seconds>600)return 2;
        transportOnly=!wcscmp(argv[1],L"--transport-check");
        stopEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);if(!stopEvent)return 1;
        DWORD result=run(seconds*1000);CloseHandle(stopEvent);
        printf("%s result=%lu\n",transportOnly?"Silent USB-only check (no Windows endpoint test)":"Native endpoint broker",result);
        return result?1:0;
    }
    if(argc!=1)return 2;
    SERVICE_TABLE_ENTRYW table[]={{const_cast<wchar_t*>(L"NimBoxAudio"),serviceMain},{nullptr,nullptr}};
    return StartServiceCtrlDispatcherW(table)?0:1;
}
