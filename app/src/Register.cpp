#include <olectl.h>

#include <string>

#include "Globals.h"

// COM server (CLSID) + TSF profile/category registration.

namespace {

const WCHAR kClsidKeyFmt[] = L"CLSID\\%s";
const WCHAR kThreadingModel[] = L"Apartment";

bool ClsidToString(REFGUID guid, WCHAR* buf, int cch) {
  return StringFromGUID2(guid, buf, cch) > 0;
}

// Registers HKCR\CLSID\{clsid}\InprocServer32 -> this DLL.
HRESULT RegisterServer() {
  WCHAR clsid[64];
  if (!ClsidToString(c_clsidBanglaPhonetic, clsid, ARRAYSIZE(clsid)))
    return E_FAIL;

  WCHAR module[MAX_PATH];
  if (GetModuleFileNameW(g_hInst, module, ARRAYSIZE(module)) == 0)
    return HRESULT_FROM_WIN32(GetLastError());

  WCHAR subkey[128];
  wsprintfW(subkey, kClsidKeyFmt, clsid);

  HKEY hkey = nullptr;
  if (RegCreateKeyExW(HKEY_CLASSES_ROOT, subkey, 0, nullptr,
                      REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hkey,
                      nullptr) != ERROR_SUCCESS)
    return E_FAIL;
  RegSetValueExW(hkey, nullptr, 0, REG_SZ,
                 reinterpret_cast<const BYTE*>(kProfileDescription),
                 static_cast<DWORD>((lstrlenW(kProfileDescription) + 1) *
                                    sizeof(WCHAR)));

  HKEY hsub = nullptr;
  if (RegCreateKeyExW(hkey, L"InprocServer32", 0, nullptr,
                      REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hsub,
                      nullptr) == ERROR_SUCCESS) {
    RegSetValueExW(hsub, nullptr, 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(module),
                   static_cast<DWORD>((lstrlenW(module) + 1) * sizeof(WCHAR)));
    RegSetValueExW(hsub, L"ThreadingModel", 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(kThreadingModel),
                   static_cast<DWORD>(sizeof(kThreadingModel)));
    RegCloseKey(hsub);
  }
  RegCloseKey(hkey);
  return S_OK;
}

void UnregisterServer() {
  WCHAR clsid[64];
  if (!ClsidToString(c_clsidBanglaPhonetic, clsid, ARRAYSIZE(clsid))) return;
  WCHAR subkey[128];
  wsprintfW(subkey, kClsidKeyFmt, clsid);
  RegDeleteTreeW(HKEY_CLASSES_ROOT, subkey);
}

// Registers the TSF text service, language profile and TIP category.
HRESULT RegisterProfiles() {
  ITfInputProcessorProfiles* profiles = nullptr;
  HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr,
                                CLSCTX_INPROC_SERVER,
                                IID_ITfInputProcessorProfiles,
                                reinterpret_cast<void**>(&profiles));
  if (FAILED(hr)) return hr;

  hr = profiles->Register(c_clsidBanglaPhonetic);
  if (SUCCEEDED(hr)) {
    WCHAR module[MAX_PATH];
    GetModuleFileNameW(g_hInst, module, ARRAYSIZE(module));
    hr = profiles->AddLanguageProfile(
        c_clsidBanglaPhonetic, kLangIdBangla, c_guidProfile,
        kProfileDescription, static_cast<ULONG>(lstrlenW(kProfileDescription)),
        module, static_cast<ULONG>(lstrlenW(module)), 0);
  }
  profiles->Release();
  if (FAILED(hr)) return hr;

  ITfCategoryMgr* category_mgr = nullptr;
  hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                        IID_ITfCategoryMgr,
                        reinterpret_cast<void**>(&category_mgr));
  if (FAILED(hr)) return hr;
  hr = category_mgr->RegisterCategory(c_clsidBanglaPhonetic,
                                      GUID_TFCAT_TIP_KEYBOARD,
                                      c_clsidBanglaPhonetic);
  category_mgr->Release();
  return hr;
}

void UnregisterProfiles() {
  ITfInputProcessorProfiles* profiles = nullptr;
  if (SUCCEEDED(CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr,
                                 CLSCTX_INPROC_SERVER,
                                 IID_ITfInputProcessorProfiles,
                                 reinterpret_cast<void**>(&profiles)))) {
    profiles->Unregister(c_clsidBanglaPhonetic);
    profiles->Release();
  }
}

}  // namespace

STDAPI DllRegisterServer() {
  HRESULT hr = RegisterServer();
  if (SUCCEEDED(hr)) hr = RegisterProfiles();
  if (FAILED(hr)) {
    UnregisterProfiles();
    UnregisterServer();
  }
  return hr;
}

STDAPI DllUnregisterServer() {
  UnregisterProfiles();
  UnregisterServer();
  return S_OK;
}
