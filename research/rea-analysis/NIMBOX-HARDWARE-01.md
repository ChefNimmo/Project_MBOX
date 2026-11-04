# NimBox independent hardware playback/capture and Ploytec removal

Date: 2026-11-04 (Australia/Sydney). Device: original Mbox VID 0DBA/PID 1000, revision 0020.

## Installed transport and removal

The user explicitly approved installation of official UsbDk 1.0.22 x64 and silent hardware tests,
including the USB host-controller reset. MSI hash and Red Hat signature were rechecked. The existing
USB class UpperFilters value was saved. Installation returned MSI exit 0 with no required reboot;
UsbDk's kernel service is RUNNING. No recent UsbDk Code Integrity rejection was found in the events
checked; actual service loading was successful. Boot security and script execution policies were
not changed. The separate custom KMDF NimBox binary remains unsigned and was never loaded.

Windows PowerShell initially blocked the helper script before installation. The identical reviewed
steps were then executed as direct elevated commands; no policy was weakened. Detailed console/MSI
logs are retained locally under `work/`.

After independent silent streaming and an audible stereo tone succeeded, the user explicitly asked
to uninstall Ploytec to remove the demo and eliminate ambiguity. Removed:

- Ploytec ASIO registration and COM class `{24BE3F40-4D2A-11D4-B580-00A0CC5BE45E}` via DLL unregister.
- Verified usb-audio.de driver packages `oem53.inf` and `oem51.inf` via PnPUtil uninstall/delete.
- Residual disabled/stopped Ploytec services `pgusbmme` and `pgusbwdm` after checking their identities.

PnPUtil no longer listed a usb-audio.de/Ploytec package, and the ASIO/COM registry entries were absent.
At rest, the physical device enumerates as Microsoft's USB Composite Device (`usb.inf`), Started.
UsbDk temporarily redirects the exact target for NimBox tests and returns it to Windows afterward.
Ploytec research/download copies are retained as evidence, not active installed components.

## Hardware results

All tests below selected interface 1 alt 1, used explicit packet lengths and verified SET/GET sample
rates on both endpoints. No USB/packet failures were reported by the client in these runs.
Only numerical statistics were saved; no microphone/voice waveform was written to disk.

| Test | Nominal OUT frames | Actual IN frames | Result |
|---|---:|---:|---|
| 48 kHz silent, 1 second | 48,000 | 47,999 | Success |
| 48 kHz silent, 10 seconds | 480,000 | 479,993 | Success |
| 44.1 kHz silent, 10 seconds | 441,000 | 440,992 | Success |
| 48 kHz tone, 5 seconds, before Ploytec removal | 240,000 | 239,996 | User heard both headphone channels |
| 48 kHz Input 2 dynamic mic, 10 seconds, after removal | 480,000 | 479,994 | Ch1 peak 34,859,520; ch2 194,876,416 |
| 48 kHz Input 1 dynamic mic, 10 seconds, after removal | 480,000 | 479,994 | Ch1 peak 113,209,088; ch2 48,341,760 |
| 48 kHz idle/windowed capture, 20 seconds, after removal | 960,000 | 959,989 | Success; startup transient measured |
| 48 kHz tone, 5 seconds, after Ploytec removal | 240,000 | 239,997 | User again heard both headphone channels |

Tone mode is explicitly selected, 750 Hz at -40 dBFS peak, with 20-ms fades and a five-second maximum.
The listener confirmed headphones at very low volume and the Mix knob toward Playback. Silent mode
still produces zero-filled output. Tone level/duration/phase tests were added and pass. The live
client now reports capture RMS, DC and peaks, with first-two-second and after-ten-second windows.
Simulated tests verify the frame windows and mean/square-mean statistics against known samples.

## Startup capture issue: characterised, not hidden

Unexpected levels in the unused channel of short mic tests prompted an idle-window investigation.
In the twenty-second run, measured first-two-second RMS was approximately -39.7/-39.8 dBFS. For the
portion **after ten seconds**, RMS was -79.42/-82.78 dBFS and DC was 0.000046049/0.000050021.
Both channels' early common offset/energy decayed strongly. This supports a startup-settling
explanation; it is not proof of a particular analogue or firmware cause.

The microphone tests show signal response on the selected channels, but short unprimed statistics
are not a reliable channel-isolation, distortion or noise-floor measurement. The frontend still
needs explicit startup handling and controlled recording-quality validation. No denoiser, DC filter,
sample alteration or source-selector write was introduced to conceal the transient.

Capture frame totals differ slightly from nominal OUT totals. That can reflect clock-domain/startup
behaviour and requires longer streaming and timing analysis. USB request completion alone does not
prove an absence of audible glitches or DAC FIFO under/overrun. The current Linux quirk corroborates
packed stereo big-endian PCM24 and the endpoint layout; its current table lists 48 kHz only, whereas
these Windows transport tests accepted and streamed at 44.1 kHz. Audible 44.1 kHz quality/frequency
has not been independently verified.

## What works and what remains

Established: signed generic transport installation; independent endpoint rate control and duplex USB
streaming; audible stereo headphone playback with Ploytec removed; microphone signal capture on
both input selections; release/reacquisition and return to the composite device at rest.

Not yet established: a selectable ASIO or Windows audio device, production startup/clock handling,
long-term dropout/latency behaviour, precise channel isolation, line-mode inputs/line outputs,
phantom-powered condenser operation, inserts, active-stream unplug/sleep recovery, or S/PDIF.
No new ASIO driver registration exists yet. The next major development component is a user-mode
ASIO frontend using the tested UsbDk backend. No EV certificate is needed merely to continue this
user-mode development on the already loaded official transport; distribution/licensing/signing
requirements must still be considered for the eventual chosen components.

Primary corroboration:
- https://github.com/daynix/UsbDk/tree/v1.00-22
- https://raw.githubusercontent.com/torvalds/linux/master/sound/usb/quirks-table.h
