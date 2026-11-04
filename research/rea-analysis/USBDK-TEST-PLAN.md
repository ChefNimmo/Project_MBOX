# NimBox hardware-test route with an existing signed USB transport

Prepared 2026-11-04 (Australia/Sydney). **Not installed or approved yet.**

## Why this route

The independent KMDF binary is unsigned and has no Windows/ASIO audio frontend. Accepting a warning
does not waive Windows x64 kernel code-integrity policy. Microsoft's test-signing route uses a
self-created development certificate and Test Mode, with possible Secure Boot implications. The
user's earlier no-boot-security-change constraint is retained until explicitly revised.

UsbDk is an Apache-2.0 generic USB transport. Its official release can potentially carry NimBox's
own user-mode rate controls, packet plans and audio conversion. This would avoid loading our new
kernel code and would not patch or use Ploytec's licensing logic. It is a feasibility test, not an
assurance of Windows 11 compatibility or a completed audio driver.

## Concrete package and checks

- Official release: daynix/UsbDk **v1.00-22**, published **2020-03-17**.
- MSI: `downloads/UsbDk_1.0.22_x64.msi`.
- SHA256: `91f6f695e1e13c656024e6d3b55620bf08d8835ef05ee0496935ba6bb62466a5`.
- MSI, helper DLL and kernel binary have valid Red Hat Authenticode signatures.
- Microsoft SignTool `verify /kp /v UsbDk.sys` succeeded with zero warnings/errors.
- The kernel uses an older cross-signed chain. Signature verification does not establish that current
  Windows policy, HVCI or vulnerable-driver blocking will permit it to load. Do not weaken protections
  if it is rejected.
- Administrative MSI extraction was checked first: only standard file/image actions were present;
  it extracted into the workspace without installing. No UsbDk service is currently registered.

## Prepared NimBox client

The compiled streaming client now supports:

```
mbox_stream.exe usbdk-list C:\absolute\UsbDkHelper.dll
mbox_stream.exe usbdk-silent C:\absolute\UsbDkHelper.dll 48000 1
```

`usbdk-list` only enumerates the matching original Mbox; no device is detached or configured.
The silent test requires exactly one matching VID/PID, full-speed operation, and the exact previously
observed 54-byte configuration before temporarily redirecting the device. It selects interface 1
alternate setting 1, performs SET+GET rate checks on both endpoints, and uses the existing queued
silent scheduler. Requests and their buffers stay alive until completion/cancellation is drained.
Closing the redirect handle requests return to Windows and the installed function driver.
No hider rules, firmware flash, tone, raw recording or driver-signing configuration change is made.

The new backend compiles and passes source static analysis. Existing simulation/core/adapter tests
pass. Loading the verified helper succeeded; enumeration returned Windows error 2 because UsbDk
is not installed. No UsbDk transfer or redirection has run on hardware. Upstream source confirms
explicit per-packet offsets and the control-setup/data buffer layout used by this client.

## Proposed installation and hardware validation

1. With approval, run the prepared `native/tools/Invoke-UsbDkSetup.ps1 -Action Install` as administrator.
   It rechecks MSI hash/signature, saves the existing USB class UpperFilters value and runs the
   official MSI with `/qn /norestart`, logging its result. It does not force a reboot.
2. Verify service/load status, relevant Code Integrity events and USB device health. Stop if policy
   rejects the driver; do not change boot, blocklist or memory-integrity settings.
3. Enumerate through the new client before redirecting anything. Run a one-second silent duplex
   test at 48 kHz, then verify that closing the test returns the Mbox to Ploytec. Extend only after
   checking packet statuses, rates and device recovery. No audible test until the listener is ready.
4. If this works, build playback/capture and ASIO integration on the independent user-mode path.
   Keep the custom KMDF backend as an alternative development path. Success here would not mean
   ASIO/Windows audio registration is already implemented.

## Scope and rollback

**This is a USB-class filter installation, not a change confined to the Mbox.** Upstream installer
code writes the USB setup class UpperFilters list and resets USB host controllers. Other USB
devices can disconnect, including external storage. Finish transfers before approving installation.
That broader effect is why approval is requested for this specific step.

The Ploytec packages stay installed. No manual registry filter replacement is planned. The official
MSI uninstall is prepared as `Invoke-UsbDkSetup.ps1 -Action Remove`; it removes UsbDk's registration
and resets controllers too. It can also require a restart. The saved prior filter value is diagnostic
evidence, not an instruction to overwrite the whole registry class later.

## Primary sources and local evidence

- https://github.com/daynix/UsbDk/releases/tag/v1.00-22
- https://github.com/daynix/UsbDk/blob/v1.00-22/Documentation/DriverSigning.txt
- https://github.com/daynix/UsbDk/blob/v1.00-22/UsbDkHelper/Installer.cpp
- https://github.com/daynix/UsbDk/blob/v1.00-22/UsbDk/Urb.cpp
- https://learn.microsoft.com/en-us/windows-hardware/drivers/install/the-testsigning-boot-configuration-option
- Local: `research/usbdk-release-identity.json`, extracted package and matching source under `work/`.
