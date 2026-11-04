using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
namespace MboxResearch {
 public static class UsbDescriptors {
  [StructLayout(LayoutKind.Sequential)] struct InterfaceData {public int size; public Guid guid; public int flags; public IntPtr reserved;}
  [DllImport("setupapi.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern IntPtr SetupDiGetClassDevs(ref Guid g,string e,IntPtr w,uint flags);
  [DllImport("setupapi.dll",SetLastError=true)] static extern bool SetupDiEnumDeviceInterfaces(IntPtr s,IntPtr d,ref Guid g,uint index,ref InterfaceData data);
  [DllImport("setupapi.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern bool SetupDiGetDeviceInterfaceDetail(IntPtr s,ref InterfaceData d,IntPtr detail,uint bytes,out uint needed,IntPtr dev);
  [DllImport("setupapi.dll")] static extern bool SetupDiDestroyDeviceInfoList(IntPtr s);
  [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern SafeFileHandle CreateFile(string path,uint access,uint share,IntPtr security,uint creation,uint flags,IntPtr template);
  [DllImport("kernel32.dll",SetLastError=true)] static extern bool DeviceIoControl(SafeFileHandle h,uint code,byte[] input,int inputSize,byte[] output,int outputSize,out int returned,IntPtr overlap);
  public sealed class Result {public string Hub;public int Port;public string DeviceHex;public string ConfigurationHex;public string Error;}
  static string Hex(byte[] b,int start,int count){return BitConverter.ToString(b,start,count).Replace("-","");}
  public static Result[] Read() {
   var results=new List<Result>(); Guid guid=new Guid("f18a0e88-c30c-11d0-8815-00a0c906bed8");
   IntPtr set=SetupDiGetClassDevs(ref guid,null,IntPtr.Zero,0x12);
   if(set==new IntPtr(-1))throw new System.ComponentModel.Win32Exception();
   try {for(uint i=0;;i++){
    var data=new InterfaceData();data.size=Marshal.SizeOf(typeof(InterfaceData));
    if(!SetupDiEnumDeviceInterfaces(set,IntPtr.Zero,ref guid,i,ref data)){if(Marshal.GetLastWin32Error()==259)break;throw new System.ComponentModel.Win32Exception();}
    uint needed;SetupDiGetDeviceInterfaceDetail(set,ref data,IntPtr.Zero,0,out needed,IntPtr.Zero);
    IntPtr detail=Marshal.AllocHGlobal((int)needed);
    string path;
    try{Marshal.WriteInt32(detail,IntPtr.Size==8?8:6);if(!SetupDiGetDeviceInterfaceDetail(set,ref data,detail,needed,out needed,IntPtr.Zero))throw new System.ComponentModel.Win32Exception();path=Marshal.PtrToStringUni(IntPtr.Add(detail,4));}finally{Marshal.FreeHGlobal(detail);}
    using(var handle=CreateFile(path,0x40000000,3,IntPtr.Zero,3,0,IntPtr.Zero)){
     if(handle.IsInvalid){results.Add(new Result{Hub=path,Error="Hub open error "+Marshal.GetLastWin32Error()});continue;}
     int returned;var node=new byte[80];
     if(!DeviceIoControl(handle,0x220408,node,node.Length,node,node.Length,out returned,IntPtr.Zero)){results.Add(new Result{Hub=path,Error="Node query error "+Marshal.GetLastWin32Error()});continue;}
     int ports=node[6];
     for(int port=1;port<=ports;port++){
      var info=new byte[4096];Array.Copy(BitConverter.GetBytes(port),info,4);
      if(!DeviceIoControl(handle,0x220448,info,info.Length,info,info.Length,out returned,IntPtr.Zero))continue;
      if(returned<22||BitConverter.ToUInt16(info,12)!=0x0dba||BitConverter.ToUInt16(info,14)!=0x1000)continue;
      var result=new Result{Hub=path,Port=port,DeviceHex=Hex(info,4,18)};
      var config=new byte[4108];Array.Copy(BitConverter.GetBytes(port),config,4);
      config[4]=0x80;config[5]=6;config[7]=2;config[10]=0;config[11]=0x10;
      if(DeviceIoControl(handle,0x220410,config,config.Length,config,config.Length,out returned,IntPtr.Zero)&&returned>=21){
       int length=BitConverter.ToUInt16(config,14);
       if(length>=9&&length<=4096&&returned>=12+length)result.ConfigurationHex=Hex(config,12,length);
       else result.Error="Incomplete configuration descriptor: returned="+returned+", declared="+length;
      }else result.Error="Descriptor query error "+Marshal.GetLastWin32Error();
      results.Add(result);
     }
    }
   }}finally{SetupDiDestroyDeviceInfoList(set);}
   return results.ToArray();
  }
 }
}
