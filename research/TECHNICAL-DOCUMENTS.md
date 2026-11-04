# Technical document collection — 4 November 2026

## Feasibility assessment
A Windows 11 implementation is technically plausible. Existing Linux capture/playback handling and Ploytec support demonstrate workable host access. This does not prove full-feature Windows compatibility. The current C# prototype only swaps PCM byte order; it is not a USB driver and does not materially establish transport feasibility.

Main unresolved work: actual USB descriptors and firmware revision, initialization/control transfers, synchronous playback versus asynchronous capture timing, digital input selection and external clock, Windows AVStream integration, power/reconnect handling, signing and sustained stability.

## Downloaded locally in research/documents
- Getting-Started-Mbox.pdf — original Digidesign manual, hosted at https://recording.rrfedu.com/images/Alex-Venne/Getting_Started_Mbox_28599.pdf
- USB-Audio-Class-1.0.pdf — https://www.usb.org/sites/default/files/audio10.pdf
- USB-Audio-Formats-1.0.pdf — https://www.usb.org/sites/default/files/frmts10.pdf
- USB-Audio-Terminal-Types-1.0.pdf — https://www.usb.org/sites/default/files/termt10.pdf
- Ploytec-installation.pdf — https://www.usb-audio.com/install.pdf (legacy multi-platform instructions, not Windows 11 validation)
- TI-TAS1020B-EVM-guide-unconfirmed-component.pdf — https://www.ti.com/lit/ug/sleu031/sleu031.pdf

TI controller family is a research lead, not confirmed board identification. Do not assume the B revision is fitted. TI datasheet symlink and SLES025B URL returned 404. These are reference specifications, not an Mbox-specific protocol disclosure.

Files checked for PDF signatures; not yet fully read or visually inspected. Local validation.json records header checks. Existing SHA256.json covers the successful initial document batch.

## Additional located documents and leads
- Original 6.4 manual including ASIO/WaveDriver appendices: https://www.manualslib.com/manual/835427/Digidesign-Mbox.html
- Original 6.1 manual: https://www.manualslib.com/manual/836187/Digidesign-Mbox.html
- Original 6.7 Basics guide: https://www.manualslib.com/manual/1391530/Digidesign-Mbox.html
- Original Linux research and firmware notes: https://www.zamaudio.com/?p=953
- Historical official download index: https://archive.digidesign.com/download/mbox/ (browser fetch failed)
- Original Mbox standalone modification: https://zachpoff.com/resources/digidesign-mbox-1-standalone-mod/

No verified public original-Mbox service schematic or complete vendor USB protocol specification was found in this search. This collection is not claimed exhaustive. Mbox 2/3, Mbox Mini and unrelated MBOX documents are excluded.

PDFs remain local; source links and findings are stored in GitHub instead of republishing copyrighted manuals.
