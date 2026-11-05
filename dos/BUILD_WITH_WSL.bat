@echo off
rem Builds TOMB.COM from the assembly source using WSL (Windows Subsystem for Linux).
cd /d "%~dp0"
wsl make
if errorlevel 1 (
    echo Build failed. See the errors above.
    pause
    exit /b 1
)
pause
