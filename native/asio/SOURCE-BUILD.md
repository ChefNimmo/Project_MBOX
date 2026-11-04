# Rebuilding NimBox 1 ASIO 0.1.0

The corresponding source archive is included in the installer under `Source` and is also supplied
beside the installer. NimBox ASIO and its linked NimBox core/transport source are GPLv3. See COPYING,
the ASIO SDK's retained LICENSE.txt and the UsbDk Apache licence/notices.

Requirements: Windows x64, Microsoft Visual Studio 2022 C++ Build Tools, Windows SDK 10.0.26100.0.
`native/asio/build.cmd` assumes the default VS 2022 BuildTools installation path. It compiles the
DLL and its diagnostic host with static CRT linking; it does not need a kernel WDK, test mode,
code-signing certificate or a proprietary ASIO licence. Keep the SDK and source notices intact.

The public API headers are from Steinberg's official ASIO SDK 2.3.4 distribution:
https://download.steinberg.net/sdk_downloads/ASIO-SDK_2.3.4_2025-10-15.zip
They are used under its GPLv3 option. ASIO is a registered trademark of Steinberg Media Technologies GmbH.

The runtime dependency is the unmodified signed UsbDk 1.0.22 x64 release:
https://github.com/daynix/UsbDk/releases/download/v1.00-22/UsbDk_1.0.22_x64.msi
SHA256: 91f6f695e1e13c656024e6d3b55620bf08d8835ef05ee0496935ba6bb62466a5
Install that official runtime or administratively extract its x64 helper. Copy `UsbDkHelper.dll`
beside the built `NimBoxAsio.dll`. Never replace or self-sign the bundled UsbDk kernel binary.

To rebuild the installer, obtain the official Inno Setup 7 compiler, verify its publisher, and
configure `native/installer/Build-Package.ps1` to point to it. Place the official MSI under the
analysis root's `downloads` folder. The script verifies its hash and the helper signature, builds
the source archive, then compiles `NimBox.iss`. Inno's compiler licensing should be reviewed for
the distributor's intended use; this package was built as a non-commercial project.

The source archive intentionally excludes proprietary original drivers, firmware, recordings,
the unsigned experimental kernel prototype, toolchain binaries and build outputs.
