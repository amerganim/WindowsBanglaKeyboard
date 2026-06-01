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

try {
    Write-Host "Uninstalling Bangla Phonetic Keyboard from $dest"

    # --- unregister (regsvr32 /u calls DllUnregisterServer) ---
    Unregister-Dll (Join-Path $env:WinDir 'System32\regsvr32.exe')  (Join-Path $dest 'BanglaPhonetic_x64.dll')
    $regWow = Join-Path $env:WinDir 'SysWOW64\regsvr32.exe'
    if (Test-Path $regWow) { Unregister-Dll $regWow (Join-Path $dest 'BanglaPhonetic_x86.dll') }

    # --- remove Add/Remove Programs entry and config ---
    $arp = "HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\$AppName"
    if (Test-Path $arp) { Remove-Item $arp -Recurse -Force }
    $cfg = 'HKCU:\Software\BanglaPhonetic'
    if (Test-Path $cfg) { Remove-Item $cfg -Recurse -Force }

    # --- delete the DLLs (best effort) ---
    # The DLLs may still be loaded by the text framework; deletion then fails
    # and Windows removes them after the next sign-out/restart. We do NOT blindly
    # schedule a folder delete (that could race a concurrent reinstall).
    $locked = $false
    foreach ($f in @('BanglaPhonetic_x64.dll', 'BanglaPhonetic_x86.dll')) {
        $p = Join-Path $dest $f
        if (Test-Path $p) {
            try { Remove-Item $p -Force } catch { $locked = $true }
        }
    }
    if (-not $locked) {
        # Files released - remove the (now DLL-free) folder contents and folder.
        Remove-Item $dest -Recurse -Force -ErrorAction SilentlyContinue
    }

    Write-Host ''
    Write-Host 'Bangla Phonetic Keyboard uninstalled (registration removed).' -ForegroundColor Green
    if ($locked) {
        Write-Host 'Some files are still in use; they will be removed after you sign out or restart.' -ForegroundColor Yellow
        Write-Host 'IMPORTANT: restart (or sign out/in) before reinstalling, so the loaded DLL is released.' -ForegroundColor Yellow
    }
} catch {
    Write-Host ''
    Write-Host "UNINSTALL ERROR: $($_.Exception.Message)" -ForegroundColor Red
} finally {
    Write-Host ''
    Read-Host 'Press Enter to close'
}
