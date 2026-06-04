@echo off
REM Build the engine + tests (and optionally the TIP) using whatever Visual
REM Studio (Build Tools, Community, or a CI runner's) is installed.
REM Usage: scripts\build.bat [x64|x86|arm64] [extra cmake args...]
REM (arm64 cross-compiles from an x64 host; needs the "MSVC ARM64 build tools".)
setlocal enabledelayedexpansion

set ROOT=%~dp0..
set ARCH=%1
if "%ARCH%"=="" set ARCH=x64

REM --- locate Visual Studio via vswhere (portable across machines/CI) ---
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" ( echo ERROR: vswhere.exe not found. & exit /b 1 )
set "VSINSTALL="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"
if not defined VSINSTALL ( echo ERROR: Visual Studio with C++ tools not found. & exit /b 1 )
set "VCVARS=%VSINSTALL%\VC\Auxiliary\Build\vcvarsall.bat"

REM --- cmake: prefer one on PATH (CI), else the VS-bundled copy ---
set "CMAKE=cmake"
where cmake >nul 2>nul || set "CMAKE=%VSINSTALL%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

REM --- target arch -> vcvarsall argument; pin arm64 to the toolset that has it ---
set VCARCH=%ARCH%
set VCVERARG=
if /I "%ARCH%"=="arm64" (
  set VCARCH=x64_arm64
  for /d %%V in ("%VSINSTALL%\VC\Tools\MSVC\*") do (
    if exist "%%V\bin\Hostx64\arm64\cl.exe" set VCVERARG=-vcvars_ver=%%~nxV
  )
)

REM --- collect any remaining args (e.g. -DBUILD_TIP=ON), quoting each ---
set EXTRA=
:collect
shift
if "%~1"=="" goto done
set EXTRA=!EXTRA! "%~1"
goto collect
:done

call "%VCVARS%" %VCARCH% %VCVERARG%
if errorlevel 1 exit /b 1

set "BUILDDIR=%ROOT%\build-%ARCH%"
"%CMAKE%" -S "%ROOT%" -B "%BUILDDIR%" -G Ninja -DCMAKE_BUILD_TYPE=Release %EXTRA%
if errorlevel 1 exit /b 1

"%CMAKE%" --build "%BUILDDIR%"
if errorlevel 1 exit /b 1

endlocal
