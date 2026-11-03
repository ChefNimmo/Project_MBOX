using System;
namespace Mbox1 {
    // Independent reference implementation; no Linux source copied.
    public static class Pcm24 {
        public static byte[] SwapStereoEndian(byte[] pcm) {
            if (pcm == null) throw new ArgumentNullException("pcm");
            if (pcm.Length % 6 != 0)
                throw new ArgumentException("Expected complete stereo 24-bit frames.", "pcm");
            byte[] output = new byte[pcm.Length];
            for (int i = 0; i < pcm.Length; i += 3) {
                output[i] = pcm[i + 2];
                output[i + 1] = pcm[i + 1];
                output[i + 2] = pcm[i];
            }
            return output;
        }
    }
}
