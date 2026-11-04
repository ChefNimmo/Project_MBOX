# NimBox naming, signing cost and noise measurement notes

Checked: 2026-11-04 (Australia/Sydney). These notes record research and implementation choices;
they are not a legal clearance or a price quotation.

## Naming and research provenance

Use **NimBox 1 USB audio device** as the prototype display name. State that this is an independent
compatibility project, without Avid/Digidesign affiliation or endorsement. Retain factual references
to the original Mbox in compatibility documentation and historical research, with source credits.
Do not rewrite Git history or delete third-party notices simply to remove a product name.

Australian Trade Marks Act 1995 section 122(1)(c) recognises good-faith use to indicate intended
purpose, subject to its conditions. That is a reason to distinguish truthful compatibility references
from product branding; it is not a conclusion that every use in this repository is legally cleared.
Removing a name does not determine copyright, licence, patent or reverse-engineering rights.
Copyright Act 1968 section 47D also has specific conditions for interoperability-related reproduction;
it is not a general permission to redistribute original drivers, firmware or copied implementation.
Any distribution review should examine the actual code/material and applicable jurisdictions.

Primary texts:
- https://www.legislation.gov.au/C2004A04969/2024-04-01/2024-04-01/text/original/epub/OEBPS/document_1/document_1.html
- https://www.legislation.gov.au/C1968A00063/2026-04-02/2026-04-02/text/original/epub/OEBPS/document_1/document_1.html

## Certificate cost and eligibility

Microsoft's Hardware Developer Program currently requires an EV code-signing certificate for
registration, organisation/company details, an Entra administrator and an authorised legal contact.
A certificate does not by itself provide a Microsoft signature for a kernel binary; the appropriate
dashboard submission/signing process is still required.

For one direct-vendor price example, SSL.com lists its one-year EV certificate at US$349/year.
Compatible key storage/cloud signing is additional. Its page explicitly warns that its listed YubiKey
option may not satisfy kernel-driver requirements. Do not assume that buying the advertised USB
token is sufficient. Confirm the precise Microsoft-compatible signing method and account eligibility
with the provider before any purchase. No purchase, enrollment or account change has been made.

Do not buy a certificate yet merely to continue software development. Build and simulated tests do
not need it. A self-generated certificate does not make this unsigned kernel driver loadable on an
unchanged, normally enforced production Windows installation. The user's boot-security constraint
remains in force.

Primary sources:
- https://learn.microsoft.com/en-us/windows-hardware/drivers/dashboard/hardware-program-register
- https://learn.microsoft.com/en-us/windows-hardware/drivers/dashboard/code-signing-reqs
- https://www.ssl.com/products/software-integrity/code-signing/ev/

## Noise measurement: deferred, no recording performed

The user has another audio interface but will connect it later. Recording is deferred.
Sensitive IEMs can reveal analogue output hiss that a software loopback never sees. A recording of
the Mbox's own ADC input is not, by itself, a measurement of its headphone amplifier's output noise.

A previously recorded random hiss waveform will not cancel subsequent independent noise when
inverted. If independent equal-power noise is added, powers add rather than cancel (about +3 dB).
A learned spectral profile can guide attenuation of noise in recordings, but may alter wanted audio
and cannot directly remove random noise introduced after the DAC by the headphone amplifier.
Stable coherent hum/tones are a separate case; diagnosis and targeted filtering can be evaluated
after measurement. No cancellation filter or correction EQ should be enabled by default in the driver.

Later measurement plan:

1. Record the original unit's line outputs through the second interface's line inputs with correct
   cables, phantom power off and levels that cannot clip. Keep the second interface's gain/settings
   documented; a baseline of its own input noise is needed to interpret results.
2. Separately measure the headphone output with an appropriate stereo breakout, attenuation and
   load. Do not assume a stereo headphone TRS feed is a balanced mono line connection.
3. Capture short silent runs at fixed sample rate/gains with playback digitally silent, then compare
   USB/CPU activity and output-volume settings. Keep the commercial demo beep identifiable; do not
   use its spectrum as the floor-noise reference.
4. Calculate per-channel RMS/peak dBFS, DC, averaged power spectrum and tonal peaks. Retain capture
   metadata and an optional spectral profile. Absolute analogue dBu requires voltage calibration;
   dBFS alone is not an absolute hardware noise specification.
5. Decide whether the cause is input noise, DAC/headphone noise, ground/USB interference or contacts
   before considering processing. A recording profile is not a promised headphone-hiss cure.
