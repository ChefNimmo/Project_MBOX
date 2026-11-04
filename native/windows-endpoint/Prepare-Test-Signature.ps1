# Creates a LOCAL development signature only. Does not trust a certificate or alter boot policy.
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath("$PSScriptRoot\..\..")
$tag=Get-Date -Format 'yyyyMMdd-HHmmss'
$stage=Join-Path $root "work\native-testsigned-$tag"
$keys=Join-Path $root "work\private-nimbox-signing-$tag"
New-Item -ItemType Directory -Force $stage,$keys | Out-Null
# Restrict private signing material to this user, SYSTEM and administrators.
$acl=Get-Acl $keys
$acl.SetAccessRuleProtection($true,$false)
foreach($sid in @([Security.Principal.WindowsIdentity]::GetCurrent().User.Value,'S-1-5-18','S-1-5-32-544')) {
    $rule=[Security.AccessControl.FileSystemAccessRule]::new([Security.Principal.SecurityIdentifier]::new($sid),'FullControl','ContainerInherit,ObjectInherit','None','Allow')
    $acl.AddAccessRule($rule)
}
Set-Acl $keys $acl
$rsa=[Security.Cryptography.RSA]::Create(3072)
$request=[Security.Cryptography.X509Certificates.CertificateRequest]::new('CN=NimBox Development Test',$rsa,[Security.Cryptography.HashAlgorithmName]::SHA256,[Security.Cryptography.RSASignaturePadding]::Pkcs1)
$request.CertificateExtensions.Add([Security.Cryptography.X509Certificates.X509BasicConstraintsExtension]::new($false,$false,0,$true))
$request.CertificateExtensions.Add([Security.Cryptography.X509Certificates.X509KeyUsageExtension]::new([Security.Cryptography.X509Certificates.X509KeyUsageFlags]::DigitalSignature,$true))
$oids=[Security.Cryptography.OidCollection]::new()
[void]$oids.Add([Security.Cryptography.Oid]::new('1.3.6.1.5.5.7.3.3'))
$request.CertificateExtensions.Add([Security.Cryptography.X509Certificates.X509EnhancedKeyUsageExtension]::new($oids,$false))
$certificate=$request.CreateSelfSigned([DateTimeOffset]::Now.AddMinutes(-5),[DateTimeOffset]::Now.AddDays(90))
$passwordBytes=[byte[]]::new(32);[Security.Cryptography.RandomNumberGenerator]::Fill($passwordBytes)
$password=[Convert]::ToBase64String($passwordBytes)
$pfx=Join-Path $keys 'NimBoxDevelopmentTest.pfx'
[IO.File]::WriteAllBytes($pfx,$certificate.Export([Security.Cryptography.X509Certificates.X509ContentType]::Pfx,$password))
$protected=[Security.Cryptography.ProtectedData]::Protect([Text.Encoding]::UTF8.GetBytes($password),$null,[Security.Cryptography.DataProtectionScope]::CurrentUser)
[IO.File]::WriteAllBytes((Join-Path $keys 'password.dpapi'),$protected)
Expand-Archive (Join-Path $root 'dist\NimBox-1-native-0.2.0-dev-x64.zip') -DestinationPath $stage
$cer=Join-Path $stage 'NimBoxDevelopmentTest.cer'
[IO.File]::WriteAllBytes($cer,$certificate.Export([Security.Cryptography.X509Certificates.X509ContentType]::Cert))
$kits="${env:ProgramFiles(x86)}\Windows Kits\10"
$sign="$kits\bin\10.0.26100.0\x64\signtool.exe"
& $sign sign /fd SHA256 /f $pfx /p $password "$stage\Driver\NimBoxAudio.sys"
if($LASTEXITCODE){throw 'Test signing kernel binary failed'}
& "$kits\bin\10.0.26100.0\x86\Inf2Cat.exe" "/driver:$stage\Driver" /os:10_GE_X64 /uselocaltime
if($LASTEXITCODE){throw 'Regenerating test catalogue failed'}
& $sign sign /fd SHA256 /f $pfx /p $password "$stage\Driver\NimBoxAudio.cat"
if($LASTEXITCODE){throw 'Test signing catalogue failed'}
# Verify the catalogue's cryptographic signature without trusting its self-signed chain.
Add-Type -AssemblyName System.Security.Cryptography.Pkcs
$cms=[Security.Cryptography.Pkcs.SignedCms]::new()
$cms.Decode([IO.File]::ReadAllBytes("$stage\Driver\NimBoxAudio.cat"))
$cms.CheckSignature($true)
if($cms.SignerInfos[0].Certificate.Thumbprint -ne $certificate.Thumbprint){throw 'Unexpected catalogue signer'}
[ordered]@{Subject=$certificate.Subject;Thumbprint=$certificate.Thumbprint;Expires=$certificate.NotAfter.ToString('o');Trusted=$false;BootChanged=$false;CatalogueCryptographicSignature='verified without trust';PrivateKeyIncluded=$false} |
    ConvertTo-Json | Set-Content "$stage\TEST-SIGNATURE.json"
Copy-Item $PSCommandPath "$stage\Prepare-Test-Signature.ps1"
@'
This package has a LOCAL self-signed development signature, NOT Microsoft approval.
The signing certificate is NOT trusted on the PC. Boot policy has NOT been changed.
Do not install before agreeing a controlled test route with the user.
Private keys/passwords are NOT in this package. The public test certificate is included
for review. It expires in 90 days and is not suitable for public driver distribution.
The unsigned manifest is superseded by testsigned-manifest.json for Driver files.
'@ | Set-Content "$stage\TEST-SIGNATURE-NOTES.txt"
Get-ChildItem "$stage\Driver" -File | ForEach-Object {@{File=$_.Name;SHA256=(Get-FileHash $_.FullName -Algorithm SHA256).Hash}} |
    ConvertTo-Json | Set-Content "$stage\testsigned-manifest.json"
$package=Join-Path $root 'dist\NimBox-1-native-0.2.0-dev-testsigned-x64.zip'
Compress-Archive "$stage\*" -DestinationPath $package -Force
$certificate.Dispose();$rsa.Dispose();$password=$null
Get-Content "$stage\TEST-SIGNATURE.json"
Get-FileHash $package -Algorithm SHA256
