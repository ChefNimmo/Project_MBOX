$ErrorActionPreference='Stop'
Add-Type -Path (Join-Path $PSScriptRoot '../windows/Stereo24.cs')
function Check($bytes,$hex){if([BitConverter]::ToString($bytes) -ne $hex){throw 'Sample/channel mismatch'}}
$left=[byte[]](255,255,127,1,0,0)
$right=[byte[]](0,0,128,86,52,18)
$wire=[MboxResearch.Stereo24]::Pack($left,$right)
Check $wire '7F-FF-FF-80-00-00-00-00-01-12-34-56'
$channels=[MboxResearch.Stereo24]::Unpack($wire)
Check $channels[0] 'FF-FF-7F-01-00-00'
Check $channels[1] '00-00-80-56-34-12'
Check $left 'FF-FF-7F-01-00-00'
foreach($length in 1,2,4,5){$bad=$false;try{[MboxResearch.Stereo24]::Unpack((New-Object byte[] $length))|Out-Null}catch{$bad=$true};if(-not $bad){throw 'Accepted partial frame'}}
$bad=$false;try{[MboxResearch.Stereo24]::Pack([byte[]](1,2,3),[byte[]]@())|Out-Null}catch{$bad=$true};if(-not $bad){throw 'Accepted unequal channels'}
Write-Output 'PASS: signed boundary vectors, channel placement, round trip, source preservation and malformed buffers. Offline only.'
