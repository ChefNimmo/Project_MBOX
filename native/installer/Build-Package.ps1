$ErrorActionPreference='Stop'
$analysisRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$native=Join-Path $analysisRoot 'native'
$dist=Join-Path $analysisRoot 'dist'
$stage=Join-Path $analysisRoot 'work\nimbox-source-0.1.0'
$msi=Join-Path $analysisRoot 'downloads\UsbDk_1.0.22_x64.msi'
if((Get-FileHash -LiteralPath $msi -Algorithm SHA256).Hash -ne '91F6F695E1E13C656024E6D3B55620BF08D8835EF05EE0496935BA6BB62466A5'){throw 'Unexpected UsbDk package hash.'}
$helper=Join-Path $native 'build\asio\UsbDkHelper.dll'
$signature=Get-AuthenticodeSignature -LiteralPath $helper
if($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notlike '*Red Hat*'){throw 'UsbDk helper signature invalid.'}
New-Item -ItemType Directory -Path $dist,$stage -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $stage 'native') -Force | Out-Null
foreach($folder in @('asio','core','transport','third_party','installer')) {
 Copy-Item -LiteralPath (Join-Path $native $folder) -Destination (Join-Path $stage 'native') -Recurse -Force
}
# Explicit runtime adapter dependencies; no research/vendor driver binaries or unsigned kernel.
New-Item -ItemType Directory -Path (Join-Path $stage 'native\kmdf') -Force | Out-Null
foreach($name in @('usbdk_backend.h','mbox_ioctl.h')) {
 Copy-Item -LiteralPath (Join-Path $native "kmdf\$name") -Destination (Join-Path $stage "native\kmdf\$name") -Force
}
Copy-Item -LiteralPath (Join-Path $native 'COPYING') -Destination (Join-Path $stage 'native\COPYING') -Force
$archive=Join-Path $dist 'NimBox-1-ASIO-0.1.0-source.zip'
Compress-Archive -Path (Join-Path $stage 'native') -DestinationPath $archive -Force
& (Join-Path $analysisRoot 'tools\inno7\ISCC.exe') (Join-Path $PSScriptRoot 'NimBox.iss')
if($LASTEXITCODE -ne 0){throw 'Installer compilation failed.'}
Get-FileHash -LiteralPath (Join-Path $dist 'NimBox-1-ASIO-0.1.0-x64-setup.exe'),$archive -Algorithm SHA256
