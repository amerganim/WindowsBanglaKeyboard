<#
  Bangla Phonetic Keyboard - installer.

  Copies the input-method DLLs to Program Files, registers them with Windows
  (both 64-bit and 32-bit so all apps can use them), and adds an entry to
  "Apps & features". Requires administrator rights and self-elevates.

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

function Test-FileLocked([string]$path) {
    if (-not (Test-Path $path)) { return $false }
    try {
        $fs = [IO.File]::Open($path, 'Open', 'ReadWrite', 'None')
        $fs.Close(); return $false
    } catch { return $true }
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
$Version   = '0.5.1'
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

    $dllX64 = Join-Path $dest 'BanglaPhonetic_x64.dll'
    $dllX86 = Join-Path $dest 'BanglaPhonetic_x86.dll'

    # A previous version's DLL may still be loaded by the text framework, which
    # blocks overwriting it. Detect that and tell the user what to do.
    if ((Test-FileLocked $dllX64) -or ($haveX86 -and (Test-FileLocked $dllX86))) {
        throw ("A previously installed Bangla Phonetic DLL is still in use. " +
               "Please RESTART Windows (or sign out and back in), then run " +
               "install.ps1 again. (No need to uninstall first.)")
    }

    Write-Host "Installing $Display $Version to $dest"
    New-Item -ItemType Directory -Force -Path $dest | Out-Null
    Copy-Item $dllX64src $dllX64 -Force
    if ($haveX86) { Copy-Item $dllX86src $dllX86 -Force }
    Copy-Item (Join-Path $src 'uninstall.ps1') (Join-Path $dest 'uninstall.ps1') -Force

    # --- register (regsvr32 calls our DllRegisterServer) ---
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

    Write-Host ''
    Write-Host "$Display installed." -ForegroundColor Green
    Write-Host 'Switch input methods with Win+Space (look for "Bangla Phonetic").'
    Write-Host 'Toggle Bangla/English with Ctrl+Shift+B.'
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
