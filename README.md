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
- ⏳ Next: **Phase 3** — compose & commit Bangla into the document.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the design and full roadmap.

## Build

Requires VS Build Tools (MSVC), Windows SDK, CMake + Ninja.

```bat
scripts\build.bat x64                      :: engine + tests
build-x64\tests\engine_tests.exe           :: run the tests
scripts\build.bat x64 "-DBUILD_TIP=ON"     :: also build the TIP DLL
```
