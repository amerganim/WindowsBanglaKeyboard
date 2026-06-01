# WindowsBanglaKeyboard

A Windows input method to type **Bangla phonetically** (e.g. `amar` → আমার).
After installation you switch your input to "Bangla Phonetic" (Win+Space) and
type romanized Bangla in any application.

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
- ✅ **Typing guide**: installed as a Start Menu shortcut ("Bangla Phonetic
  Typing Guide") and in the language-bar menu — a full key-map cheat sheet.
- ✅ **Word suggestions**: an editable bundled dictionary (`dictionary.tsv`,
  ~275 words) + frequency-ranked candidate window that **learns** which words
  you pick (stored per-user in `%LOCALAPPDATA%\BanglaPhonetic\learned.tsv`).
  ↓/↑/Tab to pick, Enter/Space to commit, Esc to dismiss.
- ⏳ Next: testing against real Bangla text; code signing; ARM64 build.

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
