# Validation — 4 November 2026

PASS: Pcm24.cs compiled using PowerShell Add-Type; known 24-bit boundary vectors, two-frame channel ordering, endian round trip, empty input and incomplete-frame rejection passed.

The machine's PowerShell execution policy blocked direct .ps1 invocation. The same reviewed script body was run inline without changing system execution policy.

Not tested: USB descriptors, firmware revision, capture/playback, clock control, S/PDIF, latency, WDK compilation, driver installation or signing. Get-PnpDevice failed because CIM access was unavailable. No existing device driver was changed.

This is a tested offline format-conversion reference, not a working Windows driver.
