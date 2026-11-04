using System;
namespace MboxResearch {
 public static class Stereo24 {
  // Per-channel little-endian PCM to interleaved big-endian wire samples.
  // No internal driver headers are added; USB framing is separate.
  public static byte[] Pack(byte[] left, byte[] right) {
   if(left==null||right==null||left.Length!=right.Length||left.Length%3!=0)
    throw new ArgumentException("Expected equal whole-sample channel buffers.");
   var result=new byte[checked(left.Length*2)];
   for(int source=0,dest=0;source<left.Length;source+=3,dest+=6){
    result[dest]=left[source+2];result[dest+1]=left[source+1];result[dest+2]=left[source];
    result[dest+3]=right[source+2];result[dest+4]=right[source+1];result[dest+5]=right[source];
   }
   return result;
  }
  public static byte[][] Unpack(byte[] frames) {
   if(frames==null||frames.Length%6!=0)throw new ArgumentException("Expected whole stereo frames.");
   var left=new byte[frames.Length/2];var right=new byte[left.Length];
   for(int source=0,dest=0;source<frames.Length;source+=6,dest+=3){
    left[dest]=frames[source+2];left[dest+1]=frames[source+1];left[dest+2]=frames[source];
    right[dest]=frames[source+5];right[dest+1]=frames[source+4];right[dest+2]=frames[source+3];
   }
   return new byte[][]{left,right};
  }
 }
}
