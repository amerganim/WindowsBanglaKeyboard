<#
  Build installer\words.tsv (the broad Bangla word-completion list) from a
  pronunciation/word lexicon whose first tab-separated column is the Bangla
  word (e.g. Google language-resources bn/data/lexicon.tsv).

  Output format:  bangla<TAB>freq   (freq is a short-word heuristic; the
  curated dictionary.tsv supplies real frequencies for common words).

  Usage:  powershell -ExecutionPolicy Bypass -File scripts\build-wordlist.ps1 [-Lexicon path] [-Out path]
#>
param(
  [string]$Lexicon = "$PSScriptRoot\..\lexicon.tsv",
  [string]$Out     = "$PSScriptRoot\..\installer\words.tsv"
)
$ErrorActionPreference = 'Stop'
if (-not (Test-Path $Lexicon)) { throw "Lexicon not found: $Lexicon" }

$seen = New-Object 'System.Collections.Generic.HashSet[string]'
$sb = [System.Text.StringBuilder]::new()
# Attribution (lines starting with # are ignored by the loader).
[void]$sb.AppendLine('# Bangla word list for Amader Bangla Keyboard.')
[void]$sb.AppendLine('# Derived from the Google language-resources (bn) lexicon,')
[void]$sb.AppendLine('# https://github.com/google/language-resources  (CC BY 4.0).')
[void]$sb.AppendLine('# Format: bangla<TAB>freq')
foreach ($line in [System.IO.File]::ReadLines($Lexicon)) {
  $t = $line.Trim()
  if ($t -eq '' -or $t.StartsWith('#')) { continue }
  $w = ($t -split "`t")[0].Trim()
  if ($w -eq '' -or $w.Contains(' ')) { continue }
  if (-not $seen.Add($w)) { continue }
  $cp = [System.Globalization.StringInfo]::new($w).LengthInTextElements
  $freq = [Math]::Max(3, 14 - $cp)   # shorter words rank a little higher
  [void]$sb.AppendLine("$w`t$freq")
}
[System.IO.File]::WriteAllText($Out, $sb.ToString(), (New-Object System.Text.UTF8Encoding($false)))
"wrote $($seen.Count) words to $Out"
