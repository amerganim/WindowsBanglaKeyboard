<#
  Build the distributable: runs package.bat (x64 + x86, and ARM64 if the ARM64
  build tools are installed), then zips dist\ into
  release\AmaderBanglaKeyboard-<version>.zip and prints its SHA-256.

  Usage:  powershell -ExecutionPolicy Bypass -File scripts\release.ps1 [-Version 0.9.1]
#>
param([string]$Version)
$ErrorActionPreference = 'Stop'

$root = Split-Path $PSScriptRoot -Parent

# Default the version to whatever install.ps1 declares, to avoid drift.
if (-not $Version) {
    $line = Get-Content (Join-Path $root 'installer\install.ps1') |
            Where-Object { $_ -match "\`$Version\s*=\s*'(.+?)'" } | Select-Object -First 1
    if ($line -match "'(.+?)'") { $Version = $Matches[1] } else { $Version = '0.0.0' }
}

Write-Host "Building release $Version ..."
& cmd /c "`"$PSScriptRoot\package.bat`""
if ($LASTEXITCODE -ne 0) { throw 'package.bat failed.' }

$dist = Join-Path $root 'dist'
$rel = Join-Path $root 'release'
New-Item -ItemType Directory -Force -Path $rel | Out-Null
$zip = Join-Path $rel "AmaderBanglaKeyboard-$Version.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }

Compress-Archive -Path (Join-Path $dist '*') -DestinationPath $zip
Write-Host ''
Write-Host "Created $zip" -ForegroundColor Green
"Contents:"; (Get-ChildItem $dist | Select-Object -ExpandProperty Name) -join ', '
Get-FileHash $zip -Algorithm SHA256 | Format-List Algorithm, Hash

# Also build the single-file Setup.exe (for direct download, winget, Store)
# if Inno Setup is installed.
$iscc = @(
    "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe",
    "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
    "$env:ProgramFiles\Inno Setup 6\ISCC.exe"
) | Where-Object { Test-Path $_ } | Select-Object -First 1
if ($iscc) {
    Write-Host ''
    Write-Host 'Building Setup.exe (Inno Setup)...'
    & $iscc "/DMyAppVersion=$Version" (Join-Path $root 'installer\setup.iss') | Select-Object -Last 2
    $setup = Join-Path $rel "AmaderBanglaKeyboard-Setup-$Version.exe"
    if (Test-Path $setup) {
        Write-Host "Created $setup" -ForegroundColor Green
        Get-FileHash $setup -Algorithm SHA256 | Format-List Algorithm, Hash
    }
} else {
    Write-Warning 'Inno Setup (ISCC.exe) not found; skipped Setup.exe. Install with: winget install JRSoftware.InnoSetup'
}
