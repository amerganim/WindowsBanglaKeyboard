#include "LangBarButton.h"

#include <olectl.h>
#include <shellapi.h>
#include <strsafe.h>

#include <string>

#include "TextService.h"

namespace {
const UINT kMenuBangla = 1;
const UINT kMenuEnglish = 2;
const UINT kMenuGuide = 3;
const DWORD kSinkCookie = 0x42;

// Open the bundled typing guide (KEYMAP.html, installed next to this DLL) in
// the default browser.
void OpenTypingGuide() {
  WCHAR path[MAX_PATH];
  DWORD n = GetModuleFileNameW(g_hInst, path, ARRAYSIZE(path));
  if (n == 0 || n >= ARRAYSIZE(path)) return;
  WCHAR* slash = wcsrchr(path, L'\\');
  if (slash == nullptr) return;
  *(slash + 1) = L'\0';
  std::wstring guide = std::wstring(path) + L"KEYMAP.html";
  ShellExecuteW(nullptr, L"open", guide.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}
}  // namespace

CLangBarButton::CLangBarButton(CTextService* owner)
    : ref_(1), owner_(owner), sink_(nullptr) {}

CLangBarButton::~CLangBarButton() {
  if (sink_) sink_->Release();
}

// ---- IUnknown ----
STDMETHODIMP CLangBarButton::QueryInterface(REFIID riid, void** ppv) {
  if (ppv == nullptr) return E_INVALIDARG;
  *ppv = nullptr;
  if (IsEqualIID(riid, IID_IUnknown) ||
      IsEqualIID(riid, IID_ITfLangBarItem) ||
      IsEqualIID(riid, IID_ITfLangBarItemButton)) {
    *ppv = static_cast<ITfLangBarItemButton*>(this);
  } else if (IsEqualIID(riid, IID_ITfSource)) {
    *ppv = static_cast<ITfSource*>(this);
  }
  if (*ppv) {
    AddRef();
    return S_OK;
  }
  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CLangBarButton::AddRef() { return InterlockedIncrement(&ref_); }

STDMETHODIMP_(ULONG) CLangBarButton::Release() {
  LONG c = InterlockedDecrement(&ref_);
  if (c == 0) delete this;
  return c;
}

// ---- ITfLangBarItem ----
STDMETHODIMP CLangBarButton::GetInfo(TF_LANGBARITEMINFO* pInfo) {
  if (pInfo == nullptr) return E_INVALIDARG;
  pInfo->clsidService = c_clsidBanglaPhonetic;
  pInfo->guidItem = c_guidLangBarItem;
  pInfo->dwStyle = TF_LBI_STYLE_BTN_BUTTON | TF_LBI_STYLE_SHOWNINTRAY;
  pInfo->ulSort = 0;
  StringCchCopyW(pInfo->szDescription, ARRAYSIZE(pInfo->szDescription),
                 L"Amader Bangla Keyboard");
  return S_OK;
}

STDMETHODIMP CLangBarButton::GetStatus(DWORD* pdwStatus) {
  if (pdwStatus == nullptr) return E_INVALIDARG;
  *pdwStatus = 0;  // visible and enabled
  return S_OK;
}

STDMETHODIMP CLangBarButton::Show(BOOL /*fShow*/) { return S_OK; }

STDMETHODIMP CLangBarButton::GetTooltipString(BSTR* pbstrToolTip) {
  if (pbstrToolTip == nullptr) return E_INVALIDARG;
  *pbstrToolTip =
      SysAllocString(L"Amader Bangla Keyboard — click or press Ctrl+Shift+B to toggle");
  return *pbstrToolTip ? S_OK : E_OUTOFMEMORY;
}

// ---- ITfLangBarItemButton ----
STDMETHODIMP CLangBarButton::OnClick(TfLBIClick click, POINT /*pt*/,
                                     const RECT* /*prcArea*/) {
  if (click == TF_LBI_CLK_LEFT && owner_) owner_->ToggleMode();
  return S_OK;
}

STDMETHODIMP CLangBarButton::InitMenu(ITfMenu* pMenu) {
  if (pMenu == nullptr) return E_INVALIDARG;
  const bool enabled = owner_ && owner_->IsEnabled();
  pMenu->AddMenuItem(kMenuBangla,
                     enabled ? TF_LBMENUF_RADIOCHECKED : 0, nullptr, nullptr,
                     L"Bangla", 6, nullptr);
  pMenu->AddMenuItem(kMenuEnglish,
                     enabled ? 0 : TF_LBMENUF_RADIOCHECKED, nullptr, nullptr,
                     L"English", 7, nullptr);
  pMenu->AddMenuItem(0, TF_LBMENUF_SEPARATOR, nullptr, nullptr, nullptr, 0,
                     nullptr);
  pMenu->AddMenuItem(kMenuGuide, 0, nullptr, nullptr, L"Typing guide…", 13,
                     nullptr);
  return S_OK;
}

STDMETHODIMP CLangBarButton::OnMenuSelect(UINT wID) {
  if (owner_ == nullptr) return S_OK;
  if (wID == kMenuBangla)
    owner_->SetEnabled(true);
  else if (wID == kMenuEnglish)
    owner_->SetEnabled(false);
  else if (wID == kMenuGuide)
    OpenTypingGuide();
  return S_OK;
}

STDMETHODIMP CLangBarButton::GetIcon(HICON* phIcon) {
  if (phIcon == nullptr) return E_INVALIDARG;
  *phIcon = nullptr;  // text-only button
  return S_OK;
}

STDMETHODIMP CLangBarButton::GetText(BSTR* pbstrText) {
  if (pbstrText == nullptr) return E_INVALIDARG;
  const bool enabled = owner_ && owner_->IsEnabled();
  *pbstrText = SysAllocString(enabled ? L"বাং" /* বাং */
                                      : L"Eng");
  return *pbstrText ? S_OK : E_OUTOFMEMORY;
}

// ---- ITfSource ----
STDMETHODIMP CLangBarButton::AdviseSink(REFIID riid, IUnknown* punk,
                                        DWORD* pdwCookie) {
  if (punk == nullptr || pdwCookie == nullptr) return E_INVALIDARG;
  if (!IsEqualIID(riid, IID_ITfLangBarItemSink)) return CONNECT_E_CANNOTCONNECT;
  if (sink_ != nullptr) return CONNECT_E_ADVISELIMIT;
  if (FAILED(punk->QueryInterface(IID_ITfLangBarItemSink,
                                  reinterpret_cast<void**>(&sink_))))
    return E_NOINTERFACE;
  *pdwCookie = kSinkCookie;
  return S_OK;
}

STDMETHODIMP CLangBarButton::UnadviseSink(DWORD dwCookie) {
  if (dwCookie != kSinkCookie || sink_ == nullptr) return CONNECT_E_NOCONNECTION;
  sink_->Release();
  sink_ = nullptr;
  return S_OK;
}

void CLangBarButton::NotifyUpdate() {
  if (sink_) sink_->OnUpdate(TF_LBI_TEXT | TF_LBI_ICON | TF_LBI_STATUS);
}
