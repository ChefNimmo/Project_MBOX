# REA setup and first binary investigation — 4 November 2026

## Setup verification
REA 6.8.0 configured for Codex; bundled skill version 49 installed.
Existing config backed up to C:\Users\chefn\.codex\config.toml.rea.backup.
Scoped doctor reported healthy=true, registration aligned, skill aligned.
environment_healthy=false: no native analysis provider configured.
REA MCP transport diagnostic exited 0.
Active chat tool discovery still shows no REA tools; restart Codex before testing active-session access.
No existing audio driver was installed or replaced.

## Initial target and observations
Inspected the official Ploytec Windows x64 2.8.45 demo archive already downloaded locally.
These are THIRD-PARTY drivers, not the original Digidesign/Avid binaries.
Archive extracted for static inspection only; no target binaries executed.
INF DriverVer: 08/13/2010,2.8.45.
INF maps USB VID_0DBA PID_1000 to Digidesign Mbox and pgusbwdm.sys.
It also contains an Mbox 2 entry; an INF entry is not proof of working support.
REA PE import parser completed on both x64 kernel images:
- pgusbwdm.sys: 67 imports across ntoskrnl.exe and USBD.SYS.
- pgusbmm3.sys: 23 imports across NTOSKRNL.exe and portcls.sys.
pgusbwdm.sys imports USBD_CreateConfigurationRequestEx, USBD_ParseConfigurationDescriptorEx and USBD_GetUSBDIVersion.
These imports suggest useful paths for later disassembly; they do not establish executed behaviour or actual USB transfer values.
Raw REA Evidence JSON retained locally in research/rea-analysis/*-imports.json, including target hashes.

## Next investigation
1. Restart Codex and verify REA tool access in the active chat.
2. Configure a native provider (Ghidra or IDA). Neither was configured by this setup.
3. Obtain the authentic original Digidesign driver package and identify binaries using INF mappings and hashes.
4. Locate initialization, control-transfer construction, isochronous scheduling, rate switching and S/PDIF paths.
5. Compare recovered commands to ALSA source and hardware USB captures.
Do not claim a working Windows 11 driver or full-feature compatibility from static imports.
