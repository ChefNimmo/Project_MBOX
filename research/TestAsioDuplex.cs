using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Threading;
namespace MboxResearch {
public static partial class AsioProbe {
 [StructLayout(LayoutKind.Sequential)]struct BufferInfo{public int input,channel;public IntPtr a,b;}
 [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Ansi)]struct ChannelInfo{public int channel,input,active,group,type;[MarshalAs(UnmanagedType.ByValTStr,SizeConst=32)]public string name;}
 [StructLayout(LayoutKind.Sequential)]struct Callbacks{public IntPtr buffer,rate,message,time;}
 [UnmanagedFunctionPointer(CallingConvention.StdCall)]delegate int SetRate(IntPtr self,double rate);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)]delegate int ActionMethod(IntPtr self);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)]delegate int ChannelMethod(IntPtr self,ref ChannelInfo info);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)]delegate int CreateBuffers(IntPtr self,IntPtr info,int channels,int samples,ref Callbacks callbacks);
 [UnmanagedFunctionPointer(CallingConvention.Cdecl)]delegate void Switch(int index,int direct);
 [UnmanagedFunctionPointer(CallingConvention.Cdecl)]delegate void RateChanged(double rate);
 [UnmanagedFunctionPointer(CallingConvention.Cdecl)]delegate int Message(int selector,int value,IntPtr message,IntPtr opt);
 [UnmanagedFunctionPointer(CallingConvention.Cdecl)]delegate IntPtr TimeSwitch(IntPtr time,int index,int direct);
 public static string[] Stream(double requestedRate,int seconds,bool tone=false){
  if(IntPtr.Size!=8)throw new Exception("x64 required");
  var result=new List<string>();int apt=CoInitializeEx(IntPtr.Zero,0);IntPtr obj=IntPtr.Zero,infos=IntPtr.Zero;bool allocated=false,started=false;int switches=0,resets=0,rateChanges=0;Switch sw=null;RateChanged rc=null;Message msg=null;TimeSwitch ts=null;
  try{Guid id=new Guid("24BE3F40-4D2A-11D4-B580-00A0CC5BE45E");Marshal.ThrowExceptionForHR(CoCreateInstance(ref id,IntPtr.Zero,1,ref id,out obj));
   if(Method<Init>(obj,3)(obj,GetDesktopWindow())==0)throw new Exception("ASIO init failed");
   int err=Method<SetRate>(obj,14)(obj,requestedRate);if(err!=0)throw new Exception("set rate error="+err);
   int min,max,size,gran;err=Method<Buffers>(obj,11)(obj,out min,out max,out size,out gran);if(err!=0||size<1||size>65536)throw new Exception("Invalid buffer size");
   var channels=new BufferInfo[4];var widths=new int[4];
   for(int c=0;c<4;c++){ChannelInfo ci=new ChannelInfo{input=c<2?1:0,channel=c%2};err=Method<ChannelMethod>(obj,18)(obj,ref ci);if(err!=0)throw new Exception("channel error="+err);int width=ci.type==16?2:ci.type==17?3:ci.type==18||ci.type==19?4:ci.type==20?8:0;if(width==0)throw new Exception("Unsupported probe sample type="+ci.type);widths[c]=width;channels[c]=new BufferInfo{input=ci.input,channel=ci.channel};result.Add("channel input="+ci.input+" index="+ci.channel+" type="+ci.type+" name="+ci.name);}
   int stride=Marshal.SizeOf(typeof(BufferInfo));infos=Marshal.AllocHGlobal(stride*4);for(int c=0;c<4;c++)Marshal.StructureToPtr(channels[c],IntPtr.Add(infos,c*stride),false);
   var silence=new byte[size*8];var scratch=new byte[size*8];var inputPeak=new long[2];var inputNonzero=new long[2];
   if(tone){if(requestedRate!=48000||size!=256||widths[2]!=4||widths[3]!=4)throw new Exception("Tone probe requires 48k/256/Int32");for(int n=0;n<size;n++){int sample=(int)(Math.Sin(2*Math.PI*750*n/requestedRate)*2147483647*0.05);byte[] encoded=BitConverter.GetBytes(sample);Array.Copy(encoded,0,silence,n*4,4);}}
   sw=(index,direct)=>{try{if(index<0||index>1)return;for(int c=2;c<4;c++){IntPtr p=index==0?channels[c].a:channels[c].b;if(p!=IntPtr.Zero)Marshal.Copy(silence,0,p,size*widths[c]);}for(int c=0;c<2;c++){if(widths[c]!=4)continue;IntPtr p=index==0?channels[c].a:channels[c].b;if(p==IntPtr.Zero)continue;Marshal.Copy(p,scratch,0,size*4);for(int n=0;n<size;n++){long sample=BitConverter.ToInt32(scratch,n*4);long abs=Math.Abs(sample);if(abs>inputPeak[c])inputPeak[c]=abs;if(sample!=0)inputNonzero[c]++;}}Interlocked.Increment(ref switches);}catch{Interlocked.Increment(ref resets);}};
   rc=(r)=>Interlocked.Increment(ref rateChanges);
   msg=(selector,value,m,o)=>{if(selector==2)return 2;if(selector==1)return value==2?1:0;if(selector==3)Interlocked.Increment(ref resets);return 0;};
   ts=(t,index,direct)=>{sw(index,direct);return t;};
   Callbacks cb=new Callbacks{buffer=Marshal.GetFunctionPointerForDelegate(sw),rate=Marshal.GetFunctionPointerForDelegate(rc),message=Marshal.GetFunctionPointerForDelegate(msg),time=Marshal.GetFunctionPointerForDelegate(ts)};
   err=Method<CreateBuffers>(obj,19)(obj,infos,4,size,ref cb);if(err!=0)throw new Exception("createBuffers error="+err);allocated=true;
   for(int c=0;c<4;c++)channels[c]=(BufferInfo)Marshal.PtrToStructure(IntPtr.Add(infos,c*stride),typeof(BufferInfo));
   for(int c=2;c<4;c++){Marshal.Copy(silence,0,channels[c].a,size*widths[c]);Marshal.Copy(silence,0,channels[c].b,size*widths[c]);}
   err=Method<ActionMethod>(obj,7)(obj);if(err!=0)throw new Exception("start error="+err);started=true;Thread.Sleep(seconds*1000);
   err=Method<ActionMethod>(obj,8)(obj);started=false;result.Add("rate="+requestedRate+" seconds="+seconds+" buffer="+size+" callbacks="+switches+" callbackErrorsOrResetRequests="+resets+" rateChanges="+rateChanges+" stopStatus="+err);for(int c=0;c<2;c++)result.Add("input="+c+" peakAbs32="+inputPeak[c]+" nonzeroSamples="+inputNonzero[c]);return result.ToArray();
  }finally{if(obj!=IntPtr.Zero){if(started)Method<ActionMethod>(obj,8)(obj);if(allocated)Method<ActionMethod>(obj,20)(obj);Method<Release>(obj,2)(obj);}if(infos!=IntPtr.Zero)Marshal.FreeHGlobal(infos);GC.KeepAlive(sw);GC.KeepAlive(rc);GC.KeepAlive(msg);GC.KeepAlive(ts);if(apt>=0)CoUninitialize();}
 }
}}
