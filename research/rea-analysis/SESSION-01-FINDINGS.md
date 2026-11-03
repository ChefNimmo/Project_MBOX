# Driver investigation session 01 — 4 November 2026

## Outcome
REA MCP and Ghidra work in the restarted chat. Traced original-device identification and a third-party USB configuration path. No binary execution, driver installation, USB writes or firmware flashing performed.

## Targets
- Digidesign Dalwdm.sys 6.4.0.138, x86, SHA256 bd225015460af0dfb6b791618ed35a4e06841cbffe5b3c8516cfda6c23e95d44. Older mirrored candidate; not authenticated final release.
- Ploytec pgusbwdm.sys 2.8.45 x64, SHA256 b4b5196138eb03d7da8bc7388617e00b09e5843fd89ff05ccad143722ec38dff. Alternative multi-device driver, not original Avid code.
Addresses are specific to these exact images.

## Original Digidesign findings
| Address | Observation | Interpretation / limit |
|---|---|---|
| 0x10002340 | Writes the same handler address into 0x1c consecutive pointers | Consistent with initializing the Windows driver dispatch table; not audio protocol recovery |
| 0x100046a0 | Calls 0x10003fb0, then compares returned identification text against hardware-ID strings | Device classifier candidate |
| 0x100bf3e4 | String USB\\Vid_0dba&Pid_1000, referenced at 0x100046d0 | Original Mbox normal identity |
| 0x100bf3b4 | String USB\\Vid_0dba&Pid_1001, referenced at 0x100046f9 | Firmware mode identity |
| 0x100bf3cc | String USB\\Vid_ffff&Pid_fffe | Additional classifier identity; exact role unconfirmed |
| 0x100046df | On a matching first comparison path, returns 0x16 | Internal normal-Mbox classification candidate |
| 0x10004708 | Match on either firmware comparison path returns 0x18 | Internal firmware-mode classification candidate |
| 0x10003fb0 | Calls IoGetDeviceProperty and converts returned wide-character text to byte text | Hardware-ID retrieval candidate; recovered function signature is incomplete |
| 0x10001a40 | Loops 256 x 32 x 8 with a callback | Appears to be PCI bus/device/function enumeration; do not confuse with Mbox USB setup |

The classifier assembly loads a comparison function through 0x100bf120. Exact function identity should be checked before reproducing case/matching semantics.
Ghidra pseudocode incorrectly eliminated several classifier branches as unreachable. Assembly still contains them. This is a concrete reason not to blindly translate decompiler output. 0x10004820 is a label written by DriverEntry, not a recovered function entry in this analysis; function decompilation at that address was rejected. Inspect instruction flow/boundaries before treating it as a callable recovered function.

## Ploytec comparison findings
| Address | Observation |
|---|---|
| 0xf10218a0 | References USBD_ParseConfigurationDescriptorEx and USBD_CreateConfigurationRequestEx; prepares interface entries, submits configuration through 0xf10087d0 and stores returned pipe handles |
| 0xf1021abf | Observed call site to descriptor parser |
| 0xf1021b79 | Observed call site to configuration request constructor |
| 0xf10087d0 | Builds a device-control request with numeric code 0x220003, calls lower driver, waits when pending and contains cancellation paths |
| 0xf10125d0 / 0xf1012620 | Dispatch into multi-device predicates; inspected predicates contain non-Mbox vendor/product IDs |

Do not attribute every branch of this generic driver to Mbox. No Mbox-specific request/value/index/payload tuple recovered yet. Constructor argument counts in pseudocode may be incomplete; compare assembly before implementation.

## Preserved local evidence
Under C:\Users\chefn\Documents\Codex\Mbox-Analysis\research:
- original-driver-session-01.json: 21 retained Evidence records, 429708 bytes.
- original-driver-session-01.snapshot.json: resumable query snapshot.
- ploytec-usb-session-01.json: 13 retained Evidence records, 237144 bytes.
- ploytec-usb-session-01.snapshot.json: resumable query snapshot.

Snapshots retain REA query results; they are not a full saved mutable Ghidra database. Both active binary sessions were closed after export.

## Next focused work
1. Follow original-driver classifier call references to per-device construction and dispatch.
2. Inspect the label at 0x10004820 through instruction decoding and enclosing control flow, without assuming a function boundary.
3. Identify Mbox-specific Ploytec vendor/product predicates, then their rate and digital-input paths.
4. Recover transfer builders and compare numeric requests with original-Mbox ALSA initialization.
5. Validate against read-only USB descriptors and recorded working-driver traffic before sending commands.
Still unresolved: final 8.0.1-era driver package/companions, firmware revision, full control protocol and hardware stability.
