# Working Ploytec baseline — analogue priority
4 November 2026. User prioritises analogue I/O and stability; S/PDIF deferred.

## Installed and verified
Official Ploytec demo 2.8.45 x64 files and catalogues passed Windows Authenticode verification.
Initial non-administrator staging failed with Access denied and changed nothing.
After Windows administrator consent, staged Pgusbwdm.inf as oem51.inf and PGUSBMM3.INF as oem53.inf.
Updated only hardware ID USB\\VID_0DBA&PID_1000 via UpdateDriverForPlugAndPlayDevices, flags 0.
Returned success, Win32 error 0, reboot not required.
Registered signed pgusbwdm_x64.dll from persistent tools/ploytec via regsvr32.
ASIO registry: usb-audio.de ASIO driver, CLSID {24BE3F40-4D2A-11D4-B580-00A0CC5BE45E}.
No Secure Boot, Memory Integrity, driver-signature enforcement or firmware settings changed.
Mbox USB node and MEDIA/PGUSBMM3ADAPTER both report OK.

## Actual tests
- Windows MME format queries accepted stereo 44.1/48 kHz at 16/24 bits.
- Separate 48 kHz/16-bit silent playback and input streams completed with expected input byte count.
- MME simultaneous output/input opens failed at input open with multimedia error 1. Do not claim MME duplex.
- ASIO initialises, reports 2 input + 2 output channels, 256-frame preferred buffer, type 18 (Int32LSB) for all four channels.
- ASIO 3-second duplex at 44100: 504 callbacks, no callback exception/reset-request count, stop status 0.
- ASIO 3-second duplex at 48000: 549 callbacks, same counters/status.
- ASIO 30 seconds at 48000: 5613 callbacks, no reported resets/callback errors or rate-change callbacks; device nodes remained OK.
- ASIO 30 seconds at 44100: 5154 callbacks, same counters/status.
- Rate switched back to 48000 for tone tests.
Callback counts are not an independent measurement of USB dropouts, clock accuracy, latency or uninterrupted analogue signal.
Managed probe is diagnostic software, not a production audio engine.
- Five-second 750 Hz tone, 5% full-scale amplitude, both outputs: user confirmed hearing both sides, very low level, after Mix was set toward Playback and headphones kept low.
Thus audible stereo headphone output is confirmed. Line outputs have not been physically verified.
- Five-second silent-output input baseline: 924 callbacks; input peaks 137472 / 114176 in signed 32-bit sample units, nonzero sample counts 227670 / 227406. This establishes sample activity only, not correct known-signal capture, channel separation or preamp quality.
No input samples saved; probe retained metadata only.

## Current limitations
This is an installed proprietary demo baseline, NOT our new driver. Vendor demo periodically beeps; no license purchased or shared unlock key used.
Analogue source/gain/phantom controls, known-signal input recording, line outputs, round-trip loopback, disconnect/reconnect, sleep/resume and long-duration stability still need qualification.
S/PDIF not tested.
Older hardware revision 0x0020 remains unchanged.

## User operation
For x64 DAWs choose the ASIO entry usb-audio.de ASIO driver; test starts used 256 frames at 44.1/48 kHz.
For computer playback in headphones set Mbox Mix toward Playback, then adjust headphone level carefully.
Windows endpoints currently show Speakers / Line In (AudioDevice on USB Bus 2.8.45).
No system default endpoint was intentionally changed by our code; Windows may automatically select newly installed endpoints.

## Preserved artifacts and recovery
Local logs in Documents/Codex/Mbox-Analysis/research: ploytec-admin-install.log, ploytec-install-result.json, wave-formats-01.txt, duplex-test-01.txt, single-direction-test-01.txt, asio-probe-01.txt, asio-duplex-01.txt, asio-duplex-30s-48k.txt, asio-duplex-30s-44k.txt, tone-test logs and asio-input-baseline-01.txt.
Probe sources: ProbeWave.cs, TestWaveDuplex.cs, ProbeAsio.cs, TestAsioDuplex.cs.
Driver-store package identifiers recorded for controlled rollback. Any removal must first confirm these identifiers still refer to the same packages and preserve other audio devices.
Do not remove tools/ploytec while its ASIO registration is active.
