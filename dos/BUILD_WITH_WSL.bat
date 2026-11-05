@echo off
rem Builds TOMB.COM from the assembly source using WSL (Windows Subsystem for Linux).
cd /d "%~dp0"
wsl make levels
if errorlevel 1 (
    echo Map generation failed. TOMB.COM was not rebuilt.
    pause
    exit /b 1
)
wsl make
if errorlevel 1 (
    echo Build failed. See the errors above.
    pause
    exit /b 1
)
pause
