// SPDX-License-Identifier: GPL-3.0-only
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <unknwn.h>
#include <stdio.h>
#include <atomic>
#include <math.h>
#include "../third_party/asio/iasiodrv.h"
#include "../core/mbox_test_tone.h"
#include "NimBoxIds.h"
static ASIOBufferInfo buffers[4];
static std::atomic<uint64_t> calls{0},bad{0};
static uint64_t tonePosition=0,lastPosition=0;static unsigned sampleRate=48000;
static bool tone=false,legacy=false;static double energy[2]={0,0};static uint64_t measured=0;
static void process(long index) {
 if(index<0||index>1){bad++;return;}
 auto l=static_cast<int32_t*>(buffers[0].buffers[index]);auto r=static_cast<int32_t*>(buffers[1].buffers[index]);
 for(unsigned n=0;n<256;n++) {
  double x=l[n]/2147483648.0,y=r[n]/2147483648.0;energy[0]+=x*x;energy[1]+=y*y;
 }
 measured+=256;
 auto outL=static_cast<int32_t*>(buffers[2].buffers[index]);auto outR=static_cast<int32_t*>(buffers[3].buffers[index]);
 if(tone)mbox_test_tone(outL,outR,256,sampleRate,&tonePosition,(uint64_t)sampleRate*5);
 else {memset(outL,0,256*sizeof(*outL));memset(outR,0,256*sizeof(*outR));}
 calls++;
}
static void bufferSwitch(long index,ASIOBool){process(index);}
static void changed(ASIOSampleRate){bad++;}
static long message(long selector,long value,void*,double*) {
 if(selector==kAsioEngineVersion)return 2;
 if(selector==kAsioSupportsTimeInfo)return legacy?0:1;
 if(selector==kAsioSelectorSupported)return value==kAsioSupportsTimeInfo?1:0;
 if(selector==kAsioResetRequest||selector==kAsioResyncRequest)bad++;
 return 0;
}
static ASIOTime *timeSwitch(ASIOTime *time,long index,ASIOBool) {
 uint64_t position=((uint64_t)time->timeInfo.samplePosition.hi<<32)|time->timeInfo.samplePosition.lo;
 if(calls.load()&&position!=lastPosition+256)bad++;
 lastPosition=position;process(index);return time;
}
int main(int argc,char **argv) {
 setvbuf(stdout,nullptr,_IONBF,0);
 if(argc<3){puts("NimBoxAsioProbe.exe absolute-DLL-path seconds [44100|48000] [--tone] [--legacy] [--registered]");return 2;}
 char *end=nullptr;unsigned long seconds=strtoul(argv[2],&end,10);
 if(!*argv[2]||*end||seconds<1||seconds>600)return 2;
 bool registered=false,restart=false;
 for(int i=3;i<argc;i++) {
  if(!strcmp(argv[i],"44100"))sampleRate=44100;else if(!strcmp(argv[i],"48000"))sampleRate=48000;
  else if(!strcmp(argv[i],"--tone"))tone=true;else if(!strcmp(argv[i],"--legacy"))legacy=true;
  else if(!strcmp(argv[i],"--registered"))registered=true;
  else if(!strcmp(argv[i],"--restart"))restart=true;else return 2;
 }
 HMODULE dll=LoadLibraryExA(argv[1],nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
 if(!dll){printf("Load DLL error=%lu\n",GetLastError());return 1;}
 LPFNGETCLASSOBJECT getFactory=nullptr;FARPROC p=GetProcAddress(dll,"DllGetClassObject");memcpy(&getFactory,&p,sizeof(p));
 typedef void (__stdcall *Diagnostics)(IASIO*,NimDiagnostics*);Diagnostics diagnostics=nullptr;
 p=GetProcAddress(dll,"NimBoxGetDiagnostics");memcpy(&diagnostics,&p,sizeof(p));
 LPFNCANUNLOADNOW canUnload=nullptr;p=GetProcAddress(dll,"DllCanUnloadNow");memcpy(&canUnload,&p,sizeof(p));
 if(!getFactory||!diagnostics||!canUnload){FreeLibrary(dll);return 1;}
 HRESULT apartment=CoInitializeEx(nullptr,COINIT_MULTITHREADED);IASIO *driver=nullptr;IClassFactory *factory=nullptr;
 int result=1;
 do {
  if(registered) {
   HRESULT hr=CoCreateInstance(CLSID_NimBox,nullptr,CLSCTX_INPROC_SERVER,CLSID_NimBox,(void**)&driver);
   printf("Registered CoCreateInstance=%08lx\n",(unsigned long)hr);if(FAILED(hr))break;
  } else {
   if(FAILED(getFactory(CLSID_NimBox,IID_IClassFactory,(void**)&factory)))break;
   if(FAILED(factory->CreateInstance(nullptr,CLSID_NimBox,(void**)&driver)))break;
   factory->Release();factory=nullptr;
  }
  if(!driver->init(GetDesktopWindow())){char error[124];driver->getErrorMessage(error);printf("init failed: %s\n",error);break;}
  char name[32];driver->getDriverName(name);long in=0,out=0,min=0,max=0,preferred=0,gran=0;
  if(driver->getChannels(&in,&out)!=ASE_OK||in!=2||out!=2)break;
  if(driver->getBufferSize(&min,&max,&preferred,&gran)!=ASE_OK||min!=256||max!=256||preferred!=256)break;
  if(driver->canSampleRate(96000)!=ASE_NoClock||driver->setSampleRate(sampleRate)!=ASE_OK)break;
  printf("%s: %ld in / %ld out, %u Hz, %ld frames\n",name,in,out,sampleRate,preferred);
  for(unsigned i=0;i<4;i++){buffers[i].isInput=i<2?ASIOTrue:ASIOFalse;buffers[i].channelNum=i%2;}
  ASIOCallbacks cb={bufferSwitch,changed,message,timeSwitch};
  ASIOBufferInfo invalid[2]={{ASIOTrue,0,{nullptr,nullptr}},{ASIOTrue,0,{nullptr,nullptr}}};
  if(driver->createBuffers(invalid,2,256,&cb)!=ASE_InvalidParameter)break;
  if(driver->createBuffers(buffers,4,128,&cb)!=ASE_InvalidParameter)break;
  if(driver->createBuffers(buffers,4,256,&cb)!=ASE_OK)break;
  if(driver->start()!=ASE_OK){char error[124];driver->getErrorMessage(error);puts(error);break;}
  puts(tone?"Five-second -40 dBFS tone enabled, then silence.":"Silent ASIO stream started.");
  Sleep((DWORD)seconds*1000);
  if(driver->stop()!=ASE_OK)break;
  NimDiagnostics d{};diagnostics(driver,&d);
  printf("callbacks=%llu, bad callbacks=%llu, IN/OUT frames=%llu/%llu, underruns=%llu, overflows=%llu, muted=%llu, USB error=%u\n",
   calls.load(),bad.load(),d.InputFrames,d.OutputFrames,d.Underruns,d.Overflows,d.MutedFrames,d.Error);
  for(unsigned c=0;c<2;c++)printf("Channel %u delivered RMS %.2f dBFS (includes startup mute)\n",c+1,
   measured&&energy[c]>0?10*log10(energy[c]/(double)measured):-300.0);
  result=calls.load()>0&&bad.load()==0&&d.Error==0&&d.Underruns==0&&d.Overflows==0?0:1;
  if(restart&&result==0) {
   calls=0;bad=0;lastPosition=0;tonePosition=0;
   if(driver->start()!=ASE_OK){puts("Immediate ASIO restart failed.");result=1;}
   else {
    Sleep(2000);driver->stop();diagnostics(driver,&d);
    printf("Restart: callbacks=%llu, bad=%llu, underruns=%llu, overflows=%llu, USB error=%u\n",
     calls.load(),bad.load(),d.Underruns,d.Overflows,d.Error);
    if(!calls.load()||bad.load()||d.Underruns||d.Overflows||d.Error)result=1;
   }
  }
  if(driver->disposeBuffers()!=ASE_OK)break;
 } while(false);
 if(driver){driver->stop();driver->disposeBuffers();driver->Release();}
 if(factory)factory->Release();
 if(canUnload()!=S_OK){puts("COM objects remain alive.");result=1;}
 if(apartment>=0)CoUninitialize();FreeLibrary(dll);return result;
}
