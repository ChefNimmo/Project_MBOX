# Native Windows endpoint development 01

2026-11-04, Australia/Sydney. User chose native NimBox Windows endpoints without an extra mixer.

The ASIO 0.1.0 package remains installed and working as previously tested; it still does not expose
Windows Sound endpoints. This work is a separate 0.2.0 **development** package, not an upgrade
declared ready for installation.

## Implemented source

- Microsoft SysVAD reference pinned to `1fd430c78971c31b624b0773bbea825d8b480d55`, MIT licence.
  Upstream source is retained separately, with a reproducible patch and prepared full source archive.
- Native WaveRT playback/capture pair named NimBox 1 USB audio device. Initial format is 48 kHz,
  stereo PCM, 24 valid bits in 32-bit samples. The Windows shared audio engine handles application
  mixing/conversion; no third-party mixer is required by this design.
- Restricted, exclusive-open kernel control interface for the own NimBox transport broker. The
  versioned buffered IOCTL contains no pointers. Sizes, sequence and reserved fields are checked.
- Broker uses the already tested USB engine and the installed signed UsbDk runtime. It transfers
  256 stereo frames per exchange, keeping USB active during idle while it runs. Native DMA positions
  and events follow exchanges driven by USB capture completions rather than a simulated QPC timer.
- Sample capture sine generator, render-to-file path, sample endpoint list, sideband startup and
  simulated hardware volume nodes are disabled/replaced. WaveRT stream lifetime, notification-list
  edits, stream states and DMA unmapping are synchronized with the broker access path.
- Kernel driver/INF, SCM-capable broker, exact Windows endpoint enumeration and silent WASAPI probe,
  shared core tests, reproducible build and unsigned development packaging scripts.

## Checks completed

- VS 2022 MSVC 14.44 with SDK/WDK 10.0.26100.0 and KMDF 1.35. Installed WDK lacks the Visual Studio
  MSBuild platform registration, so a local command-line build is used; no system toolchain changes.
- Both unchanged upstream reference and NimBox-adapted kernel compiled and linked. NimBox compiles
  with `/W4 /WX`, WDK headers external. Custom kernel control interface and broker run MSVC analysis.
- Source preparation/patch application and a fresh-source kernel rebuild were exercised.
- ABI exact-size/flag/sequence validation and 4,224 guarded DMA wrap cases pass.
- Microsoft INF verification passes. Inf2Cat signability checks pass without errors or warnings and
  generate an **unsigned** catalogue. Signability is not a signature or a kernel-load test.
- Broker's 15-second **USB-only**, silent hardware check: 2,812 callbacks; input/output frames both
  719,993; zero underruns/overflows/USB/bridge errors. No recording saved. This test bypassed the
  unloaded native driver and therefore does not validate Windows endpoints.
- Running the normal broker without the driver returns Windows error 2 before USB is claimed.

## Not yet tested / limitations

The native kernel driver has not been loaded. Endpoint creation, WASAPI playback/capture, Windows
volume/mute behaviour, EOS, timing accuracy, restart, Driver Verifier, USB unplug/reconnect, sleep,
SCM recovery and actual DAWs still need execution and debugging. Shared ASIO/broker ownership is
not implemented; the old ASIO frontend must not run while this broker owns USB. S/PDIF and 44.1 kHz
native endpoint negotiation remain later work. Physical line/phantom checks remain incomplete.

The unsigned development ZIP is distinct from the ASIO installer. Boot settings, certificate trust,
driver binding, service registration and Windows defaults have not been changed. Kernel execution
needs a separately agreed signing/testing route under the user's existing no-boot-change constraint.

## Primary references

- https://github.com/microsoft/Windows-driver-samples/tree/1fd430c78971c31b624b0773bbea825d8b480d55/audio/sysvad
- https://learn.microsoft.com/en-us/samples/microsoft/windows-driver-samples/sysvad-virtual-audio-device-driver-sample/
- https://learn.microsoft.com/en-us/windows-hardware/drivers/install-the-wdk-using-nuget
- https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ksmedia/ns-ksmedia-ksrtaudio_hwlatency
