# Both analogue microphone inputs verified
4 November 2026. Original Mbox, installed Ploytec 2.8.45 demo on Windows 11.

User moved the same dynamic XLR microphone from Input 1 to Input 2, selected Mic and kept phantom power off.
Ten seconds at 48000 Hz, 256-frame ASIO buffers, simultaneous two-channel capture and silent stereo playback:
- 1860 callbacks, zero counted callback exceptions/reset requests/rate-change callbacks, stop status 0.
- Input 1 peak 161024 signed-32 units, approximately -82.50 dBFS.
- Input 2 peak 115383040, approximately -25.40 dBFS.
Earlier Input 1 microphone test produced a peak of -29.85 dBFS on channel 1 and -85.49 dBFS on channel 2.
This verifies both microphone paths and channel separation at a signal-presence level. It is not a calibrated crosstalk, noise-floor, distortion or frequency-response measurement.
Audio samples were discarded; no voice recordings saved.

Verified baseline now includes audible stereo headphone playback, known-signal microphone capture on both analogue inputs, and simultaneous ASIO input/output callbacks at 44.1/48 kHz.
Still unqualified: physical line outputs, phantom-powered condenser operation, analogue inserts, long-duration/dropout/latency performance, reconnect/sleep behaviour and S/PDIF.
Ploytec remains a demo with periodic beeps. The custom replacement driver remains research/prototype code.
