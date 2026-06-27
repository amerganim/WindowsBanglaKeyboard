<#
  Patch the winget-releaser auto-PR to list all architectures.

  Our release is ONE universal Inno Setup .exe that serves x64/x86/arm64 (it
  installs the architecture-matched DLL at runtime). winget-releaser (komac)
  downloads the exe, detects a SINGLE architecture, and emits a single-arch
  installer manifest -- which the winget validation bot rejects as inconsistent
  with the previously published (3-arch) version.

  This finds the just-created PR on microsoft/winget-pkgs (head branch lives on
  the maintainer's winget-pkgs fork), and if any of x64/arm64/x86 are missing
  from Installers:, rebuilds the block to include all three (same URL + same
  SHA256, since one exe covers every arch). Then it re-runs validation.

  Requires gh authenticated as the fork owner (GH_TOKEN = the WINGET_TOKEN PAT).
  Idempotent: does nothing if all three architectures are already present.
#>
[CmdletBinding()]
param(
  [Parameter(Mandatory)] [string]$Version,
  [string]$Identifier   = 'Amerganim.AmaderBanglaKeyboard',
  [string]$UpstreamRepo = 'microsoft/winget-pkgs'
)
$ErrorActionPreference = 'Stop'

# --- locate the auto-created PR (winget-releaser may lag a few seconds) ---
$pr = $null
for ($i = 0; $i -lt 12; $i++) {
  $prs = gh pr list --repo $UpstreamRepo --author '@me' --state open `
           --json number,title,headRefName,headRepositoryOwner --limit 30 | ConvertFrom-Json
  $pr = $prs | Where-Object { $_.headRefName -like "$Identifier-$Version-*" } | Select-Object -First 1
  if ($pr) { break }
  Start-Sleep -Seconds 10
}
if (-not $pr) { Write-Host "No winget PR found for $Identifier $Version; nothing to patch."; exit 0 }

$forkOwner = $pr.headRepositoryOwner.login
$branch    = $pr.headRefName
Write-Host "Found PR #$($pr.number) on branch $forkOwner/winget-pkgs : $branch"

# --- manifest path: manifests/<first-letter>/<Publisher>/<Package>/<Version>/ ---
$first = $Identifier.Substring(0, 1).ToLower()
$dir   = "manifests/$first/" + (($Identifier -split '\.') -join '/') + "/$Version"
$path  = "$dir/$Identifier.installer.yaml"

$resp    = gh api "repos/$forkOwner/winget-pkgs/contents/$path`?ref=$branch" | ConvertFrom-Json
$content = [Text.Encoding]::UTF8.GetString([Convert]::FromBase64String($resp.content))
$sha     = $resp.sha
$lines   = $content -split "`r?`n"

# --- which architectures are present? ---
$present = @($lines |
  Where-Object { $_ -match 'Architecture:\s*(\S+)' } |
  ForEach-Object { ($_ -replace '.*Architecture:\s*', '').Trim() })
$desired = @('x64', 'arm64', 'x86')
$missing = @($desired | Where-Object { $present -notcontains $_ })
if ($missing.Count -eq 0) { Write-Host "All architectures present ($($present -join ', ')); nothing to do."; exit 0 }
Write-Host "Present: $($present -join ', '). Missing: $($missing -join ', '). Rebuilding Installers block."

# --- shared installer URL + hash (identical across arches: one universal exe) ---
$url  = (($lines | Where-Object { $_ -match 'InstallerUrl:' }    | Select-Object -First 1) -replace '.*InstallerUrl:\s*', '').Trim()
$hash = (($lines | Where-Object { $_ -match 'InstallerSha256:' } | Select-Object -First 1) -replace '.*InstallerSha256:\s*', '').Trim()
if (-not $url -or -not $hash) { throw "Could not read InstallerUrl/InstallerSha256 from the manifest." }

# --- split around the Installers: list and rebuild it with all three arches ---
$head = [System.Collections.Generic.List[string]]::new()
$tail = [System.Collections.Generic.List[string]]::new()
$state = 'head'
foreach ($ln in $lines) {
  switch ($state) {
    'head'       { $head.Add($ln); if ($ln -match '^Installers:') { $state = 'list' }; break }
    'list'       { if ($ln -match '^[A-Za-z]') { $state = 'tail'; $tail.Add($ln) }; break }  # drop old list entries
    default      { $tail.Add($ln) }
  }
}
$block = foreach ($a in $desired) { "- Architecture: $a"; "  InstallerUrl: $url"; "  InstallerSha256: $hash" }
$new = (@($head) + @($block) + @($tail)) -join "`n"

# --- commit the corrected file to the PR branch, then re-run validation ---
$payload = @{ message = "Add arm64/x86 installers (one universal exe) for $Version"; content = [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes($new)); sha = $sha; branch = $branch } | ConvertTo-Json -Compress
$tmp = [IO.Path]::GetTempFileName()
[IO.File]::WriteAllText($tmp, $payload, (New-Object Text.UTF8Encoding($false)))
gh api -X PUT "repos/$forkOwner/winget-pkgs/contents/$path" --input $tmp | Out-Null
Remove-Item $tmp -ErrorAction SilentlyContinue
gh pr comment $pr.number --repo $UpstreamRepo --body "@wingetbot run`n`nAdded the arm64/x86 installer entries: this package is a single universal Inno installer serving all three architectures (same URL + SHA256), matching the previously published manifest."
Write-Host "Patched PR #$($pr.number) with all architectures and re-ran validation."
