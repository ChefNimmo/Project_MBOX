# NimBox 1 ASIO 0.1.0 and installer

Date: 2026-11-04 (Australia/Sydney). Experimental Windows 11 AMD64 release.
The user approved GPLv3 for the ASIO frontend and distribution of its corresponding source.

## Implemented and packaged

- User-mode ASIO COM DLL named **NimBox 1 USB audio device**, unique CLSID
  `{90AB4C67-2788-4148-A3D0-69A217C23EA5}`. Two analogue inputs/two analogue outputs,
  44.1/48 kHz, fixed 256-frame double buffers, PCM32 host samples with packed PCM24 USB conversion.
- Legacy bufferSwitch and ASIOTime callbacks, monotonic sample positions, internal clock reporting,
  channel metadata and a basic status control panel. Unsupported modes return explicit errors.
- One worker owns the audio FIFOs, invokes host processing synchronously and uses captured packet
  lengths as subsequent OUT clock feedback. No resampling or silent sample dropping is used.
  A conservative initial silent output reserve avoids the measured startup underrun.
- Inputs are explicitly muted for the first ten seconds after an engine start, accounting for the
  previously measured capture-settling transient. No DC/noise correction filter is applied.
- Stop drains/cancels requests and joins the worker before buffer disposal. The USB claim is retained
  through Stop/Start and buffer reconfiguration until the ASIO object closes, avoiding asynchronous
  Windows re-enumeration races. Stream failures request an ASIO reset. Unplug/sleep recovery has
  not been hardware-certified. Static CRT thread notifications remain enabled and workers use
  `_beginthreadex` for runtime lifecycle management.
- Inno Setup installer bundles the verified official signed UsbDk 1.0.22 MSI/helper, NimBox ASIO DLL,
  diagnostic ASIO host, licence/notices and corresponding-source ZIP. Existing matching UsbDk is
  reused; incompatible installed versions produce an error rather than silent replacement.
  New runtime installs verify the embedded MSI hash and do not force a reboot.
- Installer targets native x64 Windows 11, not ARM64 emulation. It registers only NimBox's ASIO/COM
  keys. Uninstall removes NimBox and leaves the shared UsbDk runtime installed separately.

## Validation performed

All ASIO builds use MSVC 14.44, Windows SDK 26100, static CRT, `/W4 /WX` and source static analysis.
The included corresponding source archive was extracted to a fresh workspace directory and built
successfully, without relying on files outside that archive except the documented toolchain.

The diagnostic host checks COM creation/unload, metadata, unsupported rates, invalid/duplicate buffer
requests, correct sample position stepping, live callbacks and transport counters. It saves only
numerical statistics, never microphone waveforms.

| Check | Result |
|---|---|
| Initial ASIO startup | Exposed 127 underrun frames; corrected by conservative startup reserve |
| 48 kHz, 60 seconds, time-info callbacks | 11,249 callbacks; IN/OUT 2,879,971/2,879,971; zero bad callbacks, underruns, overflows or USB errors |
| 44.1 kHz, 15 seconds, legacy callbacks | 2,585 callbacks; IN/OUT 661,842/661,842; zero error/xrun counters |
| Initial immediate restart | Failed due to device re-enumeration; corrected by retaining the USB claim through Stop/Start |
| Corrected 48 kHz run plus immediate restart | Both generations passed, zero error/xrun counters |
| Installed COM registration | CoCreateInstance succeeded; installed DLL streamed with zero error/xrun counters |
| Quiet ASIO tone from installed build | User confirmed clean sound in both headphone channels; restart also emitted a brief repeat |
| Installer, uninstaller, reinstall | All exited 0; uninstall removed NimBox ASIO/COM entry and DLL; UsbDk stayed RUNNING; reinstall registered NimBox again |
| Final static-CRT lifecycle correction | Rebuilt with runtime-compatible thread creation/notifications; stop/restart retest passed |
| Frozen release installation | Exit 0, no reboot; final installed binary checked against build |

The final release was also checked with a registered, silent 48 kHz stream after the final lifecycle
correction, including post-warm-up input and immediate restart. Raw numerical logs accompany this
report in `nimbox-asio-package-01-metrics.json`. The 60-second and 44.1 kHz results above precede
the lifecycle-only refinements; the same feedback/reserve algorithm is retained.

The tested host here is the supplied ASIO diagnostic host, not a commercial DAW. General DAW
compatibility, long sessions, active-stream removal, sleep, line/phantom quality and calibrated
analogue round-trip latency remain to be validated. Latency reports are estimates: 2816 output
frames and about 640 input frames at 48 kHz. These are conservative, not low-latency claims.
The ten-second input mute is a visible first-release limitation.

## Delivered artifacts and licensing

`dist/NimBox-1-ASIO-0.1.0-x64-setup.exe` and
`dist/NimBox-1-ASIO-0.1.0-source.zip`; checksums are recorded in the versioned manifest.
The source archive is also installed under `Program Files/NimBox 1/Source`.

The installer/ASIO DLL are unsigned user-mode software. UsbDk is the existing official signed kernel
transport. No test mode, Secure Boot change, code-integrity exception, certificate trust change,
firmware update, or proprietary demo patch was used. Ploytec remains removed.

ASIO SDK headers are from Steinberg's official 2.3.4 distribution and used under GPLv3. Original
licence/copyright notices are retained. UsbDk remains Apache-2.0. The installer compiler is Inno
Setup 7.1.1 used as a non-commercial project; compiler licensing should be reviewed for any future
commercial build use. This release does not imply Avid/Digidesign/Steinberg endorsement.

Primary sources:
- https://download.steinberg.net/sdk_downloads/ASIO-SDK_2.3.4_2025-10-15.zip
- https://github.com/daynix/UsbDk/releases/tag/v1.00-22
- https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-disablethreadlibrarycalls
- https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/beginthread-beginthreadex
- https://jrsoftware.org/ishelp/topic_setup_architecturesallowed.htm
