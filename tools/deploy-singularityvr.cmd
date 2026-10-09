@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0deploy-singularityvr.ps1" %*
exit /b %errorlevel%
