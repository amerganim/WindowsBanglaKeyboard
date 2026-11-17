<p align="center"><img src="docs/assets/icon-256.png" width="128" alt="Amader Bangla Keyboard"></p>

# Amader Bangla Keyboard

A free Windows keyboard to type **Bangla phonetically** (e.g. `amar` → আমার)
in any app. After installing, switch your input to "Amader Bangla Keyboard"
(**Win+Space**) and type romanized Bangla.

🌐 **Website & download:** https://amerganim.github.io/WindowsBanglaKeyboard/ ·
📖 **Typing guide:** https://amerganim.github.io/WindowsBanglaKeyboard/guide.html

Also available for **Mac** ([MacOSBanglaKeyboard](https://github.com/amerganim/MacOSBanglaKeyboard))
and **Android** ([AndroidBanglaKeyboard](https://github.com/amerganim/AndroidBanglaKeyboard)).

## How to get it (free)

**Option 1 — Installer (easiest).** Download `AmaderBanglaKeyboard-Setup-x.y.z.exe`
from the [latest release](https://github.com/amerganim/WindowsBanglaKeyboard/releases/latest)
(or the [website](https://amerganim.github.io/WindowsBanglaKeyboard/)), double-click
it and click **Next → Install**.

> If Windows shows **"Windows protected your PC"**, click **More info → Run anyway**.
> The keyboard is free, so it isn't signed with a paid certificate; the full
> source code is in this repository.

**Option 2 — winget** (Command Prompt or PowerShell):

```bat
winget install Amerganim.AmaderBanglaKeyboard
```

**Option 3 — zip.** Download `AmaderBanglaKeyboard-x.y.z.zip` from the release,
unzip it, right-click `install.ps1` → **Run with PowerShell**.

**Then:** press **Win+Space** and choose **Amader Bangla Keyboard**. Type
`amar sOnar bangla` and press **Space** → আমার সোনার বাংলা.
**Ctrl+Shift+B** switches between Bangla and English. If the keyboard isn't in
the list, add the **Bangla** language in Settings → Time & language → Language
& region, or sign out and back in once.

**Uninstall:** Settings → Apps → Installed apps → Amader Bangla Keyboard →
Uninstall (or `winget uninstall Amerganim.AmaderBanglaKeyboard`).

## Status

- ✅ **Phase 1 — phonetic engine** (`engine/`): longest-match tokenizer +
  stateful assembler (anusvara, ঋ, chandrabindu, visarga, digits); 16 tests pass.
- ✅ **Phase 0 — TSF TIP skeleton** (`app/`): registrable COM input method that
  links the engine into the keystroke path.
- ✅ **Phase 3 — composition & commit** (`app/`): live underlined preview,
  commit on space/enter, backspace editing. (Builds clean; live registration +
  in-app testing is the next step.)
- ✅ **Phase 2 — standalone harness** (`harness/`): console app to convert
  romanized Bangla without registering anything.
- ✅ **Phase 4 — UX (started)** (`app/`): language-bar button (বাং/Eng),
  Ctrl+Shift+B Bangla/English toggle (session-only; starts in Bangla).
- ✅ **Phase 5 — installer** (`installer/`, `scripts\package.bat`): builds
  x64+x86 and stages `dist\` with self-elevating install/uninstall scripts.
- ✅ **Typing guide**: installed as a Start Menu shortcut ("Amader Bangla
  Keyboard Typing Guide") and in the language-bar menu — a full key-map cheat sheet.
- ✅ **Word suggestions**: a curated roman→Bangla dictionary (autocorrect-style)
  **plus ~65k-word completion** from the Google bn lexicon, frequency-ranked,
  with a candidate window that **learns** which words you pick
  (`%LOCALAPPDATA%\BanglaPhonetic\learned.tsv`). ↓/↑/Tab pick, Enter/Space
  commit, Esc dismiss.
- ✅ **Coverage tested** on a Prothom Alo article: 100% of characters typeable;
  84% of distinct words have a suggestion.
- ✅ **x64 + x86 + ARM64** binaries; architecture-aware installer.
- ⏳ Next: code signing.

The keyboard icon is shared with the Android and Mac versions
(`docs/assets/icon-source.png`; regenerate `app/res/AppIcon.ico` with
`python scripts/make-icon.py`). The website lives in `site/` and is deployed
to GitHub Pages on every push to `main`.

The bundled `words.tsv` is derived from
[google/language-resources](https://github.com/google/language-resources) (bn),
licensed **CC BY 4.0**.

## Install from source

```bat
scripts\package.bat
```
Then right-click `dist\install.ps1` → **Run with PowerShell** (it self-elevates).
See [installer/README.md](installer/README.md) for details and uninstall.

See [docs/KEYMAP.md](docs/KEYMAP.md) for the full typing guide (every letter,
sign, digit, and conjunct), and [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for
the design and roadmap.

## Build

Requires VS Build Tools (MSVC), Windows SDK, CMake + Ninja.

```bat
scripts\build.bat x64                      :: engine + tests + harness
build-x64\tests\engine_tests.exe           :: run the tests
build-x64\harness\bnphonetic_harness.exe "amar sOnar bangla"   :: try the engine
scripts\build.bat x64 "-DBUILD_TIP=ON"     :: also build the TIP DLL
```
