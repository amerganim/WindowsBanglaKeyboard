@echo off
REM Build the engine + tests (and optionally the TIP) using the VS Build Tools
REM toolchain. Usage: scripts\build.bat [x64|x86|arm64] [extra cmake args...]
REM (arm64 cross-compiles from an x64 host; needs the "MSVC ARM64 build tools"
REM VS component installed.)
setlocal enabledelayedexpansion

set ROOT=%~dp0..
set ARCH=%1
if "%ARCH%"=="" set ARCH=x64

set VCROOT=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools
set VCVARS=%VCROOT%\VC\Auxiliary\Build\vcvarsall.bat
set CMAKE=%VCROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe

REM Map target arch to the vcvarsall argument (cross-compile arm64 from x64).
set VCARCH=%ARCH%
set VCVERARG=
if /I "%ARCH%"=="arm64" (
  set VCARCH=x64_arm64
  REM The ARM64 cross tools may be a different MSVC toolset version than the
  REM default; find the one that actually has an arm64 compiler and pin to it.
  for /d %%V in ("%VCROOT%\VC\Tools\MSVC\*") do (
    if exist "%%V\bin\Hostx64\arm64\cl.exe" set VCVERARG=-vcvars_ver=%%~nxV
  )
)

REM Collect any remaining args (e.g. -DBUILD_TIP=ON), quoting each so cmd does
REM not split them at '='.
set EXTRA=
:collect
shift
if "%~1"=="" goto done
set EXTRA=!EXTRA! "%~1"
goto collect
:done

call "%VCVARS%" %VCARCH% %VCVERARG%
if errorlevel 1 exit /b 1

set BUILDDIR=%ROOT%\build-%ARCH%
"%CMAKE%" -S "%ROOT%" -B "%BUILDDIR%" -G Ninja -DCMAKE_BUILD_TYPE=Release %EXTRA%
if errorlevel 1 exit /b 1

"%CMAKE%" --build "%BUILDDIR%"
if errorlevel 1 exit /b 1

endlocal
