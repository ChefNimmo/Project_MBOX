@echo off
setlocal
set "NIM_ROOT=%~dp0.."
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist "%NIM_ROOT%\build\asio" mkdir "%NIM_ROOT%\build\asio"
pushd "%NIM_ROOT%\build\asio"
cl /nologo /LD /MT /EHsc /std:c++17 /O2 /W4 /WX /wd4505 /analyze /analyze:external- /external:anglebrackets /external:W0 "%~dp0Driver.cpp" "%~dp0Engine.cpp" /link /OUT:NimBoxAsio.dll /DEF:"%~dp0NimBox.def" advapi32.lib ole32.lib user32.lib avrt.lib
if errorlevel 1 exit /b 1
cl /nologo /MT /EHsc /std:c++17 /O2 /W4 /WX "%~dp0Probe.cpp" /Fe:NimBoxAsioProbe.exe /link ole32.lib user32.lib
set "NIM_RESULT=%errorlevel%"
popd
exit /b %NIM_RESULT%


