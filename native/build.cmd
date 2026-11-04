@echo off
setlocal
set "MBOX_ROOT=%~dp0"
set "MBOX_KITS=%ProgramFiles(x86)%\Windows Kits\10"
set "MBOX_SDK=10.0.26100.0"
set "MBOX_WDF=1.15"
set "MBOX_VS=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools"
if not exist "%MBOX_ROOT%build" mkdir "%MBOX_ROOT%build"
call "%MBOX_VS%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
cl /nologo /W4 /WX /std:c17 /Od /Zi /Fe:"%MBOX_ROOT%build\test_stream.exe" /Fo:"%MBOX_ROOT%build\test_stream.obj" /Fd:"%MBOX_ROOT%build\test_stream.pdb" "%MBOX_ROOT%tests\test_stream.c"
if errorlevel 1 exit /b 1
"%MBOX_ROOT%build\test_stream.exe"
if errorlevel 1 exit /b 1
cl /nologo /c /kernel /analyze /analyze:external- /external:W0 /W4 /WX /wd4201 /wd4324 /std:c17 /GS /Zl /Zp8 /O2 /Zi /D_AMD64_ /DAMD64 /D_WIN32_WINNT=0x0A00 /DNTDDI_VERSION=0x0A000000 /DKMDF_VERSION_MAJOR=1 /DKMDF_VERSION_MINOR=15 /external:I"%MBOX_KITS%\Include\%MBOX_SDK%\km" /external:I"%MBOX_KITS%\Include\%MBOX_SDK%\shared" /external:I"%MBOX_KITS%\Include\wdf\kmdf\%MBOX_WDF%" /Fo:"%MBOX_ROOT%build\driver.obj" /Fd:"%MBOX_ROOT%build\driver.pdb" "%MBOX_ROOT%kmdf\driver.c"
if errorlevel 1 exit /b 1
link /nologo /DRIVER /SUBSYSTEM:NATIVE /ENTRY:FxDriverEntry /NODEFAULTLIB /INTEGRITYCHECK /DYNAMICBASE /NXCOMPAT /MACHINE:X64 /OSVERSION:10.0 /VERSION:0.1 /DEBUG /OUT:"%MBOX_ROOT%build\MboxResearch.sys" /PDB:"%MBOX_ROOT%build\MboxResearch.pdb" /LIBPATH:"%MBOX_KITS%\Lib\%MBOX_SDK%\km\x64" /LIBPATH:"%MBOX_KITS%\Lib\wdf\kmdf\x64\%MBOX_WDF%" "%MBOX_ROOT%build\driver.obj" WdfDriverEntry.lib WdfLdr.lib ntoskrnl.lib hal.lib BufferOverflowFastFailK.lib
if errorlevel 1 exit /b 1
cl /nologo /W4 /WX /wd4505 /std:c17 /O2 /Fe:"%MBOX_ROOT%build\mbox_probe.exe" /Fo:"%MBOX_ROOT%build\probe.obj" "%MBOX_ROOT%kmdf\probe.c" setupapi.lib
if errorlevel 1 exit /b 1
"%MBOX_KITS%\Tools\%MBOX_SDK%\x64\infverif.exe" /w /v "%MBOX_ROOT%kmdf\MboxResearch.inf"
if errorlevel 1 exit /b 1
echo Built unsigned experimental transport and probe. This script does not install them.



