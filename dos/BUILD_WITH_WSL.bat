@echo off
rem Builds TOMB.COM from the assembly source using WSL (Windows Subsystem for Linux).
cd /d "%~dp0"
wsl make
pause
