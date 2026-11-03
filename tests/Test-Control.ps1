$ErrorActionPreference='Stop'
Add-Type -Path (Join-Path $PSScriptRoot '../windows/MboxControl.cs')
function Assert-Hex($actual,$expected) {
 if([BitConverter]::ToString($actual) -ne $expected){throw "Unexpected packet: $([BitConverter]::ToString($actual))"}
}
Assert-Hex ([MboxResearch.MboxControl]::GetRate(2)) 'A2-81-00-01-02-00-03-00'
Assert-Hex ([MboxResearch.MboxControl]::SetRate(2,44100)) '22-01-00-01-02-00-03-00-44-AC-00'
Assert-Hex ([MboxResearch.MboxControl]::SetRate(129,48000)) '22-01-00-01-81-00-03-00-80-BB-00'
if([MboxResearch.MboxControl]::DecodeRate([byte[]](68,172,0)) -ne 44100){throw 'Decode failed'}
foreach($endpoint in 0,128,18){
 $rejected=$false
 try{[MboxResearch.MboxControl]::GetRate($endpoint)|Out-Null}catch{$rejected=$true}
 if(-not $rejected){throw 'Invalid endpoint accepted'}
}
$rejected=$false
try{[MboxResearch.MboxControl]::SetRate(2,96000)|Out-Null}catch{$rejected=$true}
if(-not $rejected){throw 'Unsupported rate accepted'}
Write-Output 'PASS: setup bytes, 44.1/48 kHz payloads, decoding and invalid input rejection. Offline only.'
