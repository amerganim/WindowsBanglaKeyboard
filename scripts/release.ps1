<#
  Build the distributable: package.bat (x64 + x86, and ARM64 if those tools are
  installed), then produce release\AmaderBanglaKeyboard-<ver>.zip and (if Inno
  Setup is present) AmaderBanglaKeyboard-Setup-<ver>.exe. If a code-signing
  certificate is available in the current user's store it signs the binaries;
  otherwise it just skips signing. Finally refreshes the winget manifest.

  Usage:  powershell -ExecutionPolicy Bypass -File scripts\release.ps1 [-Version 0.10.1]
#>
param([string]$Version)
$ErrorActionPreference = 'Stop'

$root = Split-Path $PSScriptRoot -Parent

if (-not $Version) {
    $line = Get-Content (Join-Path $root 'installer\install.ps1') |
            Where-Object { $_ -match "\`$Version\s*=\s*'(.+?)'" } | Select-Object -First 1
    if ($line -match "'(.+?)'") { $Version = $Matches[1] } else { $Version = '0.0.0' }
}

# Optional code signing (no-op unless a code-signing cert is in the store).
$cert = Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $cert) { $cert = Get-ChildItem Cert:\LocalMachine\My -CodeSigningCert -ErrorAction SilentlyContinue | Select-Object -First 1 }
function Invoke-Sign([string[]]$paths) {
    if (-not $cert) { return }
    $existing = $paths | Where-Object { Test-Path $_ }
    if (-not $existing) { return }
    Write-Host "Signing: $($existing -join ', ')"
    Set-AuthenticodeSignature -FilePath $existing -Certificate $cert `
        -HashAlgorithm SHA256 -TimestampServer 'http://timestamp.digicert.com' | Out-Null
}
if ($cert) { Write-Host "Code-signing with: $($cert.Subject)" } else { Write-Host 'No code-signing cert found; building unsigned.' }

Write-Host "Building release $Version ..."
& cmd /c "`"$PSScriptRoot\package.bat`""
if ($LASTEXITCODE -ne 0) { throw 'package.bat failed.' }

$dist = Join-Path $root 'dist'
$rel = Join-Path $root 'release'
New-Item -ItemType Directory -Force -Path $rel | Out-Null

# Sign the staged binaries/scripts before they are zipped/packaged.
Invoke-Sign (Get-ChildItem $dist -Filter *.dll | ForEach-Object { $_.FullName })
Invoke-Sign @((Join-Path $dist 'install.ps1'), (Join-Path $dist 'uninstall.ps1'))

$zip = Join-Path $rel "AmaderBanglaKeyboard-$Version.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $dist '*') -DestinationPath $zip
Write-Host ''
Write-Host "Created $zip" -ForegroundColor Green
Get-FileHash $zip -Algorithm SHA256 | Format-List Algorithm, Hash

# Single-file Setup.exe (direct download, winget, Store) if Inno Setup is present.
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
        Invoke-Sign @($setup)
        $hash = (Get-FileHash $setup -Algorithm SHA256).Hash
        Write-Host "Created $setup" -ForegroundColor Green
        Write-Host "SHA256: $hash"
        # Keep the winget manifest in sync with this build.
        & "$PSScriptRoot\update-winget.ps1" -Version $Version -Sha256 $hash
    }
} else {
    Write-Warning 'Inno Setup (ISCC.exe) not found; skipped Setup.exe. Install with: winget install JRSoftware.InnoSetup'
}
