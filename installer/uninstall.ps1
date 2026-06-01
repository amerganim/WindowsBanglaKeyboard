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

function Unregister-Dll([string]$regsvr, [string]$dll) {
    if (-not (Test-Path $dll)) { return }
    Start-Process -FilePath $regsvr -ArgumentList @('/u', '/s', "`"$dll`"") `
                  -Wait -WindowStyle Hidden | Out-Null
}

$AppName = 'BanglaPhonetic'
$dest    = $PSScriptRoot   # the script lives in the install directory
$regSys  = Join-Path $env:WinDir 'System32\regsvr32.exe'
$regWow  = Join-Path $env:WinDir 'SysWOW64\regsvr32.exe'

try {
    Write-Host "Uninstalling Bangla Phonetic Keyboard from $dest"

    # --- unregister every installed DLL (any build) ---
    Get-ChildItem $dest -Filter 'BanglaPhonetic_x64*.dll' -ErrorAction SilentlyContinue |
        ForEach-Object { Unregister-Dll $regSys $_.FullName }
    if (Test-Path $regWow) {
        Get-ChildItem $dest -Filter 'BanglaPhonetic_x86*.dll' -ErrorAction SilentlyContinue |
            ForEach-Object { Unregister-Dll $regWow $_.FullName }
    }

    # --- remove Add/Remove Programs entry and config ---
    $arp = "HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\$AppName"
    if (Test-Path $arp) { Remove-Item $arp -Recurse -Force }
    $cfg = 'HKCU:\Software\BanglaPhonetic'
    if (Test-Path $cfg) { Remove-Item $cfg -Recurse -Force }

    # --- delete the DLLs (best effort) ---
    # A loaded DLL can't be deleted yet; Windows removes it after the next
    # sign-out/restart. Registration is already gone, so it is inert meanwhile.
    $locked = $false
    Get-ChildItem $dest -Filter 'BanglaPhonetic_*.dll' -ErrorAction SilentlyContinue |
        ForEach-Object { try { Remove-Item $_.FullName -Force } catch { $locked = $true } }
    if (-not $locked) {
        Remove-Item $dest -Recurse -Force -ErrorAction SilentlyContinue
    }

    Write-Host ''
    Write-Host 'Bangla Phonetic Keyboard uninstalled (registration removed).' -ForegroundColor Green
    if ($locked) {
        Write-Host 'Some DLL files are still loaded and will be removed after you sign out or restart.' -ForegroundColor Yellow
    }
} catch {
    Write-Host ''
    Write-Host "UNINSTALL ERROR: $($_.Exception.Message)" -ForegroundColor Red
} finally {
    Write-Host ''
    Read-Host 'Press Enter to close'
}
