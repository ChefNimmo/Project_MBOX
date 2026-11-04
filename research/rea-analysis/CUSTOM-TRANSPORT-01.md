# Independent Mbox transport: first native build

Date: 2026-11-04. Target: original Mbox USB VID 0DBA/PID 1000, Windows 11 x64.

## Changes

- Installed official, Microsoft-signed VS 2022 C++ Build Tools 17.14.44, SDK 26100 and WDK 26100.6584.
  User approved both administrator prompts. Both installers completed with exit code 0; no reboot.
- Added independent stereo24 packet planning/conversion and tests under `native/core` and `native/tests`.
- Built `native/build/MboxResearch.sys`, an experimental KMDF 1.15 USB diagnostic transport.
- Added an INF and `mbox_probe.exe` controller that performs a bounded silent duplex diagnostic
  only when explicitly requested. Default invocation only enumerates our custom interface.
- No commercial binary was patched, no licensing mechanism modified and no driver binding changed.

## Implemented transport behaviour

Exact original-Mbox identity/interface/endpoint validation, interface 1 alternate setting 1 selection,
endpoint class sample-rate SET and GET/readback on 0x02 and 0x81, eight-packet URBs with explicit OUT
packet offsets and maximum-sized IN slots, per-direction admission, concurrent IN/OUT capability,
control/transfer exclusion and request timeouts. WDF owns requests forwarded to its USB targets.
The experimental access policy admits Administrators/SYSTEM with one open handle.

The user-mode controller sends eight milliseconds of silence while requesting capture and prints
per-packet statuses. No audio recordings or waveform data are written to disk.

## Verification actually performed

1. Native core tests executed successfully: exact sample totals over ten simulated seconds at both
   44100 and 48000 Hz; complete stereo frames; atomic insufficient-capacity rejection; rejection of
   malformed capture packet lengths/status; signed PCM boundary conversion.
2. Native driver and controller compiled with MSVC 14.44, `/W4 /WX`. Kit-specific anonymous-layout
   warnings 4201/4324 are disabled for the kernel compile. Static unused helper warning 4505 is
   disabled for the controller because it includes core helpers it does not call.
3. Driver source static analysis completed with no source diagnostics using `/analyze`, external
   kit headers excluded via `/analyze:external- /external:W0 /external:I`. An initial unrestricted
   analysis reported WDK inline allocation-helper warnings; those are not a source-code finding.
4. Microsoft's `InfVerif /w /v`: `INF is VALID`.
5. PE inspection: x64 native image, `FxDriverEntry`, imports WDFLDR.SYS and ntoskrnl.exe; NX,
   relocation and integrity-check flags present. Authenticode status: **NotSigned**.
6. Default controller run: **No independent Mbox research transport is currently installed/present.**

No custom USB transfer, rate change, sound output, microphone capture, cancellation or power/removal
behaviour has been tested on hardware. Compiler/static-analysis success is not runtime validation.

## Remaining gaps and deployment barrier

The diagnostic deliberately accepts only one outstanding request per endpoint and does not yet
implement a continuous streaming pipeline, shared audio ring, ASIO frontend or Windows audio endpoint.
It cannot replace the installed driver for listening/recording yet. S/PDIF remains deferred.

The playback cadence follows a historical ALSA synchronous override although the live USB descriptor
reports asynchronous endpoint attributes. Hardware validation must resolve this timing assumption.
Rate controls are corroborated by original-driver analysis and ALSA, but this implementation has not
sent them to hardware. Additional analogue-source/initialisation controls may still be necessary.

The kernel binary is unsigned and there is no signed catalog/package. A supported Microsoft signing
route or a dedicated, appropriately configured driver-development test system is required before
loading. No boot/security settings were changed and the working Ploytec baseline remains installed.
The periodic demo beep remains present on that baseline; this work does not suppress it.

## Primary references

- [Microsoft explicit isochronous URBs](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/transfer-data-to-isochronous-endpoints)
- [KMDF isochronous URB allocation](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdfusb/nf-wdfusb-wdfusbtargetdevicecreateisochurb)
- [KMDF formatting and forwarding requests](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdfusb/nf-wdfusb-wdfusbtargetpipeformatrequestforurb)
- [Microsoft driver signing offerings](https://learn.microsoft.com/en-us/windows-hardware/drivers/dashboard/driver-signing-offerings)
- [WinUSB OUT packetisation](https://learn.microsoft.com/en-us/windows/win32/api/winusb/nf-winusb-winusb_writeisochpipeasap)

WinUSB's documented maximum-packet OUT packetisation is the reason to investigate explicit URBs.
It is not evidence that every user-mode alternative is impossible. UsbDk is one open-source generic
transport candidate for separate compatibility evaluation; it was not installed or tested here.
