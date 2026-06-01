<#
  Bangla Phonetic Keyboard - installer.

  Copies the input-method DLLs to Program Files, registers them with Windows
  (both 64-bit and 32-bit so all apps can use them), and adds an entry to
  "Apps & features" / "Add or remove programs". Requires administrator rights
  and self-elevates if not already elevated.

  Run by double-clicking, or:  powershell -ExecutionPolicy Bypass -File install.ps1
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

$AppName  = 'BanglaPhonetic'
$Display  = 'Bangla Phonetic Keyboard'
$Version  = '0.5.0'
$Publisher = 'WindowsBanglaKeyboard'

$src  = $PSScriptRoot
$dest = Join-Path $env:ProgramFiles $AppName

$dllX64 = Join-Path $src 'BanglaPhonetic_x64.dll'
$dllX86 = Join-Path $src 'BanglaPhonetic_x86.dll'
if (-not (Test-Path $dllX64)) { throw "Missing $dllX64 - run scripts\package.bat first." }
$haveX86 = Test-Path $dllX86

Write-Host "Installing $Display $Version to $dest"
New-Item -ItemType Directory -Force -Path $dest | Out-Null
Copy-Item $dllX64 (Join-Path $dest 'BanglaPhonetic_x64.dll') -Force
if ($haveX86) { Copy-Item $dllX86 (Join-Path $dest 'BanglaPhonetic_x86.dll') -Force }
Copy-Item (Join-Path $src 'uninstall.ps1') (Join-Path $dest 'uninstall.ps1') -Force

# --- register (regsvr32 calls our DllRegisterServer) ---
$regSys = Join-Path $env:WinDir 'System32\regsvr32.exe'
$regWow = Join-Path $env:WinDir 'SysWOW64\regsvr32.exe'

& $regSys '/s' (Join-Path $dest 'BanglaPhonetic_x64.dll')
if ($LASTEXITCODE -ne 0) { throw "Registering the 64-bit DLL failed (regsvr32 exit $LASTEXITCODE)." }

if ($haveX86 -and (Test-Path $regWow)) {
    & $regWow '/s' (Join-Path $dest 'BanglaPhonetic_x86.dll')
    if ($LASTEXITCODE -ne 0) { Write-Warning "Registering the 32-bit DLL failed (exit $LASTEXITCODE); 32-bit apps may not see Bangla." }
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
Write-Host 'Toggle Bangla/English with Ctrl+Shift+B (the language bar button shows the current mode).'
Write-Host 'If it does not appear, add the Bengali language under Settings > Time and language > Language.'
Write-Host ''
Read-Host 'Press Enter to close'
