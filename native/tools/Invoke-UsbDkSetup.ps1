param([ValidateSet('Plan','Install','Remove')][string]$Action='Plan')
$ErrorActionPreference='Stop'
$analysisRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$package=Join-Path $analysisRoot 'downloads\UsbDk_1.0.22_x64.msi'
$expected='91F6F695E1E13C656024E6D3B55620BF08D8835EF05EE0496935BA6BB62466A5'
if((Get-FileHash -LiteralPath $package -Algorithm SHA256).Hash -ne $expected){throw 'Package hash does not match the verified official release.'}
$signature=Get-AuthenticodeSignature -LiteralPath $package
if($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notlike '*Red Hat*'){throw 'Official package signature validation failed.'}
if($Action -eq 'Plan') {
 Write-Output 'Official UsbDk 1.0.22 x64: signed by Red Hat; MSI SHA256 verified.'
 Write-Output 'Install adds a USB-class UpperFilters entry and resets USB host controllers.'
 Write-Output 'USB devices can disconnect temporarily. Finish disk transfers first.'
 Write-Output 'No reboot is forced; a restart may be required. No boot-security change is made.'
 Write-Output 'Ploytec packages are retained. Use -Action Remove for the official MSI uninstall.'
 return
}
$identity=[Security.Principal.WindowsIdentity]::GetCurrent()
$principal=[Security.Principal.WindowsPrincipal]::new($identity)
if(!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){throw 'Run the approved setup action from an elevated PowerShell process.'}
$log=Join-Path $analysisRoot ('work\usbdk-'+$Action.ToLowerInvariant()+'.log')
if($Action -eq 'Install') {
 $key='HKLM:\SYSTEM\CurrentControlSet\Control\Class\{36fc9e60-c465-11cf-8056-444553540000}'
 $item=Get-ItemProperty -LiteralPath $key
 $property=$item.PSObject.Properties['UpperFilters']
 [pscustomobject]@{Captured=(Get-Date).ToUniversalTime().ToString('o');UpperFiltersExisted=($null -ne $property);UpperFilters=@($property.Value)} |
  ConvertTo-Json | Set-Content -LiteralPath (Join-Path $analysisRoot 'work\usb-upper-filters-before.json') -Encoding UTF8
 $operation='/i'
} else {$operation='/x'}
$process=Start-Process -FilePath msiexec.exe -ArgumentList @($operation,('"'+$package+'"'),'/qn','/norestart','/L*v',('"'+$log+'"')) -WindowStyle Hidden -Wait -PassThru
Write-Output "UsbDk $Action MSI exit: $($process.ExitCode). Log: $log"
if($process.ExitCode -eq 3010){Write-Output 'Restart required. No automatic reboot performed.'}
elseif($process.ExitCode -ne 0){throw "MSI $Action failed with exit $($process.ExitCode); inspect the log and device state."}
