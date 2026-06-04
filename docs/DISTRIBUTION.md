# Distributing Amader Bangla Keyboard

## 0. Automated releases (GitHub Actions)

`.github/workflows/release.yml` does everything below automatically when you
push a version tag:

```bat
REM bump installer\install.ps1 $Version + docs\RELEASE_NOTES.md, commit, then:
git tag v0.10.2
git push origin v0.10.2
```

The workflow (on a `windows-latest` runner) builds x64/x86/ARM64, signs them
**if** signing secrets are set, builds the zip + `Setup.exe`, creates the
GitHub Release with both assets, and commits the refreshed `winget\` manifest
back to `main`. Manual runs are possible via *Actions → Release → Run workflow*.

**Optional signing secrets** (repo *Settings → Secrets and variables →
Actions*): `CODESIGN_PFX_BASE64` (your .pfx as base64) and
`CODESIGN_PFX_PASSWORD`. Without them, releases are simply unsigned. To submit
the manifest to the official winget repo automatically, you can later add the
`winget-releaser` action + a PAT; the first submission to `microsoft/winget-pkgs`
is done by hand.

## 1. Build the release package (manual)

```bat
powershell -ExecutionPolicy Bypass -File scripts\release.ps1
```

This builds x64 + x86 + **ARM64** (if those build tools are installed), stages
`dist\`, and produces, in `release\`:

- `AmaderBanglaKeyboard-<ver>.zip` — extract + run `install.ps1` (the script
  installer).
- `AmaderBanglaKeyboard-Setup-<ver>.exe` — a **single-file installer** built
  with Inno Setup (`installer\setup.iss`). This is the artifact for winget and
  the Microsoft Store. It registers the architecture-matching DLL(s), adds a
  Start-Menu guide shortcut, and supports silent install:

  ```bat
  AmaderBanglaKeyboard-Setup-<ver>.exe /VERYSILENT /SUPPRESSMSGBOXES /NORESTART
  ```

  (Uninstall silently via its `unins000.exe /VERYSILENT`, or Apps & features.)
  Building the .exe needs Inno Setup: `winget install JRSoftware.InnoSetup`.

## 2. (Important) Code-sign the binaries

The DLLs and scripts are currently **unsigned**, so Windows SmartScreen shows an
“unknown publisher” warning and some PCs block the PowerShell scripts. For real
distribution, sign them with an Authenticode certificate.

- **Get a certificate:** an **OV** code-signing cert (≈ $100–300/yr from a CA)
  works; an **EV** cert gives immediate SmartScreen reputation (no warning) but
  costs more and needs a hardware token.
- **Sign the DLLs** (use the SDK `signtool.exe`):

  ```bat
  signtool sign /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 ^
    /a dist\BanglaPhonetic_x64.dll dist\BanglaPhonetic_x86.dll dist\BanglaPhonetic_arm64.dll
  ```

- **Sign the PowerShell scripts** so they pass execution policy cleanly:

  ```powershell
  $cert = Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert | Select-Object -First 1
  Set-AuthenticodeSignature dist\install.ps1   $cert -TimestampServer http://timestamp.digicert.com
  Set-AuthenticodeSignature dist\uninstall.ps1 $cert -TimestampServer http://timestamp.digicert.com
  ```

- **Sign the installer too:** sign the DLLs in `dist\` first, then build
  `Setup.exe` (so the packaged DLLs are signed), then sign the resulting
  `Setup.exe` with the same `signtool` command.

Sign **before** zipping / before building the Setup.exe (and sign the Setup.exe
after).

## 3. Publish

**GitHub Releases (recommended to start):**

```bat
REM tag the version and push it
git tag v0.9.1
git push origin v0.9.1
```

Then create the release and attach the zip. With the GitHub CLI:

```bat
gh release create v0.9.1 release\AmaderBanglaKeyboard-0.9.1.zip ^
  --title "Amader Bangla Keyboard 0.9.1" --notes-file docs\RELEASE_NOTES.md
```

Or do it on github.com → *Releases* → *Draft a new release* → pick the tag →
upload the zip.

**winget:** a ready manifest is in `winget\` (points at the Setup.exe release
asset). After each release, update the version + `InstallerSha256` in
`winget\Amerganim.AmaderBanglaKeyboard.installer.yaml`, validate, and submit:

```bat
winget validate --manifest winget
winget install --manifest winget         REM optional local test (admin)
REM submit a PR to microsoft/winget-pkgs (easiest with wingetcreate):
wingetcreate submit --token <gh-token> winget
```

**Microsoft Store (Win32 app — the right path for this keyboard):** a TSF input
method needs machine-wide registration and to load into every process, which
the **MSIX sandbox cannot do** — so do *not* package it as MSIX. Instead submit
the **`Setup.exe`** as a Win32 (EXE) app:

1. Create a (free for individuals) **Partner Center** developer account and a
   new app reservation for the name "Amader Bangla Keyboard".
2. Choose packaging type **EXE/MSI (Win32)**, upload `Setup.exe`.
3. Provide the silent **install** command
   `/VERYSILENT /SUPPRESSMSGBOXES /NORESTART` and the **uninstall** command
   (`"%ProgramFiles%\BanglaPhonetic\unins000.exe" /VERYSILENT`), plus the return
   codes (0 = success, 3010 = success/restart-needed).
4. The Store **signs the package** for you; you still get smoother trust by
   signing `Setup.exe` yourself (section 2).
5. Note for review: this app is an **input method (IME)** that processes
   keystrokes — expect extra review and a clear privacy statement (it sends no
   data anywhere; suggestions/learning are fully local).

## 4. Building the ARM64 DLL

ARM64 support is fully wired; it just needs the cross-compiler:

1. In the **Visual Studio Installer**, modify the Build Tools and add
   **“MSVC … ARM64/ARM64EC build tools”** (and the Windows SDK).
2. Re-run `scripts\release.ps1`. `package.bat` will now also produce
   `BanglaPhonetic_arm64.dll`, and `install.ps1` registers it automatically on
   ARM64 Windows (native ARM64 apps), alongside the 32-bit DLL for x86 apps.

Until then the package contains x64 + x86, which run on ARM64 Windows under
emulation for x64/x86 apps but not in native ARM64 apps.

## What the installer does per architecture

| Windows | Registered (native via System32) | Plus 32-bit (SysWOW64) |
|---------|----------------------------------|------------------------|
| x64     | `BanglaPhonetic_x64`             | `BanglaPhonetic_x86`   |
| ARM64   | `BanglaPhonetic_arm64`           | `BanglaPhonetic_x86`   |
| x86     | `BanglaPhonetic_x86`             | —                      |
