@echo off
chcp 65001 >nul
set "PROJECT=%~dp0APP\MDK\CIMC_APP.uvprojx"
set "LOG=%~dp0APP\MDK\build.log"
set "UV4="
for %%P in ("C:\Keil_v5\UV4\UV4.exe" "D:\Keil_v5\UV4\UV4.exe" "C:\Keil\UV4\UV4.exe") do if exist %%P set "UV4=%%~P"
if not defined UV4 for /f "delims=" %%P in ('where UV4.exe 2^>nul') do if not defined UV4 set "UV4=%%P"
if not defined UV4 (
  echo [ERROR] 未找到 Keil UV4.exe。请先离线安装 Keil MDK，并确认 GD32F4xx DFP 3.0.3 已安装。
  pause
  exit /b 2
)
echo 使用: %UV4%
"%UV4%" -r "%PROJECT%" -o "%LOG%"
type "%LOG%"
findstr /C:"0 Error(s)" "%LOG%" >nul
if errorlevel 1 (
  echo [ERROR] 编译未通过，请查看 %LOG%
  pause
  exit /b 1
)
echo [OK] APP 编译通过，输出目录: %~dp0APP\MDK\output
pause
