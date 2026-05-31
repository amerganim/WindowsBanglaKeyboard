#pragma once

#include <windows.h>
#include <msctf.h>

// ---------------------------------------------------------------------------
// Global identifiers and state for the Bangla Phonetic TSF Text Input Processor.
//
// PHASE 0 SKELETON: this builds a registrable input method that shows up in the
// Windows input switcher (Win+Space) and links the phonetic engine into the
// key-event path. It does NOT yet compose/commit text into the target document
// (that is Phase 3 — edit sessions + ITfComposition). On key-down it currently
// runs the engine and emits the result via OutputDebugString to prove the
// in-process wiring end to end.
// ---------------------------------------------------------------------------

// COM server class id for the text service.
// {7E8D2A14-3B6C-4F1E-9A2D-1C5B6E0F3A21}
extern const CLSID c_clsidBanglaPhonetic;

// Language profile id for the input method.
// {C2A9B6D3-4E15-4A8F-B0C7-2D9E1F3A4B56}
extern const GUID c_guidProfile;

// Bengali (Bangladesh), bn-BD.
constexpr LANGID kLangIdBangla = MAKELANGID(LANG_BENGALI, SUBLANG_BENGALI_BANGLADESH);

extern const WCHAR kProfileDescription[];

// Module-wide state.
extern HINSTANCE g_hInst;
extern LONG g_cRefDll;  // outstanding object references (DllCanUnloadNow)

void DllAddRef();
void DllRelease();
