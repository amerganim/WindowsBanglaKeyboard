<#
  Refresh the winget manifest in winget\ to a new version + Setup.exe SHA-256.
  Called by release.ps1; can also be run by hand.

  Usage: powershell -File scripts\update-winget.ps1 -Version 0.10.2 -Sha256 <hash>
#>
param(
  [Parameter(Mandatory = $true)][string]$Version,
  [Parameter(Mandatory = $true)][string]$Sha256
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$dir = Join-Path $root 'winget'
$url = "https://github.com/amerganim/WindowsBanglaKeyboard/releases/download/v$Version/AmaderBanglaKeyboard-Setup-$Version.exe"
$utf8 = New-Object System.Text.UTF8Encoding($false)

foreach ($f in Get-ChildItem $dir -Filter *.yaml) {
  $c = [System.IO.File]::ReadAllText($f.FullName)
  $c = [regex]::Replace($c, '(?m)^(PackageVersion:\s*).*$', "`${1}$Version")
  if ($f.Name -like '*installer*') {
    $c = [regex]::Replace($c, '(?m)^(\s*InstallerUrl:\s*).*$', "`${1}$url")
    $c = [regex]::Replace($c, '(?m)^(\s*InstallerSha256:\s*).*$', "`${1}$($Sha256.ToUpper())")
  }
  [System.IO.File]::WriteAllText($f.FullName, $c, $utf8)
}
Write-Host "winget manifest updated to $Version"
