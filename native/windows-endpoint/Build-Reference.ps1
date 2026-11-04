param([string]$SampleRoot = "$PSScriptRoot\..\..\work\windows-driver-samples\audio\sysvad", [switch]$NimBox)
$ErrorActionPreference = 'Stop'
if ($NimBox -and !$PSBoundParameters.ContainsKey('SampleRoot')) { $SampleRoot = "$PSScriptRoot\sysvad" }
$SampleRoot = (Resolve-Path $SampleRoot).Path
$out = [IO.Path]::GetFullPath("$PSScriptRoot\..\build\endpoint-reference")
if ($NimBox) { $out = [IO.Path]::GetFullPath("$PSScriptRoot\..\build\windows-endpoint") }
New-Item -ItemType Directory -Force $out | Out-Null
$kits = "${env:ProgramFiles(x86)}\Windows Kits\10"
$sdk = '10.0.26100.0'
$vs = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools"
$includes = @("$kits\Include\$sdk\km", "$kits\Include\$sdk\km\crt", "$kits\Include\$sdk\shared", "$kits\Include\$sdk\um", "$kits\Include\wdf\kmdf\1.35", $PSScriptRoot, $SampleRoot, "$SampleRoot\EndpointsCommon", "$SampleRoot\TabletAudioSample", "$SampleRoot\..\..\..\wil\include")
$flags = '/nologo /c /kernel /std:c++17 /Zl /GR- /GS /Zp8 /O2 /W4 /D_AMD64_ /DAMD64 /D_WIN64 /D_WIN32 /DUNICODE /D_UNICODE /DPC_IMPLEMENTATION /D_USE_WAVERT_ /D_NEW_DELETE_OPERATORS_ /D_USE_IPortClsRuntimePower /DSYSVAD_BTH_BYPASS /DSYSVAD_USB_SIDEBAND /DNTDDI_VERSION=0x0A00000C /D_WIN32_WINNT=0x0A00 /DKMDF_VERSION_MAJOR=1 /DKMDF_VERSION_MINOR=35'
$flags += ' /WX /external:W0 ' + (($includes | ForEach-Object {
    if ($_.StartsWith($kits,[StringComparison]::OrdinalIgnoreCase)) { '/external:I"' + $_ + '"' }
    else { '/I"' + $_ + '"' }
}) -join ' ')
if ($NimBox) { $flags = $flags.Replace('/DSYSVAD_BTH_BYPASS','') }
$commands = @('@echo off', 'setlocal', "call `"$vs\VC\Auxiliary\Build\vcvars64.bat`"", 'if errorlevel 1 exit /b 1')
$objects = @()
foreach ($project in @('EndpointsCommon', 'TabletAudioSample')) {
    [xml]$xml = Get-Content "$SampleRoot\$project\$project.vcxproj"
    foreach ($item in $xml.Project.ItemGroup.ClCompile) {
        if (!$item.Include) { continue }
        $file = [IO.Path]::GetFullPath("$SampleRoot\$project\$($item.Include)")
        $obj = "$out\$project-$([IO.Path]::GetFileNameWithoutExtension($file)).obj"
        $objects += '"' + $obj + '"'
        $commands += "cl $flags /Fo`"$obj`" `"$file`""
        $commands += 'if errorlevel 1 exit /b 1'
    }
}
if ($NimBox) {
    $commands += "cl $flags /analyze /analyze:external- /Fo`"$out\NimBridge.obj`" `"$PSScriptRoot\NimBridge.cpp`""
    $commands += 'if errorlevel 1 exit /b 1'
    $objects += '"' + $out + '\NimBridge.obj"'
}
$target = if ($NimBox) { 'NimBoxAudio.sys' } else { 'SysvadReference.sys' }
$commands += 'link /nologo /DRIVER /SUBSYSTEM:NATIVE /ENTRY:FxDriverEntry /NODEFAULTLIB /INTEGRITYCHECK /DYNAMICBASE /NXCOMPAT /MACHINE:X64 /OUT:"' + $out + '\' + $target + '" /LIBPATH:"' + $kits + '\Lib\' + $sdk + '\km\x64" /LIBPATH:"' + $kits + '\Lib\wdf\kmdf\x64\1.35" ' + ($objects -join ' ') + ' portcls.lib stdunk.lib libcntpr.lib ntoskrnl.lib hal.lib BufferOverflowFastFailK.lib WdfDriverEntry.lib WdfLdr.lib ks.lib wdmsec.lib'
$commands += 'exit /b %errorlevel%'
$batch = "$out\build-reference.cmd"
[IO.File]::WriteAllLines($batch, $commands, [Text.Encoding]::ASCII)
& $env:ComSpec /d /c $batch
if ($LASTEXITCODE) { throw "Reference build failed: $LASTEXITCODE" }
Write-Host "Compiled $target only. No installation or hardware claim is performed."

