param([ValidatePattern('^[0-9A-Za-z.-]+$')][string]$VersionTag='0.2.0-dev')
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath("$PSScriptRoot\..\..")
$native=Join-Path $root 'native'
$build=Join-Path $native 'build\windows-endpoint'
$dist=Join-Path $root 'dist'
# Fresh versioned stages prevent stale binaries or source from entering a package.
$tag=Get-Date -Format 'yyyyMMdd-HHmmss'
$stage=Join-Path $root "work\native-development-$tag"
$source=Join-Path $root "work\native-source-$tag"
New-Item -ItemType Directory -Force "$stage\Driver","$source\native\windows-endpoint","$source\native\asio","$source\native\kmdf",$dist | Out-Null
foreach($file in @('NimBoxAudio.sys','NimBoxAudioBroker.exe','NimBoxWindowsProbe.exe')) {
    if(!(Test-Path "$build\$file")){throw "Missing build output $file"}
}
Copy-Item "$build\NimBoxAudio.sys" "$stage\Driver"
Copy-Item "$PSScriptRoot\NimBoxAudio.inf" "$stage\Driver"
Copy-Item "$build\NimBoxAudioBroker.exe" $stage
Copy-Item "$build\NimBoxWindowsProbe.exe" $stage
Copy-Item "$PSScriptRoot\README.md","$PSScriptRoot\MICROSOFT-LICENSE.txt" $stage
Copy-Item "$native\COPYING" $stage
Copy-Item "$native\asio\THIRD-PARTY-NOTICES.txt" $stage
$kits="${env:ProgramFiles(x86)}\Windows Kits\10"
& "$kits\Tools\10.0.26100.0\x64\infverif.exe" /w /v "$stage\Driver\NimBoxAudio.inf"
if($LASTEXITCODE){throw 'INF validation failed'}
& "$kits\bin\10.0.26100.0\x86\Inf2Cat.exe" "/driver:$stage\Driver" /os:10_GE_X64 /uselocaltime /verbose
if($LASTEXITCODE){throw 'Catalogue generation failed'}
Get-ChildItem $PSScriptRoot -File | Copy-Item -Destination "$source\native\windows-endpoint"
Copy-Item "$PSScriptRoot\sysvad" "$source\native\windows-endpoint" -Recurse
foreach($file in @('Engine.cpp','Engine.h','NimBoxIds.h')) {Copy-Item "$native\asio\$file" "$source\native\asio"}
foreach($folder in @('core','transport')) {Copy-Item "$native\$folder" "$source\native" -Recurse}
foreach($file in @('usbdk_backend.h','mbox_ioctl.h')) {Copy-Item "$native\kmdf\$file" "$source\native\kmdf"}
New-Item -ItemType Directory -Force "$source\native\third_party" | Out-Null
Copy-Item "$native\third_party\usbdk" "$source\native\third_party" -Recurse
Copy-Item "$native\COPYING" "$source\native"
$sourceZip=Join-Path $dist "NimBox-1-native-$VersionTag-source.zip"
Compress-Archive -Path "$source\native" -DestinationPath $sourceZip -Force
Copy-Item $sourceZip "$stage\source.zip"
@'
UNSIGNED DEVELOPMENT SNAPSHOT — NOT INSTALLED OR KERNEL-TESTED

NimBoxAudio.sys and its catalogue are unsigned. This ZIP is not the ASIO installer.
Do not install it as a finished replacement. A Windows driver signing/testing route
must be agreed before a controlled first load. No boot, trust or device changes have
been performed by this packaging script.

See README.md for architecture, successful checks and outstanding kernel tests.
The full corresponding source, including the prepared Microsoft source and MIT
licence, is in source.zip. It can be rebuilt without downloading the sample:
  native/windows-endpoint/Build-Reference.ps1 -NimBox
  native/windows-endpoint/build-broker.cmd
Use Prepare-Source.ps1 only to reconstruct the Microsoft source into a NEW destination.

Driver/ contains the root-enumerated audio driver package; the broker requires an
already installed, verified UsbDk 1.0.22 runtime. No UsbDk installer is bundled here.
Keep the working ASIO 0.1.0 installer available for rollback. Do not open that ASIO
frontend while the native broker owns USB. Shared ASIO access is not implemented yet.
'@ | Set-Content "$stage\DEVELOPMENT-NOTES.txt"
$manifest=Get-ChildItem "$stage\Driver" -File | ForEach-Object {
    [ordered]@{File="Driver/$($_.Name)";SHA256=(Get-FileHash $_.FullName -Algorithm SHA256).Hash;Bytes=$_.Length}
}
$manifest+=Get-Item "$stage\NimBoxAudioBroker.exe" | ForEach-Object {
    [ordered]@{File=$_.Name;SHA256=(Get-FileHash $_.FullName -Algorithm SHA256).Hash;Bytes=$_.Length}
}
$manifest | ConvertTo-Json -Depth 4 | Set-Content "$stage\manifest.json"
$package=Join-Path $dist "NimBox-1-native-$VersionTag-x64.zip"
Compress-Archive -Path "$stage\*" -DestinationPath $package -Force
Get-FileHash $package,$sourceZip -Algorithm SHA256 | Select-Object Path,Hash |
    ConvertTo-Json | Set-Content (Join-Path $dist "NimBox-1-native-$VersionTag-manifest.json")
Write-Host "Unsigned development package: $package"
