<#
  Bangla Phonetic Keyboard - installer.

  Copies the input-method DLLs to Program Files, registers them with Windows
  (both 64-bit and 32-bit so all apps can use them), and adds an entry to
  "Apps & features". Requires administrator rights and self-elevates.

  Each build is installed under a content-hashed filename, so a previously
  loaded DLL never has to be overwritten -- updating does NOT require a restart.

  Run by right-clicking -> "Run with PowerShell", or:
      powershell -ExecutionPolicy Bypass -File install.ps1
#>
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'

# --- self-elevate ---
$principal = New-Object Security.Principal.WindowsPrincipal(
    [Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltinRole]::Administrator)) {
    Write-Host 'Requesting administrator rights...'
    Start-Process powershell.exe -Verb RunAs -ArgumentList @(
        '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`"")
    return
}

function Register-Dll([string]$regsvr, [string]$dll, [string]$label) {
    $p = Start-Process -FilePath $regsvr -ArgumentList @('/s', "`"$dll`"") `
                       -Wait -PassThru -WindowStyle Hidden
    if ($p.ExitCode -ne 0) {
        throw "Registering the $label DLL failed (regsvr32 exit $($p.ExitCode))."
    }
}

$AppName   = 'BanglaPhonetic'
$Display   = 'Bangla Phonetic Keyboard'
$Version   = '0.6.2'
$Publisher = 'WindowsBanglaKeyboard'

$src  = $PSScriptRoot
$dest = Join-Path $env:ProgramFiles $AppName
$log  = Join-Path $env:TEMP 'BanglaPhonetic_install.log'

try { Start-Transcript -Path $log -Force | Out-Null } catch {}

try {
    $dllX64src = Join-Path $src 'BanglaPhonetic_x64.dll'
    $dllX86src = Join-Path $src 'BanglaPhonetic_x86.dll'
    if (-not (Test-Path $dllX64src)) { throw "Missing $dllX64src - run scripts\package.bat first." }
    $haveX86 = Test-Path $dllX86src

    # Content hash -> unique, stable filename per build. A new build gets a new
    # name (no overwrite of a loaded DLL); reinstalling the same build reuses the
    # existing file as-is.
    $hash = (Get-FileHash -Algorithm SHA256 -Path $dllX64src).Hash.Substring(0, 8).ToLower()
    $dllX64 = Join-Path $dest "BanglaPhonetic_x64_$hash.dll"
    $dllX86 = Join-Path $dest "BanglaPhonetic_x86_$hash.dll"

    Write-Host "Installing $Display $Version to $dest"
    New-Item -ItemType Directory -Force -Path $dest | Out-Null
    if (-not (Test-Path $dllX64)) { Copy-Item $dllX64src $dllX64 }
    if ($haveX86 -and -not (Test-Path $dllX86)) { Copy-Item $dllX86src $dllX86 }
    Copy-Item (Join-Path $src 'uninstall.ps1') (Join-Path $dest 'uninstall.ps1') -Force -ErrorAction SilentlyContinue

    # --- register (regsvr32 calls our DllRegisterServer; same CLSID, so this
    # repoints the existing registration at the new file) ---
    Register-Dll (Join-Path $env:WinDir 'System32\regsvr32.exe') $dllX64 '64-bit'
    $regWow = Join-Path $env:WinDir 'SysWOW64\regsvr32.exe'
    if ($haveX86 -and (Test-Path $regWow)) {
        Register-Dll $regWow $dllX86 '32-bit'
    }

    # --- Add/Remove Programs entry ---
    $arp = "HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\$AppName"
    New-Item -Path $arp -Force | Out-Null
    Set-ItemProperty $arp DisplayName     $Display
    Set-ItemProperty $arp DisplayVersion  $Version
    Set-ItemProperty $arp Publisher       $Publisher
    Set-ItemProperty $arp InstallLocation $dest
    Set-ItemProperty $arp UninstallString "powershell.exe -NoProfile -ExecutionPolicy Bypass -File `"$dest\uninstall.ps1`""
    Set-ItemProperty $arp NoModify 1 -Type DWord
    Set-ItemProperty $arp NoRepair 1 -Type DWord

    # --- clean up older builds' DLLs (best effort; loaded ones are skipped and
    # are harmlessly removed after the next restart) ---
    Get-ChildItem $dest -Filter 'BanglaPhonetic_x64*.dll' -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -ne $dllX64 } |
        Remove-Item -Force -ErrorAction SilentlyContinue
    Get-ChildItem $dest -Filter 'BanglaPhonetic_x86*.dll' -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -ne $dllX86 } |
        Remove-Item -Force -ErrorAction SilentlyContinue

    Write-Host ''
    Write-Host "$Display $Version installed (no restart needed)." -ForegroundColor Green
    Write-Host 'Switch input methods with Win+Space (look for "Bangla Phonetic").'
    Write-Host 'Toggle Bangla/English with Ctrl+Shift+B.'
    Write-Host 'If an app was already open, close and reopen it to pick up this version.'
    Write-Host 'If it does not appear, add the Bengali language under Settings > Time and language > Language.'
} catch {
    Write-Host ''
    Write-Host "INSTALL FAILED: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "Details were logged to: $log"
} finally {
    try { Stop-Transcript | Out-Null } catch {}
    Write-Host ''
    Read-Host 'Press Enter to close'
}
