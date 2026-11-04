using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
namespace MboxResearch {
public static class WaveProbe {
 [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)] struct Caps {public ushort mid,pid;public uint version;[MarshalAs(UnmanagedType.ByValTStr,SizeConst=32)]public string name;public uint formats;public ushort channels,reserved;public uint support;}
 [StructLayout(LayoutKind.Sequential,Pack=2)] struct Format {public ushort tag,channels;public uint rate,bytesPerSecond;public ushort align,bits,extra;}
 [DllImport("winmm.dll")]static extern uint waveOutGetNumDevs();
 [DllImport("winmm.dll")]static extern uint waveInGetNumDevs();
 [DllImport("winmm.dll",CharSet=CharSet.Unicode)]static extern uint waveOutGetDevCaps(UIntPtr id,out Caps caps,uint size);
 [DllImport("winmm.dll",CharSet=CharSet.Unicode)]static extern uint waveInGetDevCaps(UIntPtr id,out Caps caps,uint size);
 [DllImport("winmm.dll")]static extern uint waveOutOpen(out IntPtr h,uint id,ref Format f,IntPtr cb,IntPtr instance,uint flags);
 [DllImport("winmm.dll")]static extern uint waveInOpen(out IntPtr h,uint id,ref Format f,IntPtr cb,IntPtr instance,uint flags);
 public static string[] Query(){var rows=new List<string>();for(int direction=0;direction<2;direction++){uint count=direction==0?waveOutGetNumDevs():waveInGetNumDevs();for(uint id=0;id<count;id++){Caps c;uint e=direction==0?waveOutGetDevCaps(new UIntPtr(id),out c,(uint)Marshal.SizeOf(typeof(Caps))):waveInGetDevCaps(new UIntPtr(id),out c,(uint)Marshal.SizeOf(typeof(Caps)));if(e!=0)continue;rows.Add((direction==0?"OUT":"IN")+" "+id+" "+c.name+" channels="+c.channels);if(c.name==null||c.name.IndexOf("USB",StringComparison.OrdinalIgnoreCase)<0)continue;foreach(uint rate in new uint[]{44100,48000})foreach(ushort bits in new ushort[]{16,24}){Format f=new Format{tag=1,channels=2,rate=rate,align=(ushort)(2*bits/8),bits=bits,bytesPerSecond=rate*2*bits/8};IntPtr h;uint status=direction==0?waveOutOpen(out h,id,ref f,IntPtr.Zero,IntPtr.Zero,1):waveInOpen(out h,id,ref f,IntPtr.Zero,IntPtr.Zero,1);rows.Add("FORMAT_QUERY "+(direction==0?"OUT":"IN")+" "+id+" "+rate+"/"+bits+" result="+status);}}}return rows.ToArray();}
}}
