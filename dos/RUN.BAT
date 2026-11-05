@echo off
cd /d "%~dp0"
where dosbox >nul 2>nul && (dosbox -conf tomb.conf & exit /b)
if exist "%ProgramFiles(x86)%\DOSBox-0.74-3\DOSBox.exe" ("%ProgramFiles(x86)%\DOSBox-0.74-3\DOSBox.exe" -conf tomb.conf & exit /b)
if exist "%ProgramFiles%\DOSBox-0.74-3\DOSBox.exe" ("%ProgramFiles%\DOSBox-0.74-3\DOSBox.exe" -conf tomb.conf & exit /b)
if exist "%ProgramFiles%\DOSBox Staging\dosbox.exe" ("%ProgramFiles%\DOSBox Staging\dosbox.exe" -conf tomb.conf & exit /b)
if exist "D:\a\DOSBox-0.74-3\DOSBox.exe" ("D:\a\DOSBox-0.74-3\DOSBox.exe" -conf tomb.conf & exit /b)
echo DOSBox not found. Install it from https://www.dosbox.com and run again,
echo or open DOSBox and type:  mount c "%~dp0"  then  c:  then  TOMB.COM
pause
