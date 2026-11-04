# Original Mbox research — 4 November 2026

Target: original Digidesign Mbox, not Mbox 2, Mini or third generation.

## Sources
- https://www.usb-audio.com/download/ — official v2.8.45 Windows 64-bit demo downloaded locally. Vendor lists Windows 11 and Mbox. Proprietary; not redistributed. Demo beeps every 30 seconds.
- https://www.usb-audio.com/support/ — vendor reports full-scale white noise on some Mbox1 setups.
- https://www.zamaudio.com/?p=953 — original Linux implementation notes; reports fixes merged in kernel 3.19, recommends firmware 0.22 for static.
- https://github.com/zamaudio/alsa-driver/tree/mbox1clean — downloaded historical source branch. GPL source; retain original license when reusing code.
- https://zachpoff.com/resources/digidesign-mbox-1-standalone-mod/ — standalone analogue preamp modification, not USB audio restoration.
- https://github.com/MeblIkea/MBOX1-Digidesign-driver — archived proprietary repack, not a new driver; not redistributed.
- https://github.com/ItsMeATaco/mbox2-mini-open-firmware — different hardware; do not flash to Mbox1.

## Linux source findings
sound/usb/quirks-table.h lines 2880–2944:
VID 0x0dba, PID 0x1000; interface 0 ignored; interface 1 altsetting 1.
Stereo S24_3BE (packed big-endian 24-bit), 44100 and 48000 Hz.
Playback endpoint 0x02 synchronous; capture endpoint 0x81 asynchronous.
Table maximum packet size 0x130 (304 bytes); actual descriptors must be inspected.
sound/usb/quirks.c create_fixed_dual_stream_quirk registers both streams, reads actual max packet size, sets interface to alt 0 and initializes pitch/sample rate.
This is evidence of Linux handling, not validation of Windows control-transfer sequences.

## Firmware
https://akmedia.digidesign.com/support/compressed/MboxFirmware22_33860.dmg returned 404.
https://www.zamaudio.com/mbox2/MboxFirmware22_33860.dmg failed TLS.
https://www.zamaudio.com/mbox2/mbox1-3.14.tar.gz failed TLS.
No firmware was flashed.

## Windows references
https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/usb-audio-class-system-driver--usbaudio-sys-
https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/custom-audio-driver-type-decision-tree
https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk

No verified complete Windows 11 rewrite was found. Local archives remain outside Git because of proprietary distribution terms and unreviewed third-party source licensing.
