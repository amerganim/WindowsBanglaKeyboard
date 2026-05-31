#include "Globals.h"

// Implemented in ClassFactory.cpp.
HRESULT CreateClassFactory(REFCLSID rclsid, REFIID riid, void** ppv);

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID /*reserved*/) {
  switch (reason) {
    case DLL_PROCESS_ATTACH:
      g_hInst = hInst;
      DisableThreadLibraryCalls(hInst);
      break;
    default:
      break;
  }
  return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv) {
  return CreateClassFactory(rclsid, riid, ppv);
}

STDAPI DllCanUnloadNow() { return (g_cRefDll == 0) ? S_OK : S_FALSE; }
