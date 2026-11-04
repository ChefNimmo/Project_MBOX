# Approved native test setup — restart pending

2026-11-04, Australia/Sydney.

The user explicitly approved **Windows Test Mode and native-driver testing**, including trust of
the prepared NimBox test certificate and installation of the development driver. This supersedes
the earlier no-boot-change constraint for this controlled test. No automatic restart was approved
or performed.

Completed with administrator access:

- Verified test ZIP SHA256 `034AA82621FE4D5A6B9D0F0F2D7C46341CE2740BCF10A0BF1A248FE3A3163DD3`,
  exact signing-certificate identity and each driver-package file hash.
- Firmware Secure Boot readback False; BitLocker protection Off. Neither was changed.
- Saved the pre-change current BCD entry locally. It had no explicit testsigning value.
- Imported **only** `CN=NimBox Development Test`, thumbprint
  `56F7A692B142A98E597BAAE174825438D5E09065`, to LocalMachine Root and TrustedPublisher.
  Neither store previously contained that certificate. Kernel Authenticode then verified Valid.
- Set `testsigning on` for `{current}` and verified `testsigning Yes` readback.
- Staged `NimBoxAudio.inf` with `pnputil /add-driver` **without** `/install`; published as `oem51.inf`.
  OEM numbers are reused; this is now NimBox, not the old removed Ploytec package.

No root device was created, native kernel driver loaded, broker service installed or audio default
changed. `NimBoxAudioKernel` and `NimBoxAudio` services are absent (SC error 1060). The existing
NimBox ASIO registration remains present.

**Next:** the user restarts Windows manually. Verify effective Code Integrity/Test Mode state after
the restart, then perform the controlled first root-device installation, endpoint enumeration and
silent native WASAPI tests. Do not claim native audio works before those tests.

Local administrator result: `work/native-testmode-preparation.json`.
Recovery helper: `native/windows-endpoint/Rollback-Native-Test.ps1`, to be run elevated only if
rollback is requested. It targets native NimBox components and this exact certificate, restores the
previous absent testsigning option, and does not restart automatically. Keep the tested ASIO package
as the existing rollback audio route. Windows requires a restart for boot signing changes to apply:
https://learn.microsoft.com/en-us/windows-hardware/drivers/install/the-testsigning-boot-configuration-option
