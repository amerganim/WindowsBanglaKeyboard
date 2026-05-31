# Windows Bangla Phonetic Keyboard — Architecture

A Windows input method that lets you switch your input to **Bangla** and type
**phonetically** (e.g. `amar` → আমার). It is built as a **TSF Text Input
Processor (TIP)** — a COM in-proc DLL — so it registers as a real Windows input
method that appears in the Win+Space switcher and works across applications.

## Why TSF (and not a keyboard layout or a hook)

| Approach | Phonetic-capable? | Notes |
|---|---|---|
| Custom keyboard layout (`.klc`) | ❌ | Static 1-key→1-char remap; cannot buffer `amar`→আমার. |
| Low-level keyboard hook | ✅ | The classic Avro approach; easier but hacky (elevation, some apps/games). |
| **TSF TIP** (chosen) | ✅ | The native, supported way; works in all apps incl. elevated. |

Phonetic typing requires *buffering* a sequence of Latin keystrokes and
transliterating them with context (inherent vowel, dependent vowel signs,
conjuncts). That is a stateful engine, not a static layout.

## Components

```
engine/   Portable phonetic transliteration library (no Windows/COM deps).
tests/    Self-contained unit tests for the engine.
app/      The TSF TIP: COM DLL that hosts the engine as an input method.
scripts/  build.bat — configures + builds with the VS Build Tools toolchain.
docs/     This document.
installer/  (future) packaging + registration installer.
```

### `engine` — phonetic transliteration (Phase 1, done)

`bnphonetic::Transliterate(const std::string& latin) -> std::string` (UTF-8 in,
UTF-8 Bangla out). Two pieces:

1. **Greedy longest-match tokenizer** over a rule table (keys up to 3 chars,
   e.g. `chh`, `Dh`, `OI`), so multi-letter phonemes win over single letters.
2. **Stateful assembler** that tracks whether the previous unit was a consonant:
   - vowel after a consonant → dependent sign (kar); otherwise → independent vowel;
   - the inherent vowel `o` emits no sign after a consonant;
   - consonant after a consonant → inserts hasanta `্` to form a conjunct.

The engine has **no Windows dependencies**, so it is fully unit-tested on its
own (see `tests/`). It is compiled with `/utf-8` because the rule table and
tests contain Bangla string literals.

#### Rule-set status & known limitations (intentional, for the skeleton)

The current table is a curated, self-consistent Avro-style subset that passes
the test suite. Reserved for the rule-set expansion:

- **Anusvara `ং`** (e.g. বাংলা) as distinct from the velar nasal `ঙ`.
- Disambiguating the `ng` digraph from `n` + `g`.
- `O`-class diphthongs beyond `OI`/`OU`, chandrabindu, khanda-ta, ৎ, visarga.
- Auto-correct/suggestion dictionary.

### `app` — the TSF TIP (Phase 0 skeleton, done)

A registrable in-proc COM server:

- `CTextService` implements `ITfTextInputProcessorEx` (activation) and
  `ITfKeyEventSink` (keystroke notifications).
- `Globals.cpp` defines the server CLSID `{7E8D2A14-…}` and the language profile
  GUID `{C2A9B6D3-…}`; language id is bn-BD (`LANG_BENGALI` /
  `SUBLANG_BENGALI_BANGLADESH`).
- `Register.cpp` registers the COM server (HKCR\CLSID) and the TSF
  profile/category (`ITfInputProcessorProfiles` + `ITfCategoryMgr`).
- Exports `DllGetClassObject`, `DllCanUnloadNow`, `DllRegisterServer`,
  `DllUnregisterServer` (via `BanglaPhonetic.def`).

**Composition & commit (Phase 3, done):** the TIP now eats composing keys and
drives a live TSF composition:

- `EditSession.h` — a generic `ITfEditSession` (callback-based) used for all
  document edits, since TSF only grants read/write access inside an edit session.
- `UpdateComposition()` lazily starts a composition at the selection
  (`ITfInsertAtSelection` + `ITfContextComposition::StartComposition`), then
  replaces its range text with the latest transliteration and moves the caret
  to the end.
- The composition range is tagged with `GUID_PROP_ATTRIBUTE` → our display
  attribute (a dotted underline) via `CDisplayAttributeInfo` /
  `ITfDisplayAttributeProvider` (`DisplayAttribute.*`), registered under the
  `GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER` category.
- Space/Enter commit the word (`EndComposition`, text kept) and fall through;
  Backspace edits the buffer; any other key commits first. `OnCompositionTerminated`
  handles the app tearing down the composition.

## Build

Prereqs: VS Build Tools 18 (MSVC 14.51), Windows SDK, CMake + Ninja (bundled).

```bat
REM Engine + tests (x64):
scripts\build.bat x64

REM Run the tests:
build-x64\tests\engine_tests.exe

REM Also build the TIP DLL (NOTE: quote the define so cmd does not split at '='):
scripts\build.bat x64 "-DBUILD_TIP=ON"
```

Ship **both x64 and x86** DLLs: 32-bit host apps load only the 32-bit TIP. Build
x86 the same way: `scripts\build.bat x86 "-DBUILD_TIP=ON"`.

## Roadmap

- **Phase 0 — scaffolding & COM spike** ✅ — repo, build, registrable TIP that
  links the engine into the key path.
- **Phase 1 — phonetic engine** ✅ (curated rule set; tests green) — expand rules
  toward full Avro parity.
- **Phase 2 — standalone harness** — GUI/console to validate engine feel.
- **Phase 3 — composition & commit** ✅ — edit sessions, `ITfComposition`,
  underlined preview, commit on space/enter, backspace editing. Remaining:
  live-register + test in apps; English passthrough toggle.
- **Phase 4 — UX** — candidate/suggestion window, settings, tray + language-bar
  icon.
- **Phase 5 — installer** — WiX/MSIX, per-user registration, code signing.
- **Phase 6 — compatibility QA & release** — Office, browsers, terminals,
  Electron, elevated apps; beta with native typists.
