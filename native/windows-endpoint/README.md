# NimBox native Windows endpoints — development snapshot

This is the new **native Windows audio** component, separate from the tested ASIO 0.1.0
installer. It is not installed, hardware-tested in kernel mode, production-signed, or ready for general use.

The target Windows playback and capture names are both **NimBox 1 USB audio device**.
The first implementation supports stereo, 48 kHz, PCM with 24 valid bits in a 32-bit container.
Windows shared audio can perform application format conversion. 44.1 kHz native negotiation,
S/PDIF, shared ASIO/broker access, reconnect and sleep recovery are subsequent work.

## Architecture

Windows apps -> Windows audio engine -> NimBox WaveRT miniport -> private IOCTL ->
NimBoxAudioBroker -> tested UsbDk transport -> original Mbox USB device.

The broker is our own transport service, not a third-party mixer. Its transport remains active
while running, including during silent idle. The Windows audio engine supplies ordinary shared
application mixing. The existing ASIO frontend currently owns USB directly, so it must not be
opened concurrently with this broker until shared access is implemented.

The driver adapts Microsoft's MIT-licensed SysVAD source pinned at
`1fd430c78971c31b624b0773bbea825d8b480d55`. The checked-in patch selects one render/capture
pair, restricts formats, disables offload/driver loopback instances and sample sideband startup,
removes sample sine generation/file recording, and uses USB exchange progress for DMA positions
and notification events. Unused upstream declarations remain in this first adaptation.
Hardware gain stays on the physical knobs. We do not advertise the sample's simulated hardware
volume/mute nodes. Latency values are conservative transport estimates, not calibrated results.

The private control device permits SYSTEM/administrators only, allows one open handle, uses a
fixed-size pointer-free buffered IOCTL and checks version, lengths, flags and sequence. Stream
lifetime, notification lists, DMA access and stream-state changes are synchronized. The broker
exits before touching USB if the kernel bridge cannot be opened.

## Rebuild

Prerequisites: VS 2022 Build Tools with MSVC x64; Windows SDK/WDK 10.0.26100.0 and KMDF 1.35;
Git; PowerShell 7. No Visual Studio IDE/MSBuild driver extension is needed for this local build.

```powershell
./Prepare-Source.ps1
./Build-Reference.ps1 -NimBox
./build-broker.cmd
./Build-Development-Package.ps1
```

`Prepare-Source.ps1` fetches the pinned public Microsoft source when absent, copies it to
`sysvad/` and applies `NimBox-sysvad.patch`. It refuses to overwrite an existing destination.
`Build-Reference.ps1` without `-NimBox` builds the unchanged upstream reference only.
The kernel build uses `/W4 /WX`, with WDK headers treated as external; the custom control
interface and broker also receive MSVC code analysis.

Outputs: `../build/windows-endpoint/NimBoxAudio.sys`, `NimBoxAudioBroker.exe`, core tests,
and a development ZIP under `../../dist`. The INF targets x64 Windows 11 build 26100 or later
because this build uses KMDF 1.35. Neither build script installs drivers/services, creates a
root device, trusts certificates, or changes boot policy.

## Verification and remaining work

- Kernel compiles/links; INF validation and unsigned catalogue generation are build checks.
- Core test exercises exact ABI validation and 4,224 guarded DMA ring wrap cases.
- Broker's separate silent USB-only test passed 15 seconds: 719,993 input/output frames,
  zero underruns, overflows and USB errors. No audio files were saved.
- **No native Windows endpoint playback/capture has been tested yet.** Kernel loading,
  endpoint enumeration, Windows Sound controls, WASAPI duplex, EOS, restart, service recovery,
  Driver Verifier, unplug/sleep and real DAWs still require execution and debugging.
- The 10-second input startup mute remains in the shared USB engine.
- An unsigned catalogue does not permit kernel loading under normal signing enforcement.
  A supported signing/testing route must be agreed before installation. Current boot policy
  has not been changed. The working ASIO installation remains the rollback option.

Own additions: GPL-3.0-only, see `../COPYING`. Microsoft source/patch context retains its MIT
licence, reproduced in `MICROSOFT-LICENSE.txt`. UsbDk headers retain Apache-2.0 notices.
The source archive includes our modifications and transport dependencies; the upstream source
is also included in prepared form. `Prepare-Source.ps1` can independently reconstruct it at
the pinned commit into a new directory. For an extracted source archive, skip that preparation
step and run `Build-Reference.ps1 -NimBox` directly.

`NimBoxWindowsProbe.exe` enumerates exactly one active NimBox playback/capture pair. Once the
native driver and broker have been installed through an agreed test route, `--silent 15` checks
WASAPI shared duplex using silent output and capture frame counters only. It never saves audio.

`Prepare-Test-Signature.ps1` creates a separate locally test-signed ZIP, public test certificate
and signature metadata. It signs the kernel binary, regenerates/signs the catalogue and checks
the catalogue's cryptographic signature without trusting its chain. It does **not** install the
certificate, driver or service, or change boot policy. The 90-day development certificate is not
Microsoft production signing. Private keys stay in an ACL-restricted `work/private-nimbox-signing-*`
directory, outside the source/package. Their password is protected with current-user DPAPI.
