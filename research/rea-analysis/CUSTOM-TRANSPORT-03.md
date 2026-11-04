# NimBox 1 naming and application-block adapter

Date: 2026-11-04 (Australia/Sydney).

The requested display name is **NimBox 1 USB audio device**. The experimental INF's device and
service display strings now use that exact name; its provider is NimBox project. No installed device
name or firmware string has been changed because the prototype remains unsigned and unloaded.
Existing research filenames, compatibility references, identifiers and credits are preserved.

## Implementation and verification

Added a stereo audio FIFO between planar PCM32 application blocks and packed big-endian PCM24 USB
packets. The streaming controller now uses it in both directions: zero-filled 256-frame blocks feed
OUT packets, and capture packets feed 256-frame blocks for peak computation. It has bounded external
storage, rejects overflowing writes without partial changes, zero-fills read shortages and reports
actual consumed frames. There is no sample-rate conversion or denoising.

The FIFO has a single owner and is not thread safe. The current scheduler uses it on one thread.
Future ASIO integration must introduce an explicit thread-safe ownership boundary; no ASIO driver
or registration has been added yet. Audio workspace is allocated once per diagnostic run and kept
off the control-thread stack; packet conversion itself allocates nothing.

`native/build.cmd` passed with exit code 0 after these changes:

- Existing packet/conversion tests and all 25 duplex admission states.
- Driver and integrated controller compilation with warnings treated as errors and source static
  analysis (system/kit header implementations excluded).
- All 18 simulated streaming scenarios, including ten virtual minutes per sample rate.
- New adapter round-trip tests at 44100/48000 Hz: ten virtual seconds of changing signed stereo
  samples, 256-frame host blocks, variable 44/45/48-frame packets, wraparound, overflow atomicity,
  invalid packet rejection and zero-filled underruns.
- Microsoft InfVerif accepted the renamed device strings and INF.

No custom driver was installed or loaded. No boot-security setting changed. No sound was emitted
and no microphone/input recording was made. Physical floor-noise capture is deferred at the user's
request until a second interface can be connected. Hardware streaming, lifecycle validation and
ASIO/Windows audio integration remain outstanding.
