$ErrorActionPreference = 'Stop'
Add-Type -Path (Join-Path $PSScriptRoot '../windows/Pcm24.cs')
function Assert-Bytes($actual, $expected) {
    if ([BitConverter]::ToString($actual) -ne [BitConverter]::ToString($expected)) {
        throw 'PCM byte mismatch'
    }
}
# Maximum positive and minimum negative 24-bit stereo samples.
Assert-Bytes ([Mbox1.Pcm24]::SwapStereoEndian([byte[]](255,255,127,0,0,128))) ([byte[]](127,255,255,128,0,0))
# Two stereo frames: zero, -1, +1, and a non-symmetric sample.
$inputBytes = [byte[]](0,0,0,255,255,255,1,0,0,86,52,18)
Assert-Bytes ([Mbox1.Pcm24]::SwapStereoEndian($inputBytes)) ([byte[]](0,0,0,255,255,255,0,0,1,18,52,86))
Assert-Bytes ([Mbox1.Pcm24]::SwapStereoEndian([Mbox1.Pcm24]::SwapStereoEndian($inputBytes))) $inputBytes
Assert-Bytes ([Mbox1.Pcm24]::SwapStereoEndian([byte[]]@())) ([byte[]]@())
foreach ($length in 1,3,5,7) {
    $rejected = $false
    try { [Mbox1.Pcm24]::SwapStereoEndian((New-Object byte[] $length)) | Out-Null } catch { $rejected = $true }
    if (-not $rejected) { throw "Accepted incomplete frame length $length" }
}
Write-Output 'PASS: known sample vectors, channel/frame order, round trip, empty buffer, incomplete frame rejection.'
