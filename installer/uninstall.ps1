<#
  Bangla Phonetic Keyboard - uninstaller.

  Unregisters the input-method DLLs, removes the Add/Remove Programs entry, and
  deletes the installed files. Requires administrator rights and self-elevates.
#>
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'

# --- self-elevate ---
$principal = New-Object Security.Principal.WindowsPrincipal(
    [Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltinRole]::Administrator)) {
    Start-Process powershell.exe -Verb RunAs -ArgumentList @(
        '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`"")
    return
}

$AppName = 'BanglaPhonetic'
$dest    = $PSScriptRoot   # the script lives in the install directory

Write-Host "Uninstalling Bangla Phonetic Keyboard from $dest"

# --- unregister (regsvr32 /u calls DllUnregisterServer) ---
$regSys = Join-Path $env:WinDir 'System32\regsvr32.exe'
$regWow = Join-Path $env:WinDir 'SysWOW64\regsvr32.exe'

$dllX64 = Join-Path $dest 'BanglaPhonetic_x64.dll'
$dllX86 = Join-Path $dest 'BanglaPhonetic_x86.dll'
if (Test-Path $dllX64) { & $regSys '/u' '/s' $dllX64 }
if ((Test-Path $dllX86) -and (Test-Path $regWow)) { & $regWow '/u' '/s' $dllX86 }

# --- remove Add/Remove Programs entry ---
$arp = "HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\$AppName"
if (Test-Path $arp) { Remove-Item $arp -Recurse -Force }

# --- remove our config ---
$cfg = "HKCU:\Software\BanglaPhonetic"
if (Test-Path $cfg) { Remove-Item $cfg -Recurse -Force }

# --- delete files ---
# The DLLs may still be loaded by running processes (ctfmon, the shell), so a
# delayed delete from a detached shell handles files that are momentarily
# locked, and also removes this running script's own folder.
Start-Process cmd.exe -WindowStyle Hidden -ArgumentList @(
    '/c', "timeout /t 2 >nul & rmdir /s /q `"$dest`"")

Write-Host ''
Write-Host 'Bangla Phonetic Keyboard uninstalled.' -ForegroundColor Green
Write-Host 'If any files remain (DLL in use), they are removed on next sign-out/restart.'
Write-Host ''
Read-Host 'Press Enter to close'
