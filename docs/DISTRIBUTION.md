# Distributing Amader Bangla Keyboard

## 1. Build the release package

```bat
powershell -ExecutionPolicy Bypass -File scripts\release.ps1
```

This builds x64 + x86 (and **ARM64** if those build tools are installed — see
below), stages `dist\`, and produces `release\AmaderBanglaKeyboard-<version>.zip`
plus its SHA-256. That zip is the thing you hand to users: they extract it and
run `install.ps1`.

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

Sign **before** zipping (or re-zip after signing).

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

**winget (optional):** submit a manifest to
`microsoft/winget-pkgs` pointing at the GitHub release asset so users can
`winget install`. Easiest once the installer is a signed `.exe`/`.msi`.

**Microsoft Store / MSIX (optional, widest reach + auto-update):** repackage as
**MSIX** and declare the input method in the manifest. More work and Store
policy review, but Store handles signing and updates.

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
