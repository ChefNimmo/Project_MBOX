# NimBox 1 ASIO — experimental 0.1.0

Windows 11 x64, original Digidesign Mbox (USB VID 0DBA/PID 1000).
Independent project; not affiliated with or endorsed by Avid/Digidesign or Steinberg.
ASIO is a registered trademark of Steinberg Media Technologies GmbH.

Select **NimBox 1 USB audio device** in a 64-bit application's ASIO settings.
This package does not add a Windows Speakers/Line In endpoint or an x86 ASIO driver.
Use 48 kHz initially; 44.1 kHz is also exposed and tested separately. Buffer size is fixed at 256.

Input audio is muted for the first ten seconds after each ASIO engine start while the hardware
settles. Playback is available during warm-up. The driver performs no noise reduction, software
gain or recording. Use the hardware input selectors/gain, Mix knob and headphone controls.
Start with headphone volume low. Microphone phantom power is a physical hardware setting.

The first release uses a conservative output reserve. Reported latency is an estimate, not a
calibrated analogue-loopback measurement: 2816 output frames and approximately 640 input frames
at 48 kHz. This favours a stable first release over low-latency live monitoring. The hardware's
direct-monitor Mix control remains available. The control panel shows underrun/overflow/USB status.

The signed UsbDk 1.0.22 runtime supplies USB transport. If missing, setup installs the official
MSI, which can reset USB controllers and briefly disconnect other USB devices. Finish USB storage
transfers before installing. No boot-security setting is changed. Setup never installs the
unsigned kernel prototype, downloads firmware, or patches another vendor's driver.

NimBox retains exclusive ownership from its first start until the ASIO driver instance closes,
including Stop/Start and buffer reconfiguration; close the application/driver before another Mbox
client uses it. Uninstalling NimBox unregisters its own ASIO entry and removes its files. The shared
UsbDk runtime is retained; it has its own Apps/Programs uninstall entry.

This is an experimental, hardware-tested release. S/PDIF, Windows system-audio endpoints, x86 hosts,
phantom-powered condenser/line-mode quality, calibrated latency, and unplug/sleep recovery are not
certified by this release. Hardware microphone signal tests are not a distortion/isolation test.

## Build and source

The ASIO frontend and its linked NimBox core/transport components are distributed under GPLv3.
The user approved this licence choice. See COPYING and THIRD-PARTY-NOTICES.txt. The installer
includes the corresponding source archive. It contains no proprietary original-driver binaries.

Build with VS 2022 C++ Build Tools and Windows SDK 26100:

```
native\asio\build.cmd
```

The result is a user-mode `NimBoxAsio.dll` and diagnostic ASIO host. Copy the official matching
x64 `UsbDkHelper.dll` beside the DLL. No kernel-driver signing certificate is required for this
user-mode build using the separately signed transport. The installer itself is currently unsigned.

## Engineering notes

The USB worker owns both PCM FIFOs and invokes the host callback synchronously in 256-frame blocks;
the callback never shares those FIFO indices with another producer thread. Requests, packet-length
arrays and capture buffers live until cancellation is drained. Stop joins the worker before buffers
are disposed. Captured packet lengths feed the subsequent OUT schedule as implicit clock feedback,
avoiding nominal-rate FIFO drift without resampling. This differs from the older diagnostic's fixed
nominal cadence and needs continued validation across devices/firmware and long sessions.
