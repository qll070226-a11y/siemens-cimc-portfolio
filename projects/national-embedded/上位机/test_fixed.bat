@echo off
setlocal
set CSC=%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe
if not exist "%CSC%" set CSC=%WINDIR%\Microsoft.NET\Framework\v4.0.30319\csc.exe
"%CSC%" /nologo /target:exe /optimize+ /out:ProtocolTests.exe /reference:System.dll /reference:System.Core.dll src\Protocol.cs src\ProtocolTestsFixed.cs
if errorlevel 1 exit /b 1
ProtocolTests.exe
exit /b %errorlevel%
