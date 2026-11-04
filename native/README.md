# Independent original-Mbox replacement

Target: Windows 11 x64, original Digidesign Mbox VID 0DBA/PID 1000.
No proprietary driver binaries, patched licensing or demo timers form part of this implementation.

## Current implementation

`core/mbox_stream.h` implements bounded packet planning and PCM conversion without hardware access.
`tests/test_stream.c` checks exact sample totals over ten simulated seconds at 44.1/48 kHz,
capacity rejection without partial state changes, capture packet validity and signed PCM boundaries.
The planner's synchronous one-millisecond cadence follows the historical ALSA playback quirk;
the live device describes asynchronous endpoints, so this assumption still needs transport validation.

`kmdf/driver.c` builds an independent Windows x64 KMDF USB diagnostic transport. It checks the
device identity and exact pipe layout, selects interface 1 alternate setting 1, verifies endpoint
sample-rate controls by readback, and submits bounded eight-packet IN/OUT URBs with explicit offsets.
Requests have a two-second transfer timeout. Only one request per direction is accepted; a rate
change requires both directions idle. Administrators and SYSTEM can open its exclusive interface.

`kmdf/probe.c` builds a separate controller. With no arguments it only searches for our interface.
`mbox_probe.exe probe 48000` (or `44100`) would set the rate and request eight milliseconds of silent
playback and capture. It reports USB and packet statuses without saving microphone audio.
It does not install the driver or access the Ploytec interface.

This is a bounded diagnostic, not continuous low-latency streaming. A pipeline allowing multiple
pending URBs, robust stream lifecycle management and an audio frontend are still required.

## Reproduce the build

Install Microsoft VS 2022 C++ Build Tools, SDK 10.0.26100.0 and matching WDK 26100.6584.
Run `native\build.cmd` from a developer workstation. This builds and executes the core tests,
then builds `build/MboxResearch.sys` and `build/mbox_probe.exe`; it never installs them.
KMDF 1.15 targets the inbox Windows 10/11 framework. The experimental INF restricts installation
to Windows 11 x64 and the original Mbox hardware ID.

The November 4 build passed `/W4 /WX`, core tests and `InfVerif /w`. Source static analysis passed
with `/analyze /analyze:external- /external:W0` and kit directories marked external. An initial
analysis also emitted warnings within WDK inline pool-allocation helpers; these external kit
implementations are excluded by that setting. This is not Driver Verifier, HLK or a hardware test.

## Transport decision

The live endpoints advertise 304 maximum bytes per USB interval. Complete stereo PCM24 frames require
lengths divisible by six: normal playback payloads at these rates are 264/270 or 288 bytes.
Microsoft documents WinUSB's isochronous write packetisation at the endpoint maximum. That API must
not be assumed to provide the variable packet boundaries required here.
A kernel USB transport with explicit URB packet offsets is therefore the initial candidate.
This is a feasibility decision, not proof that all alternative transports are impossible.

## Remaining work before replacement

1. Obtain a supported driver-signing/test environment; the built `.sys` is unsigned.
2. Validate the bounded diagnostic's cancellation, removal and power lifecycle on a test system.
3. Validate silent USB transfers, rate control and captured data on hardware; add continuous queues.
4. Expose the independent transport to ASIO and/or the Windows audio subsystem.
5. Test channel mapping, signal levels, recovery, latency and longer-duration duplex streaming.
6. Produce an appropriately signed installable package. Existing Windows security stays enabled.

There is a compiled experimental `.sys`, but no signed, validated audio-driver replacement yet.
The working Ploytec baseline is still installed. Buying a legitimate Ploytec license is a separate path;
this source does not remove or suppress the proprietary demo beep.

References:
- https://learn.microsoft.com/en-us/windows/win32/api/winusb/nf-winusb-winusb_writeisochpipeasap
- https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/transfer-data-to-isochronous-endpoints
- https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk
- https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdfusb/nf-wdfusb-wdfusbtargetdevicecreateisochurb
- https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdfusb/nf-wdfusb-wdfusbtargetpipeformatrequestforurb
- https://learn.microsoft.com/en-us/windows-hardware/drivers/dashboard/driver-signing-offerings
