@echo off
setlocal
set CSC=%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe
if not exist "%CSC%" set CSC=%WINDIR%\Microsoft.NET\Framework\v4.0.30319\csc.exe
if not exist "%CSC%" (
  echo [ERROR] .NET Framework C# compiler not found.
  exit /b 1
)
"%CSC%" /nologo /codepage:65001 /warn:4 /target:winexe /optimize+ /platform:anycpu /out:CIMC_DebugTool_ADC修复版.exe /reference:System.dll /reference:System.Core.dll /reference:System.Drawing.dll /reference:System.Windows.Forms.dll /reference:System.Windows.Forms.DataVisualization.dll src\Program.cs src\Protocol.cs src\SerialTransport.cs src\MainForm.cs
if errorlevel 1 exit /b 1
echo [OK] CIMC_DebugTool_ADC修复版.exe built with UTF-8 source encoding.
endlocal
