# Live hardware inspection and implementation progress
4 November 2026, Australia/Sydney.

## Observed hardware (read-only)
Original Mbox enumerates as VID 0DBA / PID 1000, device revision 0020.
Parent USB composite device: status OK, usbccgp service and Microsoft's usb.inf.
Interface children MI_00 and MI_01: problem code 28, no installed interface driver.
No firmware or audio driver changed. No proprietary control requests or audio streams sent.

Device descriptor hex:
1201100100000008BA0D0010200001020001
This reports USB 1.10, EP0 maximum packet size 8, device revision 0x0020, one configuration and no serial-string index.

Configuration descriptor hex:
0902360002010080F00904000000FF0000000904010000FF0000000904010102FF000000090581053001010000090502053001010000

Decoded topology:
| Interface | Alternate | Class | Endpoints |
|---|---|---|---|
| 0 | 0 | FF vendor-specific | none |
| 1 | 0 | FF vendor-specific | none |
| 1 | 1 | FF vendor-specific | 81 IN and 02 OUT |

Both endpoint descriptors have attributes 05, maximum packet size 0x0130 (304 bytes), interval 1.
Attributes 05 decode to isochronous/asynchronous in the standard USB descriptor bitfields.
Do not confuse descriptor claims with proven operating synchronization: historical ALSA overrides may intentionally differ.
Configuration total length 54; configuration value 1; bus-powered declaration 0x80; bMaxPower 0xf0 (480 mA for USB 2-era units).
No class-specific AudioControl descriptors are present in this 54-byte configuration; unit/control meaning cannot be recovered from AudioControl descriptors here.
This explains why a compliant USB Audio class driver cannot simply infer audio topology from the raw descriptors.
It does not prove that descriptor rewriting alone would be sufficient.

## Firmware consideration
The 0x0020 reported revision is below the 0x0022 recommended in ZamAudio's original-Mbox white-noise investigation.
This is a comparison to historical advice, not proof of a present fault or permission to flash. No update performed.

## Comparison to original-driver analysis
Original driver requests interface 0/alt0 and interface 1/alt1, matching observed hardware.
Historical ALSA maxpacket 0x130 matches actual endpoint descriptors.
Original buffer stride 0x138 must remain distinct from that actual 0x130 maximum; it is not evidence of a 312-byte legal USB packet.

## Rate-control cross-check
Historical ALSA sound/usb/clock.c set_sample_rate_v1 lines 217 onward uses SET_CUR / GET_CUR, class endpoint recipient, value 0x0100, endpoint address index and three little-endian rate bytes. This matches fields recovered from original Digidesign helpers.
That source only executes rate control when its format attributes permit it; do not assume both endpoints accept every command.
No live rate request has been sent.

## Implementation work
windows/MboxControl.cs: tested offline control-request serialization.
windows/Stereo24.cs: added two-channel planar little-endian to interleaved big-endian packing and reverse unpacking, following original-driver byte-copy loops.
tests/Test-Stereo24.ps1 passed known positive/negative boundary vectors, channel placement, round trip, non-mutation and malformed-buffer rejection.
Still no Windows audio transport, AVStream registration, isochronous submission or driver package.

## Local artifacts
research/hardware-state-02.json — read-only PnP properties (contains local topology identifiers; not republished).
research/usb-descriptors-01.json — raw read from USB hub driver.
research/usb-descriptors-decoded-01.json — descriptor boundary parsing.
research/mbox-configuration-descriptor.bin — exact configuration bytes.
research/ReadUsbDescriptors.cs — read-only hub descriptor collector.
research/Collect-MboxDevice.ps1 — read-only PnP metadata collector.

The first descriptor-reader attempt incorrectly required 36 returned bytes; corrected to require only the 22 bytes needed for the fields actually examined, avoiding dependence on packed trailing fields. The successful read returned a complete internally consistent 54-byte configuration descriptor.

## Next constraints
Need an authenticated complete original-driver bundle/companions, source/clock flag semantics, and recorded working-driver traffic.
A full Windows 11 audio solution likely needs a custom audio transport/descriptor adaptation rather than a simple INF. Architecture choice must account for duplex timing, signing and user-facing WASAPI/ASIO requirements.
Astra is not currently necessary; observed hardware and protocol verification matter more than model escalation.
