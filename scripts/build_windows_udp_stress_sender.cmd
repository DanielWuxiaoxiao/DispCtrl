@echo off
setlocal

set "VSDEVCMD=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
if not exist "%VSDEVCMD%" (
    echo VsDevCmd.bat not found: %VSDEVCMD%
    exit /b 1
)

call "%VSDEVCMD%" -arch=x64 >nul 2>nul
if errorlevel 1 (
    echo Failed to initialize Visual Studio build environment.
    exit /b 1
)

cl /nologo /std:c++17 /EHsc /O2 /Fe:"%~dp0windows_udp_stress_sender.exe" "%~dp0windows_udp_stress_sender.cpp"
exit /b %errorlevel%
