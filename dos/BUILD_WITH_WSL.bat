@echo off
setlocal
rem Builds TOMB.COM from the assembly source using WSL (Windows Subsystem for Linux).
cd /d "%~dp0"
wsl.exe --cd "%CD%" --exec make
if errorlevel 1 goto build_failed
echo Build succeeded.
pause
exit /b 0

:build_failed
echo Build failed.
echo Make sure WSL is installed and binutils, make, and python3 are available in its Linux distribution.
pause
exit /b 1
