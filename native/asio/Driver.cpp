// SPDX-License-Identifier: GPL-3.0-only
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <unknwn.h>
#include <atomic>
#include <new>
#include <stdio.h>
#include <string>
#include <stdint.h>
#include "../third_party/asio/iasiodrv.h"
#include "Engine.h"
static HMODULE moduleHandle;
static std::atomic<long> objects{0},serverLocks{0};
static constexpr long blockSize=256;
template<class T> static void split64(T *out,uint64_t value){out->hi=(unsigned long)(value>>32);out->lo=(unsigned long)value;}
static uint64_t nanoTime() {
 LARGE_INTEGER count,frequency;QueryPerformanceCounter(&count);QueryPerformanceFrequency(&frequency);
 return (uint64_t)((double)count.QuadPart*1000000000.0/(double)frequency.QuadPart);
}
class NimBox final:public IASIO {
 std::atomic<ULONG> refs{1};NimEngine *engine=nullptr;
 ASIOCallbacks callbacks{};bool buffers=false,timeInfo=false;unsigned rate=48000,index=0;
 bool selected[2][2]{};int32_t samples[2][2][2][blockSize]{};
 std::atomic<uint64_t> position{0},timestamp{0};
 char error[124]="";HWND window=nullptr;
 static void audio(void *context,const int32_t *left,const int32_t *right,int32_t *outL,int32_t *outR) {
  auto self=static_cast<NimBox*>(context);unsigned slot=self->index;
  memcpy(self->samples[0][0][slot],left,blockSize*sizeof(int32_t));
  memcpy(self->samples[0][1][slot],right,blockSize*sizeof(int32_t));
  memset(self->samples[1][0][slot],0,blockSize*sizeof(int32_t));
  memset(self->samples[1][1][slot],0,blockSize*sizeof(int32_t));
  uint64_t now=nanoTime();self->timestamp=now;
  if(self->timeInfo) {
   ASIOTime time{};time.timeInfo.speed=1.0;time.timeInfo.sampleRate=self->rate;
   split64(&time.timeInfo.samplePosition,self->position.load());split64(&time.timeInfo.systemTime,now);
   time.timeInfo.flags=kSystemTimeValid|kSamplePositionValid|kSampleRateValid;
   self->callbacks.bufferSwitchTimeInfo(&time,(long)slot,ASIOTrue);
  } else self->callbacks.bufferSwitch((long)slot,ASIOTrue);
  if(self->selected[1][0])memcpy(outL,self->samples[1][0][slot],blockSize*sizeof(int32_t));
  if(self->selected[1][1])memcpy(outR,self->samples[1][1][slot],blockSize*sizeof(int32_t));
  self->position.fetch_add(blockSize);self->index^=1;
 }
public:
 NimBox(){objects++;}
 ~NimBox(){nimDestroy(engine);objects--;}
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void **object) override {
  if(!object)return E_POINTER;*object=nullptr;
  if(iid==IID_IUnknown||iid==CLSID_NimBox){*object=static_cast<IASIO*>(this);AddRef();return S_OK;}
  return E_NOINTERFACE;
 }
 ULONG STDMETHODCALLTYPE AddRef() override{return ++refs;}
 ULONG STDMETHODCALLTYPE Release() override {
  // Host must not destroy its driver from inside its own audio callback.
  if(refs.load()==1&&nimOnWorker(engine))return 1;
  ULONG count=--refs;if(!count)delete this;return count;
 }
 ASIOBool init(void *handle) override {
  window=static_cast<HWND>(handle);if(engine)return ASIOTrue;
  wchar_t path[1024];DWORD n=GetModuleFileNameW(moduleHandle,path,1024);
  if(!n||n>=1024){strcpy_s(error,"Cannot locate NimBox runtime.");return ASIOFalse;}
  std::wstring helper(path,n);auto separator=helper.find_last_of(L"\\/");
  if(separator==std::wstring::npos)return ASIOFalse;
  helper.resize(separator+1);helper+=L"UsbDkHelper.dll";
  engine=nimCreate(helper.c_str());
  if(!engine){strcpy_s(error,"Connect the original Mbox and install the signed UsbDk runtime.");return ASIOFalse;}
  error[0]=0;return ASIOTrue;
 }
 void getDriverName(char *name) override{if(name)strcpy_s(name,32,"NimBox 1 USB audio device");}
 long getDriverVersion() override{return 100;}
 void getErrorMessage(char *text) override{if(text)strcpy_s(text,124,error);}
 ASIOError start() override {
  if(!engine||!buffers)return ASE_InvalidMode;
  NimDiagnostics d{};nimStats(engine,&d);if(d.Running)return ASE_InvalidMode;
  position=0;timestamp=nanoTime();index=0;
  if(!nimStart(engine,rate,audio,this,[](void *context){auto self=static_cast<NimBox*>(context);if(self->callbacks.asioMessage)self->callbacks.asioMessage(kAsioResetRequest,0,nullptr,nullptr);})){strcpy_s(error,"USB streaming failed to start; close other Mbox clients.");return ASE_HWMalfunction;}
  return ASE_OK;
 }
 ASIOError stop() override {
  if(nimOnWorker(engine)){nimStop(engine);return ASE_InvalidMode;}
  nimStop(engine);return ASE_OK;
 }
 ASIOError getChannels(long *input,long *output) override {
  if(!input||!output)return ASE_InvalidParameter;*input=*output=2;return ASE_OK;
 }
 ASIOError getLatencies(long *input,long *output) override {
  if(!input||!output)return ASE_InvalidParameter;
  *input=blockSize+8*(long)((rate+999)/1000);*output=2560+blockSize;return ASE_OK;
 }
 ASIOError getBufferSize(long *minimum,long *maximum,long *preferred,long *granularity) override {
  if(!minimum||!maximum||!preferred||!granularity)return ASE_InvalidParameter;
  *minimum=*maximum=*preferred=blockSize;*granularity=0;return ASE_OK;
 }
 ASIOError canSampleRate(ASIOSampleRate requested) override{return requested==48000||requested==44100?ASE_OK:ASE_NoClock;}
 ASIOError getSampleRate(ASIOSampleRate *value) override{if(!value)return ASE_InvalidParameter;*value=rate;return ASE_OK;}
 ASIOError setSampleRate(ASIOSampleRate value) override {
  if(canSampleRate(value)!=ASE_OK)return ASE_NoClock;
  NimDiagnostics d{};nimStats(engine,&d);if(d.Running)return value==rate?ASE_OK:ASE_InvalidMode;
  rate=(unsigned)value;return ASE_OK;
 }
 ASIOError getClockSources(ASIOClockSource *clocks,long *count) override {
  if(!count)return ASE_InvalidParameter;
  if(!clocks){*count=1;return ASE_OK;}if(*count<1)return ASE_InvalidParameter;
  clocks[0]={};clocks[0].index=0;clocks[0].associatedChannel=-1;clocks[0].associatedGroup=-1;
  clocks[0].isCurrentSource=ASIOTrue;strcpy_s(clocks[0].name,"Internal");*count=1;return ASE_OK;
 }
 ASIOError setClockSource(long reference) override{return reference==0?ASE_OK:ASE_NoClock;}
 ASIOError getSamplePosition(ASIOSamples *sample,ASIOTimeStamp *time) override {
  if(!sample||!time)return ASE_InvalidParameter;split64(sample,position.load());split64(time,timestamp.load());return ASE_OK;
 }
 ASIOError getChannelInfo(ASIOChannelInfo *info) override {
  if(!info||info->channel<0||info->channel>=2||(info->isInput!=0&&info->isInput!=1))return ASE_InvalidParameter;
  info->type=ASIOSTInt32LSB;info->channelGroup=0;info->isActive=selected[info->isInput?0:1][info->channel]?ASIOTrue:ASIOFalse;
  sprintf_s(info->name,"%s %ld",info->isInput?"Analogue Input":"Analogue Output",info->channel+1);return ASE_OK;
 }
 ASIOError createBuffers(ASIOBufferInfo *info,long count,long size,ASIOCallbacks *cb) override {
  if(!engine||buffers)return ASE_InvalidMode;
  if(!info||!cb||count<1||count>4||size!=blockSize||!cb->bufferSwitch)return ASE_InvalidParameter;
  bool chosen[2][2]{};
  for(long i=0;i<count;i++) {
   if(info[i].channelNum<0||info[i].channelNum>=2||(info[i].isInput!=0&&info[i].isInput!=1))return ASE_InvalidParameter;
   unsigned direction=info[i].isInput?0:1,channel=(unsigned)info[i].channelNum;
   if(chosen[direction][channel])return ASE_InvalidParameter;chosen[direction][channel]=true;
  }
  callbacks=*cb;timeInfo=cb->bufferSwitchTimeInfo&&cb->asioMessage&&cb->asioMessage(kAsioSupportsTimeInfo,0,nullptr,nullptr)==1;
  memcpy(selected,chosen,sizeof(selected));memset(samples,0,sizeof(samples));
  for(long i=0;i<count;i++)for(unsigned b=0;b<2;b++)info[i].buffers[b]=samples[info[i].isInput?0:1][info[i].channelNum][b];
  buffers=true;return ASE_OK;
 }
 ASIOError disposeBuffers() override {
  if(nimOnWorker(engine))return ASE_InvalidMode;
  nimStop(engine);buffers=false;callbacks={};memset(selected,0,sizeof(selected));return ASE_OK;
 }
 ASIOError controlPanel() override {
  NimDiagnostics d{};nimStats(engine,&d);wchar_t text[640];
  unsigned seconds=(unsigned)(d.MutedFrames/rate);
  swprintf_s(text,L"NimBox 1 USB audio device\n\n2 inputs / 2 outputs, %u Hz\n256-frame ASIO buffers\nInput is muted for the first 10 seconds while it settles.\n\nRunning: %s\nInput warm-up processed: %u seconds\nUnderrun frames: %llu\nOverflow events: %llu\nUSB error: %u\n\nExperimental build. Latency values are estimates.\nUse the hardware gain, Mix and headphone controls.",
   rate,d.Running?L"Yes":L"No",seconds,d.Underruns,d.Overflows,d.Error);
  MessageBoxW(window,text,NIMBOX_NAME,MB_OK|MB_ICONINFORMATION);return ASE_OK;
 }
 ASIOError future(long selector,void*) override{return selector==kAsioCanTimeInfo?ASE_SUCCESS:ASE_NotPresent;}
 ASIOError outputReady() override{return ASE_NotPresent;}
 void diagnostics(NimDiagnostics *out){nimStats(engine,out);}
};
class Factory final:public IClassFactory {
 std::atomic<ULONG> refs{1};
public:
 Factory(){objects++;}~Factory(){objects--;}
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void **out) override {
  if(!out)return E_POINTER;*out=nullptr;if(iid!=IID_IUnknown&&iid!=IID_IClassFactory)return E_NOINTERFACE;
  *out=this;AddRef();return S_OK;
 }
 ULONG STDMETHODCALLTYPE AddRef() override{return ++refs;}
 ULONG STDMETHODCALLTYPE Release() override{ULONG n=--refs;if(!n)delete this;return n;}
 HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown *outer,REFIID iid,void **out) override {
  if(!out)return E_POINTER;*out=nullptr;if(outer)return CLASS_E_NOAGGREGATION;
  auto driver=new(std::nothrow) NimBox;if(!driver)return E_OUTOFMEMORY;
  HRESULT status=driver->QueryInterface(iid,out);driver->Release();return status;
 }
 HRESULT STDMETHODCALLTYPE LockServer(BOOL lock) override{if(lock)serverLocks++;else if(serverLocks>0)serverLocks--;return S_OK;}
};
_Check_return_ STDAPI DllGetClassObject(_In_ REFCLSID clsid,_In_ REFIID iid,_Outptr_ LPVOID *out) {
 if(!out)return E_POINTER;*out=nullptr;if(clsid!=CLSID_NimBox)return CLASS_E_CLASSNOTAVAILABLE;
 auto factory=new(std::nothrow) Factory;if(!factory)return E_OUTOFMEMORY;
 HRESULT status=factory->QueryInterface(iid,out);factory->Release();return status;
}
__control_entrypoint(DllExport) STDAPI DllCanUnloadNow(){return objects==0&&serverLocks==0?S_OK:S_FALSE;}
static LONG writeValue(const wchar_t *path,const wchar_t *name,const wchar_t *value) {
 HKEY key;LONG status=RegCreateKeyExW(HKEY_LOCAL_MACHINE,path,0,nullptr,0,KEY_WRITE|KEY_WOW64_64KEY,nullptr,&key,nullptr);
 if(status!=ERROR_SUCCESS)return status;
 status=RegSetValueExW(key,name,0,REG_SZ,(const BYTE*)value,(DWORD)((wcslen(value)+1)*sizeof(wchar_t)));RegCloseKey(key);return status;
}
extern "C" __declspec(dllexport) HRESULT __stdcall DllUnregisterServer() {
 LONG a=RegDeleteTreeW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\ASIO\\" NIMBOX_NAME);
 LONG b=RegDeleteTreeW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Classes\\CLSID\\" NIMBOX_CLSID_TEXT);
 if(a!=ERROR_SUCCESS&&a!=ERROR_FILE_NOT_FOUND)return HRESULT_FROM_WIN32(a);
 return b==ERROR_SUCCESS||b==ERROR_FILE_NOT_FOUND?S_OK:HRESULT_FROM_WIN32(b);
}
extern "C" __declspec(dllexport) HRESULT __stdcall DllRegisterServer() {
 wchar_t path[1024];DWORD n=GetModuleFileNameW(moduleHandle,path,1024);if(!n||n>=1024)return E_FAIL;
 LONG status=writeValue(L"SOFTWARE\\Classes\\CLSID\\" NIMBOX_CLSID_TEXT,nullptr,NIMBOX_NAME);
 if(!status)status=writeValue(L"SOFTWARE\\Classes\\CLSID\\" NIMBOX_CLSID_TEXT L"\\InprocServer32",nullptr,path);
 if(!status)status=writeValue(L"SOFTWARE\\Classes\\CLSID\\" NIMBOX_CLSID_TEXT L"\\InprocServer32",L"ThreadingModel",L"Both");
 if(!status)status=writeValue(L"SOFTWARE\\ASIO\\" NIMBOX_NAME,L"CLSID",NIMBOX_CLSID_TEXT);
 if(!status)status=writeValue(L"SOFTWARE\\ASIO\\" NIMBOX_NAME,L"Description",NIMBOX_NAME);
 if(status){DllUnregisterServer();return HRESULT_FROM_WIN32(status);}return S_OK;
}
extern "C" __declspec(dllexport) void __stdcall NimBoxGetDiagnostics(IASIO *driver,NimDiagnostics *out) {
 if(driver&&out)static_cast<NimBox*>(driver)->diagnostics(out);
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID) {
 // /MT requires CRT thread attach/detach notifications to remain enabled.
 if(reason==DLL_PROCESS_ATTACH)moduleHandle=instance;return TRUE;
}
