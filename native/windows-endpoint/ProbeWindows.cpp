// SPDX-License-Identifier: GPL-3.0-only
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

template<class T> struct Com {
    T* p=nullptr;
    ~Com(){if(p)p->Release();}
    T** address(){return &p;}
    T* operator->(){return p;}
};
static HRESULT find(IMMDeviceEnumerator* enumerator,EDataFlow flow,IMMDevice** device)
{
    *device=nullptr;Com<IMMDeviceCollection> list;
    HRESULT hr=enumerator->EnumAudioEndpoints(flow,DEVICE_STATE_ACTIVE,list.address());
    if(FAILED(hr))return hr;
    UINT count=0;hr=list->GetCount(&count);if(FAILED(hr))return hr;
    unsigned matches=0;
    for(UINT i=0;i<count;i++) {
        Com<IMMDevice> item;Com<IPropertyStore> properties;PROPVARIANT name;PropVariantInit(&name);
        if(SUCCEEDED(list->Item(i,item.address())) &&
           SUCCEEDED(item->OpenPropertyStore(STGM_READ,properties.address())) &&
           SUCCEEDED(properties->GetValue(PKEY_Device_FriendlyName,&name)) && name.vt==VT_LPWSTR &&
           wcsstr(name.pwszVal,L"NimBox 1 USB audio device")) {
            wprintf(L"%s: %s\n",flow==eRender?L"Playback":L"Capture",name.pwszVal);
            matches++;if(matches==1){item.p->AddRef();*device=item.p;}
        }
        PropVariantClear(&name);
    }
    if(matches!=1){if(*device){(*device)->Release();*device=nullptr;}return HRESULT_FROM_WIN32(matches?ERROR_DUP_NAME:ERROR_NOT_FOUND);}
    return S_OK;
}
static HRESULT silentDuplex(IMMDevice* output,IMMDevice* input,unsigned seconds)
{
    Com<IAudioClient> render,capture;Com<IAudioRenderClient> writer;Com<IAudioCaptureClient> reader;
    WAVEFORMATEX *rf=nullptr,*cf=nullptr;
    HRESULT hr=output->Activate(__uuidof(IAudioClient),CLSCTX_ALL,nullptr,reinterpret_cast<void**>(render.address()));
    if(SUCCEEDED(hr))hr=input->Activate(__uuidof(IAudioClient),CLSCTX_ALL,nullptr,reinterpret_cast<void**>(capture.address()));
    if(SUCCEEDED(hr))hr=render->GetMixFormat(&rf);
    if(SUCCEEDED(hr))hr=capture->GetMixFormat(&cf);
    if(SUCCEEDED(hr)) {
        printf("Windows mix: render %lu Hz/%u channels/%u bits; capture %lu Hz/%u channels/%u bits\n",
            rf->nSamplesPerSec,rf->nChannels,rf->wBitsPerSample,cf->nSamplesPerSec,cf->nChannels,cf->wBitsPerSample);
        // Native capture numerical analysis handles PCM only in this first probe.
        if(cf->nChannels!=2||cf->wBitsPerSample!=32||cf->nSamplesPerSec!=48000)hr=AUDCLNT_E_UNSUPPORTED_FORMAT;
    }
    if(SUCCEEDED(hr))hr=render->Initialize(AUDCLNT_SHAREMODE_SHARED,0,1000000,0,rf,nullptr);
    if(SUCCEEDED(hr))hr=capture->Initialize(AUDCLNT_SHAREMODE_SHARED,0,1000000,0,cf,nullptr);
    if(SUCCEEDED(hr))hr=render->GetService(__uuidof(IAudioRenderClient),reinterpret_cast<void**>(writer.address()));
    if(SUCCEEDED(hr))hr=capture->GetService(__uuidof(IAudioCaptureClient),reinterpret_cast<void**>(reader.address()));
    UINT32 capacity=0;BYTE* buffer=nullptr;unsigned long long frames=0,discontinuities=0;
    if(SUCCEEDED(hr))hr=render->GetBufferSize(&capacity);
    if(SUCCEEDED(hr))hr=writer->GetBuffer(capacity,&buffer);
    if(SUCCEEDED(hr))hr=writer->ReleaseBuffer(capacity,AUDCLNT_BUFFERFLAGS_SILENT);
    if(SUCCEEDED(hr))hr=capture->Start();
    if(SUCCEEDED(hr))hr=render->Start();
    ULONGLONG deadline=GetTickCount64()+seconds*1000ull;
    while(SUCCEEDED(hr)&&GetTickCount64()<deadline) {
        UINT32 padding=0;hr=render->GetCurrentPadding(&padding);
        if(SUCCEEDED(hr)&&padding<capacity) {
            hr=writer->GetBuffer(capacity-padding,&buffer);
            if(SUCCEEDED(hr))hr=writer->ReleaseBuffer(capacity-padding,AUDCLNT_BUFFERFLAGS_SILENT);
        }
        UINT32 packet=0;if(SUCCEEDED(hr))hr=reader->GetNextPacketSize(&packet);
        while(SUCCEEDED(hr)&&packet) {
            UINT32 count=0;DWORD flags=0;
            hr=reader->GetBuffer(&buffer,&count,&flags,nullptr,nullptr);
            if(SUCCEEDED(hr)) {
                frames+=count;if(flags&AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY)discontinuities++;
                // No waveform is retained, logged or saved.
                hr=reader->ReleaseBuffer(count);
            }
            if(SUCCEEDED(hr))hr=reader->GetNextPacketSize(&packet);
        }
        Sleep(2);
    }
    if(render.p)render->Stop();if(capture.p)capture->Stop();
    CoTaskMemFree(rf);CoTaskMemFree(cf);
    printf("WASAPI silent duplex: capture frames=%llu discontinuities=%llu HRESULT=%08lx\n",frames,discontinuities,(unsigned long)hr);
    return SUCCEEDED(hr)&&!frames?E_FAIL:hr;
}
int wmain(int argc,wchar_t** argv)
{
    unsigned seconds=0;
    if(argc==3&&!wcscmp(argv[1],L"--silent")) {
        wchar_t* end=nullptr;unsigned long parsed=wcstoul(argv[2],&end,10);
        if(!*argv[2]||*end||parsed<1||parsed>600)return 2;seconds=parsed;
    } else if(argc!=1)return 2;
    HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);if(FAILED(hr))return 1;
    int result=1;
    {
        Com<IMMDeviceEnumerator> enumerator;Com<IMMDevice> output,input;
        hr=CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,__uuidof(IMMDeviceEnumerator),reinterpret_cast<void**>(enumerator.address()));
        if(SUCCEEDED(hr))hr=find(enumerator.p,eRender,output.address());
        if(SUCCEEDED(hr))hr=find(enumerator.p,eCapture,input.address());
        if(SUCCEEDED(hr)&&seconds)hr=silentDuplex(output.p,input.p,seconds);
        printf("Native Windows endpoint probe HRESULT=%08lx\n",(unsigned long)hr);
        result=SUCCEEDED(hr)?0:1;
    }
    CoUninitialize();return result;
}
