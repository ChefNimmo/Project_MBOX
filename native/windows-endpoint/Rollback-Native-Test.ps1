# Elevated recovery helper. Run only when rollback is requested; never called by a build.
# Removes only NimBox native test components, leaving the existing ASIO and UsbDk installations.
$ErrorActionPreference='Stop'
$root='C:\Users\chefn\Documents\Codex\Mbox-Analysis'
$thumb='56F7A692B142A98E597BAAE174825438D5E09065'
$log=Join-Path $root 'work\native-test-rollback.log'
Start-Transcript -Path $log -Append | Out-Null
try {
    $service=Get-CimInstance Win32_Service -Filter "Name='NimBoxAudio'"
    if($service) {
        if($service.PathName -notlike '*NimBoxAudioBroker.exe*'){throw 'Unexpected NimBoxAudio service binary; refusing removal'}
        Stop-Service NimBoxAudio -Force -ErrorAction Stop
        & sc.exe delete NimBoxAudio
        if($LASTEXITCODE){throw 'Could not remove NimBox broker service'}
    }
    $devcon="${env:ProgramFiles(x86)}\Windows Kits\10\Tools\10.0.26100.0\x64\devcon.exe"
    & $devcon remove 'Root\NimBoxAudio'
    if($LASTEXITCODE -gt 1){throw 'Could not remove native NimBox root device'}
    # OEM numbers are reused: identify packages by BOTH original INF and provider.
    $packages=Get-WindowsDriver -Online -All | Where-Object {
        [IO.Path]::GetFileName($_.OriginalFileName) -ieq 'NimBoxAudio.inf' -and
        $_.ProviderName -eq 'NimBox Project'
    }
    foreach($package in $packages) {
        & pnputil.exe /delete-driver $package.Driver /uninstall
        if($LASTEXITCODE -notin @(0,3010)){throw "Could not remove $($package.Driver)"}
    }
    # The saved pre-test BCD entry had no explicit testsigning setting.
    $current=& bcdedit.exe /enum '{current}' 2>&1
    if($LASTEXITCODE){throw 'Cannot read boot entry'}
    if(($current | Out-String) -match '(?im)^testsigning\s+') {
        & bcdedit.exe /deletevalue '{current}' testsigning
        if($LASTEXITCODE){throw 'Could not restore testsigning default'}
    }
    foreach($store in @('Cert:\LocalMachine\Root','Cert:\LocalMachine\TrustedPublisher')) {
        $path="$store\$thumb"
        if(Test-Path $path){Remove-Item -LiteralPath $path}
    }
    Write-Output 'NimBox native test components removed. Restart manually to restore normal boot signing policy.'
} finally { Stop-Transcript | Out-Null }
