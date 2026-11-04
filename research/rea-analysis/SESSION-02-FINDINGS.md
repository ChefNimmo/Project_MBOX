# Session 02 — original Mbox object and streaming path
4 November 2026. Static analysis only; no target execution or USB writes.

Target: Dalwdm.sys 6.4.0.138 x86, SHA256 bd225015460af0dfb6b791618ed35a4e06841cbffe5b3c8516cfda6c23e95d44. Addresses apply only to this image. Decompiled types and argument counts are provisional.

## Confirmed control flow
Decoded the previously unrecognised function body beginning 0x10004820 instruction by instruction:
- 0x1000492f calls classifier 0x100046a0.
- 0x10004934 compares result with 0x16.
- Match: allocates 0x1bc bytes, calls constructor 0x10007ed0, then attachment/init helper 0x10006dd0.
- 0x18 branch instead allocates 0xf8, calls 0x10009990 then 0x10009660.
Combined with prior identity-string branches, this isolates normal original-Mbox code from firmware mode.
Ghidra's procedure_callers returned an empty list for the classifier even though decoded instructions contain its call: missing recovered boundaries limit the call graph.

## Object observations
Constructor 0x10007ed0:
- writes pointer 0x100bf978 at object offset 0 (virtual-function-table candidate).
- writes 0xac44 (44100 decimal) at byte offset 0xf4.
- initialises flags and a mutex.
44100 is a strong default-sample-rate lead, not yet a traced USB rate setter.
The first nine pointers at 0x100bf978 are:
0x10003bc0, 0x10003bd0, 0x10003f60, 0x10007f50, 0x100090d0, 0x10003e30, 0x10006e10, 0x10003be0, 0x10003bf0.
Only this candidate nine-entry region is described; subsequent bytes include other data.

## Configuration and transfer path
- 0x10007f50 is a small wrapper calling 0x10006eb0.
- 0x10006eb0 retrieves configuration data, calls 0x100060f0 with (0,0) and (1,1), builds/submits configuration and stores interface/pipe information.
- 0x100060f0 walks descriptors, matching descriptor type byte 4, interface-number byte and alternate-setting byte. Thus the above calls select interface 0 alt 0 and interface 1 alt 1.
- 0x10005ae0 constructs an IRP and writes 0x220003 plus its request pointer into the next-stack data before calling a submission helper.
- 0x10007240 and 0x100072f0 construct paired transfer structures with function field 10, different pipe references and flag values 4 / 7.
- 0x10006e60 caps a grouping value at 8 and computes buffer capacity using a stride of 0x138.
Do not equate that 0x138 stride with endpoint wMaxPacketSize: the Linux table's 0x130 and this buffer layout need reconciliation with real descriptors.

## Audio format evidence
0x100074a0 loops over two channels and reverses three bytes per sample while moving from per-channel buffers into a stream buffer, stepping destination by six bytes per stereo frame.
0x10007540 performs the corresponding byte reversal into per-channel buffers, with six-byte source frame stride.
This independently supports stereo packed 24-bit endian conversion in the original Windows driver, consistent with the historical ALSA S24_3BE findings. It does not by itself establish every buffer header/layout or hardware mode.
Our existing C# endian prototype is still a limited format reference, not the full planar/interleaved driver implementation.

## Raw control-request leads (semantic mapping pending)
The builders below submit stack-built structures through 0x10005ae0.
Record field values as observations, not complete USB setup packets:
- 0x10007730: structure length 0x50, function-like field 0x1c; byte fields 0 and 0x81; word 0x100; another word from caller's byte; buffer length 3; transfer-flags-like value 1. Returns collected data on success and waits through helper(200).
- 0x100077c0: length 0x50, function-like 0x1b; byte 0x81; words 0 and 0x500; buffer length 1; compares returned byte with 2.
- 0x10007830: length 0x50, function-like 0x1a; request-like byte 6; word 0x101; length 0x12; copies a returned word.
- 0x10007c90: length 0x50, function-like 0x1b; request-like byte 0; words 0x10 and 1; no data buffer.
These need validated x86 WDK structure offsets, assembly checks and caller tracing before mapping to bRequest/wValue/wIndex. No claim yet that any is the sample-rate or S/PDIF setter.

## Evidence and continuation
Local research/original-driver-session-02.json: 201 records, 1370485 bytes.
Local research/original-driver-session-02.snapshot.json: snapshot for later queries.
Binary session closed after export.
Next: inspect callers and instruction-level references to the four control helpers; verify URB layouts against WDK definitions; follow reads/writes of object+0xf4; compare observed requests with ALSA source and recorded USB traffic.
