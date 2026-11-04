using System;
namespace MboxResearch {
    // Offline USB Audio 1.0 request serialization. Never opens hardware.
    public static class MboxControl {
        static void CheckEndpoint(byte endpoint) {
            if ((endpoint & 0x0f) == 0 || (endpoint & 0x70) != 0)
                throw new ArgumentOutOfRangeException("endpoint");
        }
        public static byte[] GetRate(byte endpoint) {
            CheckEndpoint(endpoint);
            return new byte[] {0xa2,0x81,0,1,endpoint,0,3,0};
        }
        public static byte[] SetRate(byte endpoint, int rate) {
            CheckEndpoint(endpoint);
            if (rate != 44100 && rate != 48000)
                throw new ArgumentOutOfRangeException("rate");
            return new byte[] {0x22,1,0,1,endpoint,0,3,0,
                (byte)(rate & 255),(byte)((rate >> 8) & 255),(byte)((rate >> 16) & 255)};
        }
        public static int DecodeRate(byte[] data) {
            if(data == null || data.Length != 3) throw new ArgumentException("Expected three rate bytes.");
            return data[0] | data[1] << 8 | data[2] << 16;
        }
    }
}
