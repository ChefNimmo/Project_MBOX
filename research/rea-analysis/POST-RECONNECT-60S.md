# Post-reconnect one-minute ASIO run
4 November 2026. Original Mbox, Ploytec demo 2.8.45, Windows 11.
After successful USB replug recovery, ran 60 seconds of two-input/two-output ASIO at 48000 Hz, 256-frame buffers. Output buffers contained digital silence; input audio discarded.
11236 callbacks, zero counted callback exceptions/reset requests, zero rate-change callbacks, stop status 0.
Input peaks were identical at 134744064 on both channels. These metadata and callback counters do not measure analogue distortion, crosstalk, clock drift, bus-level dropped packets or round-trip latency. Demo processing may affect signal observations.
Result: one-minute software duplex continuation after closed-stream reconnect completed; not a long-term stability certification.
Raw local metadata log: asio-60s-post-reconnect.txt.
