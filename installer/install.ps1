<#
  Amader Bangla Keyboard - installer.

  Copies the input-method DLLs to Program Files and registers the ones that
  match this PC's architecture so all apps can use them (native + 32-bit on
  x64; native ARM64 + 32-bit on ARM64 Windows), and adds an "Apps & features"
  entry. Requires administrator rights and self-elevates.

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

# Copy a component DLL under a content-hashed name (so a loaded old build is
# never overwritten) and register it with the matching-architecture regsvr32.
# Returns the installed target path, or $null if the source isn't present.
function Install-Component([string]$srcName, [string]$regsvr, [string]$label) {
    $srcPath = Join-Path $script:src $srcName
    if (-not (Test-Path $srcPath)) { return $null }
    if (-not (Test-Path $regsvr)) {
        Write-Warning "regsvr32 for $label not found; skipping $srcName."
        return $null
    }
    $hash = (Get-FileHash -Algorithm SHA256 -Path $srcPath).Hash.Substring(0, 8).ToLower()
    $base = [IO.Path]::GetFileNameWithoutExtension($srcName)  # e.g. BanglaPhonetic_x64
    $target = Join-Path $script:dest "${base}_${hash}.dll"
    if (-not (Test-Path $target)) { Copy-Item $srcPath $target }
    $p = Start-Process -FilePath $regsvr -ArgumentList @('/s', "`"$target`"") `
                       -Wait -PassThru -WindowStyle Hidden
    if ($p.ExitCode -ne 0) {
        throw "Registering the $label DLL failed (regsvr32 exit $($p.ExitCode))."
    }
    return $target
}

$AppName   = 'BanglaPhonetic'
$Display   = 'Amader Bangla Keyboard'
$Version   = '0.11.0'
$Publisher = 'WindowsBanglaKeyboard'

$src  = $PSScriptRoot
$dest = Join-Path $env:ProgramFiles $AppName
$log  = Join-Path $env:TEMP 'BanglaPhonetic_install.log'

try { Start-Transcript -Path $log -Force | Out-Null } catch {}

try {
    Write-Host "Installing $Display $Version to $dest"
    New-Item -ItemType Directory -Force -Path $dest | Out-Null
    Copy-Item (Join-Path $src 'uninstall.ps1') (Join-Path $dest 'uninstall.ps1') -Force -ErrorAction SilentlyContinue
    $guide = Join-Path $dest 'KEYMAP.html'
    Copy-Item (Join-Path $src 'KEYMAP.html') $guide -Force -ErrorAction SilentlyContinue
    Copy-Item (Join-Path $src 'dictionary.tsv') (Join-Path $dest 'dictionary.tsv') -Force -ErrorAction SilentlyContinue
    Copy-Item (Join-Path $src 'words.tsv') (Join-Path $dest 'words.tsv') -Force -ErrorAction SilentlyContinue

    # Register the DLLs that match this PC's architecture. System32\regsvr32 is
    # the OS-native one (x64 on x64 Windows, ARM64 on ARM64 Windows); SysWOW64 is
    # the 32-bit one. (DllRegisterServer must run in a matching-arch process, so
    # we don't try to register, say, an x64 DLL on ARM64.)
    $regSys = Join-Path $env:WinDir 'System32\regsvr32.exe'
    $regWow = Join-Path $env:WinDir 'SysWOW64\regsvr32.exe'
    $osArch = [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString()
    Write-Host "Detected Windows architecture: $osArch"

    switch ($osArch) {
        'Arm64' { $nativeSrc = 'BanglaPhonetic_arm64.dll' }
        'X86'   { $nativeSrc = 'BanglaPhonetic_x86.dll' }
        default { $nativeSrc = 'BanglaPhonetic_x64.dll' }   # X64
    }

    $installed = @()
    $native = Install-Component $nativeSrc $regSys "native ($osArch)"
    if ($native) { $installed += $native }

    # Add the 32-bit DLL for x86 apps on 64-bit/ARM64 Windows.
    if ($osArch -eq 'X64' -or $osArch -eq 'Arm64') {
        $x86 = Install-Component 'BanglaPhonetic_x86.dll' $regWow '32-bit'
        if ($x86) { $installed += $x86 }
    }

    if ($installed.Count -eq 0) {
        throw "No matching DLL for $osArch was found in this package (run scripts\package.bat)."
    }
    if (-not $native -and $osArch -eq 'Arm64') {
        Write-Warning 'No ARM64 DLL in this package: native ARM64 apps will not see the keyboard (x86 apps will). Rebuild with the ARM64 build tools to add it.'
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

    # --- Start Menu shortcut to the typing guide (discoverable tutorial) ---
    $programs = [Environment]::GetFolderPath('CommonPrograms')
    # Remove the legacy folder name from earlier versions, if present.
    $legacy = Join-Path $programs 'Bangla Phonetic'
    if (Test-Path $legacy) { Remove-Item $legacy -Recurse -Force -ErrorAction SilentlyContinue }
    if (Test-Path $guide) {
        $folder = Join-Path $programs 'Amader Bangla Keyboard'
        New-Item -ItemType Directory -Force -Path $folder | Out-Null
        $ws = New-Object -ComObject WScript.Shell
        $lnk = $ws.CreateShortcut((Join-Path $folder 'Amader Bangla Keyboard Typing Guide.lnk'))
        $lnk.TargetPath = $guide
        $lnk.Description = 'How to type Bangla phonetically (key map / tutorial)'
        $lnk.Save()
    }

    # --- clean up older builds' DLLs (best effort; loaded ones are skipped and
    # are harmlessly removed after the next restart) ---
    Get-ChildItem $dest -Filter 'BanglaPhonetic_*.dll' -ErrorAction SilentlyContinue |
        Where-Object { $installed -notcontains $_.FullName } |
        Remove-Item -Force -ErrorAction SilentlyContinue

    Write-Host ''
    Write-Host "$Display $Version installed (no restart needed)." -ForegroundColor Green
    Write-Host 'Switch input methods with Win+Space (look for "Amader Bangla Keyboard").'
    Write-Host 'Toggle Bangla/English with Ctrl+Shift+B.'
    Write-Host 'Typing guide: Start Menu > Amader Bangla Keyboard > "Amader Bangla Keyboard Typing Guide".'
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
