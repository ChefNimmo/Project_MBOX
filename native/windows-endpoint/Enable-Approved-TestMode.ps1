# Run elevated only after explicit user approval. Does not restart or load the kernel driver.
$ErrorActionPreference='Stop'
$root='C:\Users\chefn\Documents\Codex\Mbox-Analysis'
$package=Join-Path $root 'dist\NimBox-1-native-0.2.0-dev-testsigned-x64.zip'
$stage=Join-Path $root 'work\native-testsigned-20261104-163652'
$log=Join-Path $root 'work\native-testmode-preparation.json'
$thumb='56F7A692B142A98E597BAAE174825438D5E09065'
$result=[ordered]@{Time=(Get-Date).ToString('o');UserApproved=$true;Restarted=$false;DriverLoaded=$false;Actions=@()}
try {
    if((Get-FileHash $package -Algorithm SHA256).Hash -ne '034AA82621FE4D5A6B9D0F0F2D7C46341CE2740BCF10A0BF1A248FE3A3163DD3'){throw 'Unexpected development package hash'}
    $certPath=Join-Path $stage 'NimBoxDevelopmentTest.cer'
    $certificate=New-Object Security.Cryptography.X509Certificates.X509Certificate2($certPath)
    if($certificate.Thumbprint -ne $thumb -or $certificate.Subject -ne 'CN=NimBox Development Test'){throw 'Unexpected signing certificate'}
    $kernel=Join-Path $stage 'Driver\NimBoxAudio.sys'
    if((Get-AuthenticodeSignature $kernel).SignerCertificate.Thumbprint -ne $thumb){throw 'Unexpected kernel signer'}
    foreach($entry in (Get-Content "$stage\testsigned-manifest.json" -Raw | ConvertFrom-Json)) {
        if((Get-FileHash (Join-Path "$stage\Driver" $entry.File) -Algorithm SHA256).Hash -ne $entry.SHA256){throw "Driver file hash mismatch: $($entry.File)"}
    }
    $secure=Confirm-SecureBootUEFI
    $result.SecureBoot=$secure
    if($secure){throw 'Secure Boot is now enabled; no firmware setting will be changed'}
    $volume=Get-BitLockerVolume -MountPoint $env:SystemDrive
    $result.BitLockerProtection=[string]$volume.ProtectionStatus
    if([string]$volume.ProtectionStatus -ne 'Off'){throw 'BitLocker protection is active or uncertain; boot change stopped without suspending protection'}
    $before=& bcdedit.exe /enum '{current}' 2>&1
    if($LASTEXITCODE){throw 'Cannot read current boot entry'}
    $before | Set-Content (Join-Path $root 'work\native-bcd-before-testmode.txt')
    $result.CertificateExistedBefore=@{}
    foreach($store in @('Cert:\LocalMachine\Root','Cert:\LocalMachine\TrustedPublisher')) {
        $result.CertificateExistedBefore[$store]=Test-Path "$store\$thumb"
        if(!(Test-Path "$store\$thumb")) {
            Import-Certificate -FilePath $certPath -CertStoreLocation $store | Out-Null
            $result.Actions+="Imported reviewed NimBox certificate to $store"
        }
    }
    if((Get-AuthenticodeSignature $kernel).Status -ne 'Valid'){throw 'Kernel Authenticode validation failed after certificate trust'}
    $change=& bcdedit.exe /set '{current}' testsigning on 2>&1
    $result.BcdChangeOutput=($change | Out-String).Trim()
    if($LASTEXITCODE){throw 'BCDEdit did not enable test signing'}
    $after=& bcdedit.exe /enum '{current}' 2>&1
    if($LASTEXITCODE){throw 'Cannot verify boot configuration'}
    $after | Set-Content (Join-Path $root 'work\native-bcd-after-testmode.txt')
    if(($after | Out-String) -notmatch '(?im)^testsigning\s+Yes\s*$'){throw 'Expected testsigning Yes readback absent'}
    $result.Actions+='Enabled TESTSIGNING for current boot entry; restart required'
    # Stage the reviewed package only. No /install, root-device creation or service startup.
    $stageOutput=& pnputil.exe /add-driver "$stage\Driver\NimBoxAudio.inf" 2>&1
    $result.StageExitCode=$LASTEXITCODE
    $stageOutput | Set-Content (Join-Path $root 'work\native-driver-staging.txt')
    if($result.StageExitCode -ne 0){throw 'Driver staging failed; see native-driver-staging.txt'}
    $result.Actions+='Staged native driver package without installing a device'
    $result.Success=$true
} catch {
    $result.Success=$false;$result.Error=$_.Exception.Message
} finally {
    $result | ConvertTo-Json -Depth 6 | Set-Content $log
}
if(!$result.Success){exit 1}
