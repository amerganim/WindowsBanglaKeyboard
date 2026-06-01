# Bangla Phonetic Keyboard — install / uninstall

## Build the package

From the repo root (needs the VS Build Tools toolchain):

```bat
scripts\package.bat
```

This builds the 64-bit and 32-bit input-method DLLs and stages an installable
package in `dist\`:

```
dist\
  BanglaPhonetic_x64.dll
  BanglaPhonetic_x86.dll
  install.ps1
  uninstall.ps1
  README.md
  LICENSE
```

## Install

Right-click `dist\install.ps1` → **Run with PowerShell** (it will prompt for
administrator rights), or from an elevated prompt:

```powershell
powershell -ExecutionPolicy Bypass -File dist\install.ps1
```

It copies the DLLs to `C:\Program Files\BanglaPhonetic\`, registers them for
both 64-bit and 32-bit apps, and adds an **Apps & features** entry.

After installing:

- Switch input methods with **Win+Space** and pick **"Bangla Phonetic"**.
- Toggle Bangla/English with **Ctrl+Shift+B**; the language bar shows **বাং / Eng**.
- If it does not appear, add the **Bengali** language under
  *Settings → Time & language → Language & region*.

## Uninstall

Use **Apps & features → Bangla Phonetic Keyboard → Uninstall**, or run
`uninstall.ps1` from the install folder. It unregisters the DLLs, removes the
config and the Apps entry, and deletes the files (any DLL still loaded is
removed on next sign-out/restart).

## Updating / reinstalling (no restart needed)

A Windows input-method DLL stays **loaded** by the text framework after use, so
it can't be overwritten while loaded. To avoid that, each build is installed
under a **content-hashed filename** (e.g. `BanglaPhonetic_x64_1a2b3c4d.dll`):

- Installing a new build writes a *new* file and re-points the registration at
  it — nothing has to be overwritten, so **no restart is required**.
- You don't even need to uninstall first; just run `install.ps1`.
- Old builds' DLLs are cleaned up automatically (any still loaded are removed
  after the next restart — harmless, since they're no longer registered).

To be sure a program uses the just-installed version, **close and reopen that
program** (a full Windows restart is not necessary). If an install ever fails it
keeps the window open with the error and logs to
`%TEMP%\BanglaPhonetic_install.log`.

## FAQ

**The keyboard reverts to English when I switch apps. Why?**
That's standard Windows behaviour, not this software: Windows remembers a
*separate* input method for **each application window**, so switching to another
app restores that app's own last-used language. To make your choice apply
everywhere instead, go to **Settings → Time & language → Typing → Advanced
keyboard settings** and **untick "Let me use a different input method for each
app window."** (Optionally set **"Override for default input method"** to
*Bangla Phonetic*.) Within Bangla you can still toggle English with
**Ctrl+Shift+B**.

**Do I need to install a Bangla font?**
No. Windows already includes a Bengali font (**Nirmala UI**, shipped since
Windows 8) and renders Bengali automatically. This keyboard only produces
Unicode text, so it bundles no fonts and needs none — it works out of the box on
a normal Windows 8/10/11 PC. (Only rare, stripped-down or "N" editions might
need the *Bengali Supplemental Fonts* optional Windows feature.)

## Notes

- Administrator rights are required (the input method is registered
  machine-wide); both scripts self-elevate.
- The DLLs are **unsigned**. For distribution to other machines you should
  code-sign them (e.g. with `signtool`) so SmartScreen and enterprise policies
  trust them; signing is not required for local testing.
