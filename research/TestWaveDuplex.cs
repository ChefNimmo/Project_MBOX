using System;
using System.Runtime.InteropServices;
using System.Threading;
namespace MboxResearch {
 public static class DuplexProbe {
 [StructLayout(LayoutKind.Sequential,Pack=2)] struct F {public ushort tag,ch;public uint rate,bps;public ushort align,bits,extra;}
 [StructLayout(LayoutKind.Sequential)] struct H {public IntPtr data;public uint length,recorded;public UIntPtr user;public uint flags,loops;public IntPtr next;public UIntPtr reserved;}
 [DllImport("winmm.dll")]static extern uint waveOutOpen(out IntPtr h,uint id,ref F f,IntPtr cb,IntPtr instance,uint flags);
 [DllImport("winmm.dll")]static extern uint waveInOpen(out IntPtr h,uint id,ref F f,IntPtr cb,IntPtr instance,uint flags);
 [DllImport("winmm.dll")]static extern uint waveOutPrepareHeader(IntPtr h,IntPtr p,uint size);
 [DllImport("winmm.dll")]static extern uint waveInPrepareHeader(IntPtr h,IntPtr p,uint size);
 [DllImport("winmm.dll")]static extern uint waveOutUnprepareHeader(IntPtr h,IntPtr p,uint size);
 [DllImport("winmm.dll")]static extern uint waveInUnprepareHeader(IntPtr h,IntPtr p,uint size);
 [DllImport("winmm.dll")]static extern uint waveOutWrite(IntPtr h,IntPtr p,uint size);
 [DllImport("winmm.dll")]static extern uint waveInAddBuffer(IntPtr h,IntPtr p,uint size);
 [DllImport("winmm.dll")]static extern uint waveInStart(IntPtr h);
 [DllImport("winmm.dll")]static extern uint waveInReset(IntPtr h);
 [DllImport("winmm.dll")]static extern uint waveOutReset(IntPtr h);
 [DllImport("winmm.dll")]static extern uint waveInClose(IntPtr h);
 [DllImport("winmm.dll")]static extern uint waveOutClose(IntPtr h);
 static void Check(uint result,string operation){if(result!=0)throw new Exception(operation+" error="+result);}
 public static string Run(uint output,uint input,uint rate,ushort bits,int mode=3) {
  F format=new F{tag=1,ch=2,rate=rate,bps=rate*2*bits/8,align=(ushort)(2*bits/8),bits=bits};
  IntPtr oh=IntPtr.Zero,ih=IntPtr.Zero,ob=IntPtr.Zero,ib=IntPtr.Zero,op=IntPtr.Zero,ip=IntPtr.Zero;
  bool preparedOut=false,preparedIn=false;uint size=(uint)Marshal.SizeOf(typeof(H));
  try{
   if((mode&1)!=0)Check(waveOutOpen(out oh,output,ref format,IntPtr.Zero,IntPtr.Zero,0),"open output");
   if((mode&2)!=0)Check(waveInOpen(out ih,input,ref format,IntPtr.Zero,IntPtr.Zero,0),"open input");
   int bytes=checked((int)format.bps*2);ob=Marshal.AllocHGlobal(bytes);ib=Marshal.AllocHGlobal(bytes);
   Marshal.Copy(new byte[bytes],0,ob,bytes);Marshal.Copy(new byte[bytes],0,ib,bytes);
   op=Marshal.AllocHGlobal((int)size);ip=Marshal.AllocHGlobal((int)size);
   Marshal.StructureToPtr(new H{data=ob,length=(uint)bytes},op,false);Marshal.StructureToPtr(new H{data=ib,length=(uint)bytes},ip,false);
   if(oh!=IntPtr.Zero){Check(waveOutPrepareHeader(oh,op,size),"prepare output");preparedOut=true;}
   if(ih!=IntPtr.Zero){Check(waveInPrepareHeader(ih,ip,size),"prepare input");preparedIn=true;Check(waveInAddBuffer(ih,ip,size),"queue input");Check(waveInStart(ih),"start input");}
   if(oh!=IntPtr.Zero)Check(waveOutWrite(oh,op,size),"queue output");
   DateTime deadline=DateTime.UtcNow.AddSeconds(6);H o=new H(),i=new H();
   do{Thread.Sleep(20);o=(H)Marshal.PtrToStructure(op,typeof(H));i=(H)Marshal.PtrToStructure(ip,typeof(H));if((oh==IntPtr.Zero||(o.flags&1)!=0)&&(ih==IntPtr.Zero||(i.flags&1)!=0))break;}while(DateTime.UtcNow<deadline);
   return "rate="+rate+" bits="+bits+" outputDone="+((o.flags&1)!=0)+" inputDone="+((i.flags&1)!=0)+" inputBytes="+i.recorded+" expected="+bytes;
  }finally{
   if(ih!=IntPtr.Zero)waveInReset(ih);if(oh!=IntPtr.Zero)waveOutReset(oh);
   if(preparedIn)waveInUnprepareHeader(ih,ip,size);if(preparedOut)waveOutUnprepareHeader(oh,op,size);
   if(ih!=IntPtr.Zero)waveInClose(ih);if(oh!=IntPtr.Zero)waveOutClose(oh);
   foreach(IntPtr p in new IntPtr[]{ob,ib,op,ip})if(p!=IntPtr.Zero)Marshal.FreeHGlobal(p);
  }
 }
 }
}
