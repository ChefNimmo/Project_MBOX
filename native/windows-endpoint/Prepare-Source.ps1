param(
    [string]$Checkout = "$PSScriptRoot\..\..\work\windows-driver-samples",
    [string]$Destination = "$PSScriptRoot\sysvad"
)
$ErrorActionPreference='Stop'
$revision='1fd430c78971c31b624b0773bbea825d8b480d55'
if (!(Test-Path $Checkout)) {
    git clone --filter=blob:none --sparse --no-checkout https://github.com/microsoft/Windows-driver-samples.git $Checkout
    if ($LASTEXITCODE) { throw 'Microsoft source clone failed' }
    git -C $Checkout sparse-checkout set audio/sysvad
    git -C $Checkout checkout --detach $revision
    if ($LASTEXITCODE) { throw 'Pinned source checkout failed' }
}
$head=git -C $Checkout rev-parse HEAD
if ($head -ne $revision) { throw "Expected pinned Microsoft revision $revision; got $head. Existing checkout was not changed." }
if (Test-Path $Destination) { throw 'Destination already exists. Use a fresh destination to preserve local edits.' }
New-Item -ItemType Directory -Force $Destination | Out-Null
foreach ($sub in @('', 'EndpointsCommon', 'TabletAudioSample')) {
    $target=Join-Path $Destination $sub
    New-Item -ItemType Directory -Force $target | Out-Null
    Get-ChildItem (Join-Path "$Checkout\audio\sysvad" $sub) -File |
        Where-Object { $_.Extension -in '.cpp','.h','.vcxproj' } | Copy-Item -Destination $target
}
# Patch paths are relative to the sysvad directory; no absolute paths or deletions.
git -C $Destination apply --check "$PSScriptRoot\NimBox-sysvad.patch"
if ($LASTEXITCODE) { throw 'Patch verification failed' }
git -C $Destination apply "$PSScriptRoot\NimBox-sysvad.patch"
if ($LASTEXITCODE) { throw 'Patch application failed' }
Write-Host "Prepared pinned Microsoft source with NimBox changes at $Destination"
