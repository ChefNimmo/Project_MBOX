# First native kernel load and format correction

2026-11-04, Australia/Sydney. User restarted after approving Test Mode and native-driver testing.

## Observed results

- Runtime NtQuerySystemInformation(SystemCodeIntegrityInformation) returned flags 2621955;
  CODEINTEGRITY_OPTION_TESTSIGN was set. This verifies active Test Mode, rather than only a BCD setting.
- First `devcon install` of signed 0.2.0.0 returned 0. NimBoxAudioKernel service was RUNNING.
- Windows endpoint probe found exactly one active playback/capture pair:
  Speakers (NimBox 1 USB audio device), Microphone (NimBox 1 USB audio device).
  The root instance created by DevCon is `ROOT\MEDIA\0001`; `Root\NimBoxAudio` is its hardware ID.
- Own broker ran 45 seconds with the native control interface active: 8,437 callbacks;
  IN/OUT frames both 2,159,979; zero underruns, overflows, USB or bridge errors.
  No audible tone or waveform recording was generated/saved. This validates USB/control exchange;
  no WaveRT stream had successfully opened, so it does not validate Windows audio payloads.
- First silent WASAPI duplex attempt failed before streaming: render/capture IAudioClient activation
  succeeded, but render GetMixFormat returned `AUDCLNT_E_UNSUPPORTED_FORMAT` (`0x88890008`).
  Capture GetMixFormat was skipped by the failing-HRESULT chain (its printed status repeats that error).

## Correction prepared, not yet loaded

The inherited generic PortCls intersection cannot express 24 valid bits separately from the 32-bit
container. The corrected DataRangeIntersection returns the full supported WAVEFORMATEXTENSIBLE,
with bounds/type checks, stereo mask, 48 kHz and 24 valid bits. The INF also explicitly supplies both
endpoints' default OEM format using Microsoft's documented serialized PROPVARIANT representation.
These are fixes for the negotiation failure; success still requires a new native WASAPI test.

The first correction, 0.2.0.1, was installed by DevCon but returned 1 (restart required). A controlled
attempt to remove the NimBox root and stop its kernel service did not permit an immediate unload:
DevCon returned 'Removed on reboot'; SC stop returned 1052. Do not force unloading or create a second
root device while this removal is pending. The first driver's removal will finish at the next restart.
The control-device teardown/unload lifecycle needs further investigation; current updates may need
restarts. No PC restart was triggered by the agent.

Final prepared correction: **0.2.0.2**, under local `work/native-format-fix-02`:

- SYS SHA256 `2B571686DEC315E5D6B3E132D72F6849E1764195C033693FD0D9F9E0914F9F86`.
- INF SHA256 `EFEE15A64110424EAF3078E2EBA6B2708771AC2C71CC38B16D5B8FAE8902733A`.
- CAT SHA256 `50ED7DDC0A812ED1ED75B631EAC3FF103B3C1599AD78E33453A31EA58DBE83CE`.
- Same trusted test certificate `56F7A692B142A98E597BAAE174825438D5E09065`; no new certificate trust.
- Kernel build `/W4 /WX`, custom control code analysis, INF verification and catalogue generation pass.
- Core tests still pass 4,224 guarded wrap cases. Probe now prints each activation/format call status.

Final 0.2.0.2 package staging succeeded without `/install`, published as **oem59.inf**. Full-source,
unsigned development and separately test-signed dev2 ZIPs are in local `dist`. No public binary release
or finished installer is claimed. The previous installed root is still pending removal on reboot.

## Resume after the next manual restart

1. Verify runtime Test Mode and current NimBox root-device state. The old root is pending removal.
2. Verify final package hashes/signature. Install `work/native-format-fix-02/NimBoxAudio.inf` using
   DevCon install for `Root\NimBoxAudio` only if no matching hardware-ID device exists; otherwise update
   the existing device. Identify by hardware ID, not by a guessed root instance prefix/OEM number.
3. Enumerate native Windows endpoints and run the own broker, then `NimBoxWindowsProbe --silent 15`.
4. Debug remaining native audio errors. Do not install/start an automatic broker until first duplex
   functionality works; do not claim the device is usable as Windows system audio yet.
5. Continue service lifecycle/recovery, shared ASIO ownership, audible low-level verification and
   final installer work. Existing ASIO installation and UsbDk are retained.

Sources:
- https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntquerysysteminformation
- https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/pkey-audioengine-oemformat
