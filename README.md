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
- ⏳ Next: register & test in real apps; English passthrough toggle; installer.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the design and full roadmap.

## Build

Requires VS Build Tools (MSVC), Windows SDK, CMake + Ninja.

```bat
scripts\build.bat x64                      :: engine + tests + harness
build-x64\tests\engine_tests.exe           :: run the tests
build-x64\harness\bnphonetic_harness.exe "amar sOnar bangla"   :: try the engine
scripts\build.bat x64 "-DBUILD_TIP=ON"     :: also build the TIP DLL
```
