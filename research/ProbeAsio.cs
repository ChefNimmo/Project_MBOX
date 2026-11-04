using System;
using System.Text;
using System.Collections.Generic;
using System.Runtime.InteropServices;
namespace MboxResearch {
public static partial class AsioProbe {
 [DllImport("ole32.dll")]static extern int CoInitializeEx(IntPtr p,uint flags);
 [DllImport("ole32.dll")]static extern void CoUninitialize();
 [DllImport("ole32.dll")]static extern int CoCreateInstance(ref Guid c,IntPtr outer,uint context,ref Guid iid,out IntPtr obj);
 [DllImport("user32.dll")]static extern IntPtr GetDesktopWindow();
 [UnmanagedFunctionPointer(CallingConvention.StdCall)]delegate int Init(IntPtr self,IntPtr window);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)]delegate int Release(IntPtr self);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)]delegate void Name(IntPtr self,StringBuilder text);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)]delegate int Channels(IntPtr self,out int input,out int output);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)]delegate int Buffers(IntPtr self,out int min,out int max,out int preferred,out int granularity);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)]delegate int Rate(IntPtr self,out double rate);
 static T Method<T>(IntPtr self,int slot) where T:class {return Marshal.GetDelegateForFunctionPointer(Marshal.ReadIntPtr(Marshal.ReadIntPtr(self),slot*IntPtr.Size),typeof(T)) as T;}
 public static string[] Read(){if(IntPtr.Size!=8)throw new Exception("x64 required");var rows=new List<string>();int apartment=CoInitializeEx(IntPtr.Zero,0);IntPtr obj=IntPtr.Zero;try{Guid id=new Guid("24BE3F40-4D2A-11D4-B580-00A0CC5BE45E");int hr=CoCreateInstance(ref id,IntPtr.Zero,1,ref id,out obj);rows.Add("CoCreate HRESULT="+hr.ToString("X8"));if(hr<0)return rows.ToArray();int initialized=Method<Init>(obj,3)(obj,GetDesktopWindow());rows.Add("init="+initialized);var name=new StringBuilder(256);Method<Name>(obj,4)(obj,name);rows.Add("name="+name);if(initialized==0){var error=new StringBuilder(256);Method<Name>(obj,6)(obj,error);rows.Add("error="+error);return rows.ToArray();}int input,output,min,max,preferred,gran;double rate;rows.Add("channels status="+Method<Channels>(obj,9)(obj,out input,out output)+" inputs="+input+" outputs="+output);rows.Add("buffer status="+Method<Buffers>(obj,11)(obj,out min,out max,out preferred,out gran)+" min="+min+" max="+max+" preferred="+preferred+" granularity="+gran);rows.Add("rate status="+Method<Rate>(obj,13)(obj,out rate)+" rate="+rate);return rows.ToArray();}finally{if(obj!=IntPtr.Zero)Method<Release>(obj,2)(obj);if(apartment>=0)CoUninitialize();}}
}}
