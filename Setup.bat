@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install-Dependencies.ps1" -InstallRuntime
if errorlevel 1 goto failed
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install-Dependencies.ps1"
if errorlevel 1 goto failed
goto done
:failed
echo Dependency setup did not complete. Read the message above.
:done
pause
