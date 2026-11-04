# Ghidra configuration and original driver search — 4 November 2026

## Completed
Installed portable Ghidra 12.1.4 (REA-supported 12.1 series) and Temurin JDK 21.0.13+9 under C:\Users\chefn\Documents\Codex\Mbox-Analysis\tools.
Downloads were fetched from official GitHub/Adoptium sources and checked against published SHA-256 values.
Configured REA's Codex entry with GHIDRA_INSTALL_DIR and JAVA_HOME, retaining backups.
REA reports Ghidra configured=true, available=true, providerVersion=12.1.4.
Successfully decompiled the Ploytec pgusbwdm.sys entry at 0xf1026ac0 using Ghidra. It calls initialization helpers and references PGUSBWDM registry parameters. This does not recover the USB protocol yet.

## Original Digidesign target
Downloaded https://download.mictests.com/drivers/audio/Avid/Pro/Windows-x86.zip as Digidesign-legacy-x86-mirror.zip.
Source listing: https://mictests.com/drivers/avid/avid-pro-microphone-driver-for-windows-xp-vista-7-8-8.1-10-x86
Extracted Dalwdm.sys and dalwdm.inf for static inspection.
Version resource: FileVersion 6.4.0.138, ProductVersion 6.4, company Digidesign, A Division of Avid Technology, Inc.
INF maps USB VID_0DBA PID_1000 (original Mbox) and PID_1001 (firmware mode) to Dalwdm.sys.
The INF DriverVer line is commented out; use binary version metadata, not its commented example.
Windows reports no embedded digital signature. Vendor metadata is not authenticated provenance; this is an untrusted mirror candidate.
No driver or installer was executed, installed or bound to hardware.
Import inspection completed; Ghidra entry-point analysis initiated.
Local Evidence and logs: research/ and work/ under the analysis directory.

## Latest-release limitation
This is NOT the latest original driver. Pro Tools LE 8.0.1 is identified as the final original-Mbox-compatible Pro Tools release in Avid community support:
https://duc.avid.com/showthread.php?t=427224
This does not establish the exact version or contents of its driver files.
Manufacturer archive endpoints returned 500/526, old KB pages returned 404, archive lookup timed out, and an alternative distributor returned 403.
No final 8.0.1-era driver archive was successfully obtained or authenticated.
Do not substitute Mbox 2 or third-generation packages.

## Next
Analyze original-Mbox-specific dispatch/control paths and obtain the missing final package and companion files.
The existing chat's REA process predates provider configuration; reconnect/restart it to pick up the new environment. Ghidra was verified through REA CLI in the meantime.
