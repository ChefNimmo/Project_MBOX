# Windows implementation start

The first implemented component is an offline C# reference for converting stereo packed PCM between Windows little-endian and Mbox big-endian byte order. It does not access USB or register an audio endpoint. Run tests with:
powershell -NoProfile -File tests/Test-Pcm.ps1

Next:
1. Obtain read-only USB descriptors, firmware revision and installed-driver information on real hardware.
2. Confirm endpoint topology, 24-bit packing, sample-rate control and duplex timing with USB captures.
3. Build an AVStream USB audio transport with the WDK; implement cancellation, PnP, power, isochronous scheduling and buffer validation.
4. Connect tested format conversion; implement capture/playback clocks and rate switching.
5. Validate WASAPI shared/exclusive operation, analogue I/O, S/PDIF, latency, sleep/reconnect and extended duplex stability.
6. Assess ASIO separately; WASAPI support alone does not provide native ASIO.
7. Package and sign only after driver validation.

Do not bind WinUSB over the current audio driver merely to inspect the device.
No speculative INF or guessed USB writes are supplied.

Local assessment: git available; cl/MSBuild/CMake not on PATH; standard Visual Studio and Windows Kits directories not found. Hardware query failed due to CIM access restrictions. No hardware streaming or WDK build completed.
