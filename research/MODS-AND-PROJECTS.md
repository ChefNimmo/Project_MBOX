# Mbox modifications and community projects

Research sweep: 4 November 2026 (Australia/Sydney).

A broad, best-effort search, not an exhaustive inventory of the web. Results concern Digidesign/Avid audio interfaces; mailbox software and unrelated MBOX products excluded. Author claims are distinguished from hardware tests: none of these projects was installed, flashed or electrically tested during this sweep.

## Most useful for Project MBOX

1. Original-Mbox ALSA source remains the most direct protocol reference.
2. Zach Poff's updated LK1 mod is a new hardware lead: the author reports standalone S/PDIF as well as analogue operation. Our earlier analogue-only description applied to the old mod and was incomplete for the 2024 update.
3. Mbox 2 Mini open firmware offers an example of class-driver compatibility and user-mode ASIO, but targets a different chip/device and currently advertises only 16-bit recording. It is not a full-feature original-Mbox solution.
4. SelahOS documents Mbox 2 initialization reverse engineering; obtain and review the exact patch before drawing conclusions for Mbox1.

## Catalogue

### mbox1-jumper: Standalone hardware mod

- Model: **Original Mbox**
- Source: [mbox1-jumper](https://zachpoff.com/resources/digidesign-mbox-1-standalone-mod/)
- Evidence: Author article inspected.
- Findings: 2024 update describes reversible LK1 jumper for standalone preamp and S/PDIF; later confirmations reported. Earlier destructive trace-cut mod is superseded. Does not restore host USB audio.
- Value to our project: High: hardware enable/power behaviour; corrects our earlier analogue-only summary.

### mbox1-headphones: Headphone socket mod

- Model: **Original Mbox**
- Source: [mbox1-headphones](https://groupdiy.com/threads/digidesign-mbox-1-headphone-sockets-mod.30586/)
- Evidence: Original poster inspected.
- Findings: Two jumper wires bypass socket switching so front and rear headphones can operate together. Photo attachment linked; electrical loading not independently tested.
- Value to our project: Low for driver; useful hardware reference.

### mbox1-linux: Linux driver

- Model: **Original Mbox**
- Source: [mbox1-linux](https://www.zamaudio.com/?p=953)
- Evidence: Previously inspected author source.
- Findings: Capture/playback fixes, firmware 0.22 recommendation and historical source; author reports upstream inclusion at Linux 3.19.
- Value to our project: Highest protocol relevance; use with current kernel comparison.

### mbox1-alsa-source: Historical source

- Model: **Original Mbox**
- Source: [mbox1-alsa-source](https://github.com/zamaudio/alsa-driver/tree/mbox1clean)
- Evidence: Local source inspected earlier.
- Findings: Original-Mbox fixed dual-endpoint handling and packed 24-bit big-endian format. Existing archive in downloads.
- Value to our project: High; source license must be preserved.

### mbox1-ploytec: Proprietary alternative driver

- Model: **Original Mbox**
- Source: [mbox1-ploytec](https://www.usb-audio.com/download/)
- Evidence: Vendor inspected earlier.
- Findings: Official Windows x64 demo lists Mbox and Windows 11; old release, no full-feature validation. Vendor support documents white-noise issue.
- Value to our project: High practical baseline; not open source.

### mbox1-repack: Archived binary repack

- Model: **Original Mbox**
- Source: [mbox1-repack](https://github.com/MeblIkea/MBOX1-Digidesign-driver)
- Evidence: Repository inspected.
- Findings: Archived in 2023; author says they did not create driver. Not evidence of reverse engineering or ongoing development. Shared keys excluded.
- Value to our project: Low; prefer official vendor demo.

### mbox2-mini-open: Open firmware / ASIO / hub

- Model: **Mbox 2 Mini ONLY**
- Source: [mbox2-mini-open](https://github.com/ItsMeATaco/mbox2-mini-open-firmware)
- Evidence: Repository README inspected.
- Findings: Author describes TUSB3200A replacement firmware and Windows class-driver operation, 24-bit playback but 16-bit recording, 44.1/48 kHz; user-mode ASIO layer and hub. GPL-3.0 shown. Claimed tests not independently reproduced.
- Value to our project: High architectural reference; cannot flash onto original Mbox.

### mbox2-linux: Linux driver and firmware research

- Model: **Mbox 2**
- Source: [mbox2-linux](https://www.zamaudio.com/97/)
- Evidence: Author article inspected.
- Findings: Historical unofficial Linux driver, source module and firmware upgrade discussion. Not original-Mbox firmware.
- Value to our project: Useful comparative initialization research.

### mbox2-upgrade: Firmware upgrade walkthrough

- Model: **Mbox 2**
- Source: [mbox2-upgrade](https://gist.github.com/vectorsigma/37c8e9993de29b25caef6c9c2686797b)
- Evidence: Author gist inspected.
- Findings: Legacy firmware recovery/update account; starts with a notice directing readers to a newer method in comments. Preserve revision context.
- Value to our project: Reference only; old procedures may be superseded.

### mbox2-selah: Linux patch / control console

- Model: **Mbox 2**
- Source: [mbox2-selah](https://www.selahos.io/docs/audio-interfaces.html)
- Evidence: Project documentation inspected.
- Findings: Project says it reverse-engineered Windows initialization and skips an incompatible command in Linux. Playback/recording/MIDI reported on one machine at 48 kHz; stability unqualified; docs say changes not in Beta 2.0.1 download.
- Value to our project: High comparison lead; exact patch not obtained.

### mbox2-analogue: Analogue DIY modification

- Model: **Mbox 2**
- Source: [mbox2-analogue](https://groupdiy.com/threads/digidesign-mbox-2-mods.51554/)
- Evidence: Search-index excerpt only; direct fetch 403.
- Findings: Discussion of converter buffer circuitry, component changes and separate 5V supply; attachments/schematics not verified in this sweep.
- Value to our project: Hardware lead only; not an original Mbox schematic.

### mbox3pro-apple: Experimental Apple Silicon driver

- Model: **Mbox Pro 3 FireWire**
- Source: [mbox3pro-apple](https://www.reddit.com/r/Avid/comments/1vtzqyu/avid_mbox_pro_3_firewire_sillicon_driver_third/)
- Evidence: Author announcement inspected.
- Findings: Author reports user-space FireWire/DICE work and plans for HAL integration; source release promised, not confirmed here.
- Value to our project: Conceptual only; different transport and generation.

### mbox3pro-bla: Commercial analogue upgrade

- Model: **Mbox Pro 3**
- Source: [mbox3pro-bla](https://mods.blacklionaudio.com/products/avid-mbox-3-pro-mod)
- Evidence: Vendor listing inspected.
- Findings: Vendor offers op-amp changes and premium signal-path capacitor changes. Performance claims are vendor claims; no independent measurements collected.
- Value to our project: No Windows compatibility benefit established.

### mbox-schematics: Schematic availability discussion

- Model: **Multiple / original mentioned**
- Source: [mbox-schematics](https://groupdiy.com/threads/digidesign-schematics.36564/)
- Evidence: Search-index excerpt.
- Findings: Community requests for Digidesign circuit diagrams; no verified original-Mbox service schematic obtained.
- Value to our project: Documents missing information, not a schematic source.

## Collection and limits

Machine-readable catalogue: PROJECTS.json. Download attempts and SHA-256 values: collection/MANIFEST.json. Saved pages are raw HTML and may require network assets for full viewing; archive presence is not proof of all linked images being preserved. Source ZIPs are reference snapshots, not endorsed builds. No executables from these new projects were run. Existing Ploytec and ALSA downloads remain in downloads/.

Public GitHub research stores this catalogue and source links. Raw third-party pages, binary archives and shared license keys are not republished there. A license label at repository level is not a completed dependency/license audit.

Open gaps: original-Mbox service schematics, verified Windows 11 full-feature driver, S/PDIF control protocol, hardware revision identification, USB captures, and long-running stability tests. Announcements and vendor descriptions do not close those gaps.
