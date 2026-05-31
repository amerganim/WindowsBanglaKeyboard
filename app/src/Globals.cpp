#include "Globals.h"

#include <initguid.h>

// {7E8D2A14-3B6C-4F1E-9A2D-1C5B6E0F3A21}
DEFINE_GUID(c_clsidBanglaPhonetic, 0x7e8d2a14, 0x3b6c, 0x4f1e, 0x9a, 0x2d,
            0x1c, 0x5b, 0x6e, 0x0f, 0x3a, 0x21);

// {C2A9B6D3-4E15-4A8F-B0C7-2D9E1F3A4B56}
DEFINE_GUID(c_guidProfile, 0xc2a9b6d3, 0x4e15, 0x4a8f, 0xb0, 0xc7, 0x2d, 0x9e,
            0x1f, 0x3a, 0x4b, 0x56);

const WCHAR kProfileDescription[] = L"Bangla Phonetic";

HINSTANCE g_hInst = nullptr;
LONG g_cRefDll = 0;

void DllAddRef() { InterlockedIncrement(&g_cRefDll); }
void DllRelease() { InterlockedDecrement(&g_cRefDll); }
