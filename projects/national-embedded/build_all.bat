@echo off
setlocal
set "UV4=%LOCALAPPDATA%\Keil_v5\UV4\UV4.exe"
if not exist "%UV4%" set "UV4="
if not defined UV4 for %%P in ("C:\Keil_v5\UV4\UV4.exe" "D:\Keil_v5\UV4\UV4.exe") do if exist %%P set "UV4=%%~P"
if not defined UV4 for /f "delims=" %%P in ('where UV4.exe 2^>nul') do if not defined UV4 set "UV4=%%P"
if not defined UV4 (
  echo [ERROR] Keil UV4.exe was not found.
  pause
  exit /b 2
)

echo [1/3] Static checks
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\self_check_ascii.ps1"
if errorlevel 1 goto :failed

echo [2/3] Rebuild Bootloader
"%UV4%" -r "%~dp0BootLoader\MDK\CIMC_BL.uvprojx" -o "%~dp0BootLoader\MDK\build.log"
type "%~dp0BootLoader\MDK\build.log"
findstr /C:"0 Error(s)" "%~dp0BootLoader\MDK\build.log" >nul
if errorlevel 1 goto :failed

echo [3/3] Rebuild APP
"%UV4%" -r "%~dp0APP\MDK\CIMC_APP.uvprojx" -o "%~dp0APP\MDK\build.log"
type "%~dp0APP\MDK\build.log"
findstr /C:"0 Error(s)" "%~dp0APP\MDK\build.log" >nul
if errorlevel 1 goto :failed

echo [OK] Bootloader and APP builds passed.
echo APP outputs: %~dp0APP\MDK\output
echo BL outputs : %~dp0BootLoader\MDK\output
pause
exit /b 0

:failed
echo [ERROR] Build/check failed. Review the log above.
pause
exit /b 1

