# Dynamic microphone input check — 4 November 2026

User connected an SM56-style XLR microphone to Input 1, selected Mic, kept phantom power off and spoke during a ten-second test. No audio retained.

Ploytec ASIO demo, 48000 Hz, 256-frame buffers, two inputs and two silent outputs active:
- 1861 callbacks; no callback exceptions/reset requests or sample-rate-change callbacks reported; stop status 0.
- Input 1 peak absolute signed-32 sample: 69105664, approximately -29.85 dBFS.
- Input 2 peak: 114176, approximately -85.49 dBFS.
- Input 1 peak is substantially above its earlier silent baseline of 137472.
- Nonzero sample counts are not proof of signal quality: analogue noise also produces nonzero samples.

Result: known microphone signal reaches software on Input 1 while duplex streaming runs. This does not establish distortion, frequency response, calibrated gain, input clipping margin over time or Input 2 microphone performance.
Next: move mic to Input 2 and repeat. No recorded voice samples were saved.
