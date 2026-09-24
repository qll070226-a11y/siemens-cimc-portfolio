@echo off
set "PROJECT=%~dp0APP\MDK\CIMC_APP.uvprojx"
set "LOG=%~dp0APP\MDK\build.log"
set "UV4="
for %%P in ("%LOCALAPPDATA%\Keil_v5\UV4\UV4.exe" "C:\Keil_v5\UV4\UV4.exe" "D:\Keil_v5\UV4\UV4.exe" "C:\Keil\UV4\UV4.exe") do if exist %%P set "UV4=%%~P"
if not defined UV4 for /f "delims=" %%P in ('where UV4.exe 2^>nul') do if not defined UV4 set "UV4=%%P"
if not defined UV4 (
  echo [ERROR] Keil UV4.exe was not found. Install Keil MDK and GD32F4xx DFP 3.0.3 offline.
  pause
  exit /b 2
)
echo Using: %UV4%
"%UV4%" -r "%PROJECT%" -o "%LOG%"
type "%LOG%"
findstr /C:"0 Error(s)" "%LOG%" >nul
if errorlevel 1 (
  echo [ERROR] Build failed. See %LOG%
  pause
  exit /b 1
)
echo [OK] APP build passed. Output: %~dp0APP\MDK\output
pause
