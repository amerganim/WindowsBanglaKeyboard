@echo off
REM Build both architectures of the TIP and stage an installable package under
REM dist\. Run install.ps1 (as admin) from there to install.
setlocal
set ROOT=%~dp0..

call "%~dp0build.bat" x64 "-DBUILD_TIP=ON"
if errorlevel 1 exit /b 1
call "%~dp0build.bat" x86 "-DBUILD_TIP=ON"
if errorlevel 1 exit /b 1

set DIST=%ROOT%\dist
if exist "%DIST%" rmdir /s /q "%DIST%"
mkdir "%DIST%"

copy /y "%ROOT%\build-x64\app\BanglaPhonetic.dll" "%DIST%\BanglaPhonetic_x64.dll" >nul
copy /y "%ROOT%\build-x86\app\BanglaPhonetic.dll" "%DIST%\BanglaPhonetic_x86.dll" >nul
copy /y "%ROOT%\installer\install.ps1"   "%DIST%\install.ps1" >nul
copy /y "%ROOT%\installer\uninstall.ps1" "%DIST%\uninstall.ps1" >nul
copy /y "%ROOT%\installer\README.md"     "%DIST%\README.md" >nul
copy /y "%ROOT%\LICENSE"                 "%DIST%\LICENSE" >nul

echo.
echo Packaged to %DIST%
echo Right-click install.ps1 there and "Run with PowerShell" (it will elevate),
echo or run:  powershell -ExecutionPolicy Bypass -File "%DIST%\install.ps1"
endlocal
