# Amader Bangla Keyboard

A Windows input method to type **Bangla phonetically** (e.g. `amar` → আমার).
After installation you switch your input to "Amader Bangla Keyboard" (Win+Space)
and type romanized Bangla in any application.

Built as a **TSF Text Input Processor** (a native Windows input method) with a
portable, unit-tested phonetic engine at its core.

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
- ⏳ Next: code signing; ARM64 binary (build is wired).

The bundled `words.tsv` is derived from
[google/language-resources](https://github.com/google/language-resources) (bn),
licensed **CC BY 4.0**.

## Install

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
