@echo off
setlocal
set "NIM_ROOT=%~dp0.."
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist "%NIM_ROOT%\build\windows-endpoint" mkdir "%NIM_ROOT%\build\windows-endpoint"
pushd "%NIM_ROOT%\build\windows-endpoint"
cl /nologo /MT /EHsc /std:c++17 /O2 /W4 /WX "%~dp0TestEndpointCore.cpp" /Fe:TestEndpointCore.exe
if errorlevel 1 exit /b 1
TestEndpointCore.exe
if errorlevel 1 exit /b 1
cl /nologo /MT /EHsc /std:c++17 /O2 /W4 /WX "%~dp0ProbeWindows.cpp" /Fe:NimBoxWindowsProbe.exe /link ole32.lib uuid.lib
if errorlevel 1 exit /b 1
cl /nologo /MT /EHsc /std:c++17 /O2 /W4 /WX /wd4505 /analyze /analyze:external- /external:anglebrackets /external:W0 "%~dp0Broker.cpp" "%NIM_ROOT%\asio\Engine.cpp" /Fe:NimBoxAudioBroker.exe /link advapi32.lib avrt.lib
set "NIM_RESULT=%errorlevel%"
popd
exit /b %NIM_RESULT%
