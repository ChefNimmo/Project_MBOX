param([string]$OutputPath)
$ErrorActionPreference='Stop'
if(-not $OutputPath){throw 'Choose an output JSON file.'}
$devices=@(Get-PnpDevice -PresentOnly | Where-Object {$_.InstanceId -match 'VID_0DBA'})
$rows=@(foreach($device in $devices){
 $properties=@(Get-PnpDeviceProperty -InstanceId $device.InstanceId -ErrorAction SilentlyContinue |
  Where-Object {$_.KeyName -match 'HardwareIds|CompatibleIds|BusReportedDeviceDesc|DriverVersion|DriverDate|DriverProvider|DriverInfPath|ProblemCode|Service|LocationInfo|Parent|ContainerId'} |
  Select-Object KeyName,Type,Data)
 [pscustomobject]@{Status=$device.Status;Class=$device.Class;FriendlyName=$device.FriendlyName;InstanceId=$device.InstanceId;Properties=$properties}
})
[pscustomobject]@{CollectedUtc=[DateTime]::UtcNow.ToString('o');PresentDeviceCount=$devices.Count;Devices=$rows;Scope='Read-only PnP metadata. No USB control requests or driver changes.'} |
 ConvertTo-Json -Depth 9 | Set-Content -LiteralPath $OutputPath
Write-Output "Saved read-only metadata for $($devices.Count) Digidesign USB device(s)."
