# WindowsBanglaKeyboard

A Windows input method to type **Bangla phonetically** (e.g. `amar` → আমার).
After installation you switch your input to "Bangla Phonetic" (Win+Space) and
type romanized Bangla in any application.

Built as a **TSF Text Input Processor** (a native Windows input method) with a
portable, unit-tested phonetic engine at its core.

## Status

- ✅ **Phase 1 — phonetic engine** (`engine/`): longest-match tokenizer +
  stateful assembler; 13 unit tests passing.
- ✅ **Phase 0 — TSF TIP skeleton** (`app/`): registrable COM input method that
  links the engine into the keystroke path.
- ✅ **Phase 3 — composition & commit** (`app/`): live underlined preview,
  commit on space/enter, backspace editing. (Builds clean; live registration +
  in-app testing is the next step.)
- ⏳ Next: register & test in real apps; English passthrough toggle; Phase 2
  harness for rule iteration.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the design and full roadmap.

## Build

Requires VS Build Tools (MSVC), Windows SDK, CMake + Ninja.

```bat
scripts\build.bat x64                      :: engine + tests
build-x64\tests\engine_tests.exe           :: run the tests
scripts\build.bat x64 "-DBUILD_TIP=ON"     :: also build the TIP DLL
```
