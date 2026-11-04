# Windows system-audio endpoint gap

Date: 2026-11-04 (Australia/Sydney).

The user clarified that always-active Windows playback/recording devices are required, not only
ASIO access from a DAW. The 0.1.0 installer does **not** satisfy that requirement. Its ASIO-only
limitation was documented, but the readiness statement did not adequately distinguish this from a
replacement for the previous driver's normal Windows audio functionality.

Read-only checks after the report:

- The physical original Mbox is present and Started as USB Composite Device (`usb.inf`).
- UsbDk enumeration finds revision 0020 at full speed.
- The NimBox 1 USB audio device ASIO registration exists.
- No NimBox Windows playback/capture endpoint has been implemented. An ASIO client must open
  the device to configure/start the audio engine; installing/registering ASIO alone does not keep
  it active or expose it in Windows Sound Settings.

Do not treat an always-running silent ASIO test process as a solution: it would not create Windows
audio endpoints and would exclusively hold the physical device.

Two routes were presented for the user's preference, without installing anything:

1. Use an existing signed virtual-audio bridge/mixer to expose Windows playback/capture devices
   and connect them to NimBox ASIO. This adds third-party software/background routing. While that
   application owns NimBox, other clients normally need its virtual ASIO/routing rather than opening
   the physical NimBox ASIO instance concurrently. Package signatures, input patching, licensing,
   startup behaviour and installation/rollback must be checked before deployment.
2. Develop native NimBox Windows endpoints (WDM/WaveRT/appropriate kernel audio architecture)
   and a broker to share the physical USB transport. This requires substantial additional code,
   lifecycle validation and a supported kernel signing/testing route. The current ASIO DLL cannot
   simply be registered as a Windows Sound endpoint. Do not change boot security without an
   explicitly reviewed decision.

Sources consulted:
- https://learn.microsoft.com/en-us/samples/microsoft/windows-driver-samples/sysvad-virtual-audio-device-driver-sample/
- https://voicemeeter.com/user-guide-startup-the-first-steps-replace-the-windows-mixer-with-voicemeeter/
- https://voicemeeter.com/setup-a-mic-with-your-asio-audio-interface/
- https://vb-audio.com/Cable/index.htm

The user selected **native NimBox Windows endpoints, without an extra mixer**.
Development is now under `native/windows-endpoint`; see `NATIVE-ENDPOINT-01.md`.
No third-party bridge installation, default audio-device change, autostart registration,
native endpoint installation or driver/security-policy change was performed.
