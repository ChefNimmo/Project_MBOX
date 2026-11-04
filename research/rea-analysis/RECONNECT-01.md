# USB reconnect verification — 4 November 2026

User unplugged USB, waited five seconds and reconnected the original Mbox.
Read-only enumeration after reconnect:
- USB VID_0DBA PID_1000: OK, Ploytec commercial 2.8.45 description.
- MEDIA PGUSBMM3ADAPTER: OK.
No reinstall, reboot or security change required.

ASIO reopened successfully; 2 inputs and 2 outputs, 256-frame buffers, current rate 48000.
Ten-second 48 kHz silent-output duplex run:
1863 callbacks, zero counted callback exceptions/reset requests or rate-change callbacks, stop status 0.
Input sample activity continued. Peak values were equal on both channels in this run; without a controlled single-channel signal this does not establish crosstalk or audio quality. Demo processing may affect observations.
No input audio retained.

Result: unplug/replug recovery with streams CLOSED before unplugging is verified on this USB port.
Not tested: unplug during active streaming, another USB port, system sleep/resume or reboot persistence.
Ploytec remains a periodic-beep demo; this does not change the status of our replacement-driver prototypes.
