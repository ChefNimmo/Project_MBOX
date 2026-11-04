# NimBox 1 USB audio device

Target: Windows 11 x64, original Digidesign Mbox VID 0DBA/PID 1000.
NimBox is an independent compatibility project; it is not affiliated with or endorsed by Avid or
Digidesign. Original product names identify supported hardware and historical research sources.
The experimental INF uses the display name "NimBox 1 USB audio device". Internal research filenames,
hardware identifiers, third-party notices and historical evidence are retained.
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
Requests have a two-second transfer timeout. Up to four requests per direction are accepted;
sample-rate changes require both directions idle. The admission transitions are shared with
`core/mbox_gate.h` and exhaustively checked across all 25 valid duplex count states. A power-stop
callback requests cancellation, and resume invalidates the cached rate so SET/GET is required again.
Administrators and SYSTEM can open its exclusive interface.

`kmdf/probe.c` builds a separate controller. With no arguments it only searches for our interface.
`mbox_probe.exe probe 48000` (or `44100`) would set the rate and request eight milliseconds of silent
playback and capture. It reports USB and packet statuses without saving microphone audio.
It does not install the driver or access the Ploytec interface.

`kmdf/stream.c` adds a queued silent-stream diagnostic. It keeps up to four eight-frame requests
pending in each direction (32 ms per direction), maintains fractional packet cadence across OUT
buffers, validates capture packet bounds/status and computes per-channel peaks without recordings.
`mbox_stream.exe silent 48000 10` would run a ten-second diagnostic on the **custom** device only;
Ctrl+C cancels and drains outstanding requests. Playback is always zero-filled. There is no tone.
A five-second interval without completions causes cancellation; pending buffers are retained until
their completion/ownership returns. Kernel transfer timeouts still apply.

`mbox_stream.exe --self-test` uses the **same client scheduler** with a deterministic fake backend;
it never enumerates or opens the Mbox. It checks ten virtual minutes at each sample rate, known
stereo sample preservation, varied completion ordering and eight fault/cancellation cases per rate.
The fake backend does not execute the kernel driver or emulate USB hardware/Windows scheduling.

`core/mbox_audio_fifo.h` now adapts planar 256-frame host blocks to variable USB packets and back.
It is used by the streaming client for silent output and capture peak processing. Writes reject
overflow atomically; reads zero-fill underruns and report the consumed frame count. It allocates
nothing during operation. This adapter has a single owner; an eventual ASIO callback thread must
add a deliberate synchronisation/ownership boundary before sharing it with the USB thread.

Continuous queued requests and a block adapter are now implemented for the silent diagnostic, but
live timing, kernel lifecycle and recovery still need validation. An ASIO/Windows audio frontend
and its thread boundary are still required before the project can replace an audio driver.

## Reproduce the build

Install Microsoft VS 2022 C++ Build Tools, SDK 10.0.26100.0 and matching WDK 26100.6584.
Run `native\build.cmd` from a developer workstation. This builds and executes the core tests,
then builds `build/MboxResearch.sys`, `build/mbox_probe.exe` and `build/mbox_stream.exe`, runs
admission-state and simulated-pipeline checks, and validates the INF; it never installs anything.
The audio-adapter test round-trips changing signed stereo data through 256-frame blocks and
44/45/48-frame USB packets over ten virtual seconds per rate, including FIFO wraparound, full/empty
states, malformed packets and zero-filled underruns.
KMDF 1.15 targets the inbox Windows 10/11 framework. The experimental INF restricts installation
to Windows 11 x64 and the original Mbox hardware ID.

The November 4 build passed `/W4 /WX`, core tests and `InfVerif /w`. Source static analysis passed
with `/analyze /analyze:external- /external:W0` and kit directories marked external. An initial
analysis also emitted warnings within WDK inline pool-allocation helpers; these external kit
implementations are excluded by that setting. This is not Driver Verifier, HLK or a hardware test.
The streaming client also passes source static analysis with angle-bracket system headers external.

## Transport decision

The live endpoints advertise 304 maximum bytes per USB interval. Complete stereo PCM24 frames require
lengths divisible by six: normal playback payloads at these rates are 264/270 or 288 bytes.
Microsoft documents WinUSB's isochronous write packetisation at the endpoint maximum. That API must
not be assumed to provide the variable packet boundaries required here.
A kernel USB transport with explicit URB packet offsets is therefore the initial candidate.
This is a feasibility decision, not proof that all alternative transports are impossible.

## Remaining work before replacement

1. Obtain a supported driver-signing/test environment; the built `.sys` is unsigned.
2. Validate the queued diagnostic's kernel cancellation, removal and power lifecycle on a test system.
3. Validate silent USB transfers, sample cadence, rate control and captured data on hardware.
4. Expose the independent transport to ASIO and/or the Windows audio subsystem.
5. Test channel mapping, signal levels, recovery, latency and longer-duration duplex streaming.
6. Produce an appropriately signed installable package. Existing Windows security stays enabled.

There is a compiled experimental `.sys`, but no signed, validated audio-driver replacement yet.
The user has chosen continued development on this PC without changing boot security. The unsigned
driver is not loaded and the existing driver binding is retained; no boot-security change is planned.
The working Ploytec baseline is still installed. Buying a legitimate Ploytec license is a separate path;
this source does not remove or suppress the proprietary demo beep.

References:
- https://learn.microsoft.com/en-us/windows/win32/api/winusb/nf-winusb-winusb_writeisochpipeasap
- https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/transfer-data-to-isochronous-endpoints
- https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk
- https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdfusb/nf-wdfusb-wdfusbtargetdevicecreateisochurb
- https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdfusb/nf-wdfusb-wdfusbtargetpipeformatrequestforurb
- https://learn.microsoft.com/en-us/windows-hardware/drivers/dashboard/driver-signing-offerings
