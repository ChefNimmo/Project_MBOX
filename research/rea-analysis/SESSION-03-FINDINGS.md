# Session 03 — sampling-frequency controls and offline prototype
4 November 2026. Static inspection only; no hardware requests sent.

## Hardware
Read-only Get-PnpDevice initially needed elevated CIM access. Elevated queries succeeded but returned no present VID_0DBA device on two checks. This does not diagnose the hardware: it may not yet be connected or enumerating under the expected identity.

## Rate path established from assembly
Target remains Dalwdm.sys 6.4.0.138, hash as earlier sessions.
- 0x10008cb0 copies configuration word[10] into object+0xf4 (constructor default 44100).
- 0x10008710 reads endpoint sampling frequency via 0x10007730 and compares it with object+0xf4.
- If mismatched, caller assembly pushes the desired rate and endpoint address into 0x100083e0. It reads back and compares again.
- 0x100083e0 ends RET 8; its actual two stack arguments were incompletely recovered in pseudocode. Assembly points the outgoing 3-byte buffer to the rate argument.
- 0x10007730: class endpoint request, IN, request 0x81, value 0x0100, index endpoint address, length 3.
- 0x100083e0: class endpoint request, OUT, request 0x01, value 0x0100, index endpoint address, length 3.
These map to USB Audio 1.0 GET_CUR/SET_CUR sampling-frequency control. Desired rate is a 24-bit little-endian quantity; do not confuse control payload endianness with big-endian audio samples.

Expected setup bytes (derived from request fields, not a captured bus transaction):
GET: A2 81 00 01 EP 00 03 00
SET: 22 01 00 01 EP 00 03 00, followed by three rate bytes.
44100 payload: 44 AC 00. 48000 payload: 80 BB 00.
Actual supported endpoint addresses and sequencing still require hardware verification.

## Configuration flags and unresolved clock/source meanings
0x10008cb0 extracts configuration bits 0, 4, 8 into object+0xe9, +0xea, +0xeb.
0x10008490 writes one byte (boolean+1, hence 1 or 2), through class-interface request 1, value 0, index 0x0500; 0x100077c0 reads that control with request 0x81 and compares returned byte with 2.
The +0xeb branch causes both stream endpoints to be set to rate 0 instead of the desired rate. This may relate to external clock, but exact semantics and whether it selects S/PDIF remain unconfirmed.
Instruction decoding at 0x10008510..0x1000863f confirms run-time reconfiguration paths also call these helpers, including zero versus stored-rate writes and interface-control updates.
No zero-rate operation is exposed by the offline prototype.

## Windows prototype and validation
windows/MboxControl.cs adds pure serialization for GET rate, SET rate at 44100/48000 and decoding 3-byte responses.
tests/Test-Control.ps1 checks exact setup bytes/payloads for 44.1/48 kHz, decoding, endpoint validity and unsupported-rate rejection.
Compiled using Add-Type and all tests passed. The prototype has no hardware-access capability and is not an installed driver. It does not validate Windows 11 transport or real hardware.

## References used to map URB structures/constants
https://raw.githubusercontent.com/microsoft/win32metadata/main/generation/WinSDK/RecompiledIdlHeaders/shared/usb.h
https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/usb/ns-usb-_urb_control_vendor_or_class_request
Microsoft header defines class-device/interface/endpoint URB functions 0x1a/0x1b/0x1c and transfer-direction bits. Matching 32-bit structure field sequence explains the observed length 0x50 and trailing Request/Value/Index fields.

## Saved
original-driver-session-03.json: 300 records; snapshot also retained. Session closed after export.
Next: obtain read-only live descriptors, identify interface unit 5 from descriptors, correlate the configuration flags with user-facing control names, and validate requests against working-driver captures. Astra is not needed yet; hardware evidence is the current constraint.
