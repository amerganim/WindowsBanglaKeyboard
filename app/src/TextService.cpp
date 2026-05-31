#include "TextService.h"

#include "bnphonetic/Transliterator.h"

CTextService::CTextService()
    : ref_(1), thread_mgr_(nullptr), client_id_(TF_CLIENTID_NULL) {
  DllAddRef();
}

CTextService::~CTextService() { DllRelease(); }

// ---- IUnknown ----
STDMETHODIMP CTextService::QueryInterface(REFIID riid, void** ppv) {
  if (ppv == nullptr) return E_INVALIDARG;
  *ppv = nullptr;
  if (IsEqualIID(riid, IID_IUnknown) ||
      IsEqualIID(riid, IID_ITfTextInputProcessor) ||
      IsEqualIID(riid, IID_ITfTextInputProcessorEx)) {
    *ppv = static_cast<ITfTextInputProcessorEx*>(this);
  } else if (IsEqualIID(riid, IID_ITfKeyEventSink)) {
    *ppv = static_cast<ITfKeyEventSink*>(this);
  }
  if (*ppv) {
    AddRef();
    return S_OK;
  }
  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CTextService::AddRef() { return InterlockedIncrement(&ref_); }

STDMETHODIMP_(ULONG) CTextService::Release() {
  LONG c = InterlockedDecrement(&ref_);
  if (c == 0) delete this;
  return c;
}

// ---- Activation ----
STDMETHODIMP CTextService::Activate(ITfThreadMgr* ptim, TfClientId tid) {
  return ActivateEx(ptim, tid, 0);
}

STDMETHODIMP CTextService::ActivateEx(ITfThreadMgr* ptim, TfClientId tid,
                                      DWORD /*dwFlags*/) {
  thread_mgr_ = ptim;
  thread_mgr_->AddRef();
  client_id_ = tid;
  buffer_.clear();

  // Register for keystroke notifications.
  ITfKeystrokeMgr* keystroke_mgr = nullptr;
  if (SUCCEEDED(thread_mgr_->QueryInterface(IID_ITfKeystrokeMgr,
                                            reinterpret_cast<void**>(&keystroke_mgr)))) {
    keystroke_mgr->AdviseKeyEventSink(client_id_,
                                      static_cast<ITfKeyEventSink*>(this), TRUE);
    keystroke_mgr->Release();
  }
  return S_OK;
}

STDMETHODIMP CTextService::Deactivate() {
  if (thread_mgr_) {
    ITfKeystrokeMgr* keystroke_mgr = nullptr;
    if (SUCCEEDED(thread_mgr_->QueryInterface(
            IID_ITfKeystrokeMgr, reinterpret_cast<void**>(&keystroke_mgr)))) {
      keystroke_mgr->UnadviseKeyEventSink(client_id_);
      keystroke_mgr->Release();
    }
    thread_mgr_->Release();
    thread_mgr_ = nullptr;
  }
  client_id_ = TF_CLIENTID_NULL;
  buffer_.clear();
  return S_OK;
}

// ---- ITfKeyEventSink ----
STDMETHODIMP CTextService::OnSetFocus(BOOL /*fForeground*/) { return S_OK; }

STDMETHODIMP CTextService::OnTestKeyDown(ITfContext* /*pic*/, WPARAM /*wParam*/,
                                         LPARAM /*lParam*/, BOOL* pfEaten) {
  // Phase 0: observe only, do not consume keys yet.
  if (pfEaten) *pfEaten = FALSE;
  return S_OK;
}

STDMETHODIMP CTextService::OnTestKeyUp(ITfContext* /*pic*/, WPARAM /*wParam*/,
                                       LPARAM /*lParam*/, BOOL* pfEaten) {
  if (pfEaten) *pfEaten = FALSE;
  return S_OK;
}

STDMETHODIMP CTextService::OnKeyDown(ITfContext* /*pic*/, WPARAM wParam,
                                     LPARAM /*lParam*/, BOOL* pfEaten) {
  if (pfEaten) *pfEaten = FALSE;

  const WPARAM vk = wParam;
  if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) {
    // Respect Shift to preserve letter case (the engine is case-sensitive).
    bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    char ch = static_cast<char>(vk);
    if (vk >= 'A' && vk <= 'Z' && !shift) ch = static_cast<char>(vk - 'A' + 'a');
    buffer_.push_back(ch);
  } else if (vk == VK_SPACE || vk == VK_RETURN) {
    buffer_.clear();  // word boundary
  } else if (vk == VK_BACK && !buffer_.empty()) {
    buffer_.pop_back();
  }

  // Trace the running transliteration so we can see the engine working
  // in-process (DebugView / debugger output). Phase 3 replaces this with a
  // real composition committed into the document.
  const std::string bangla = bnphonetic::Transliterate(buffer_);
  OutputDebugStringA(("[BanglaPhonetic] \"" + buffer_ + "\" -> \"" + bangla +
                      "\"\n").c_str());
  return S_OK;
}

STDMETHODIMP CTextService::OnKeyUp(ITfContext* /*pic*/, WPARAM /*wParam*/,
                                   LPARAM /*lParam*/, BOOL* pfEaten) {
  if (pfEaten) *pfEaten = FALSE;
  return S_OK;
}

STDMETHODIMP CTextService::OnPreservedKey(ITfContext* /*pic*/, REFGUID /*rguid*/,
                                          BOOL* pfEaten) {
  if (pfEaten) *pfEaten = FALSE;
  return S_OK;
}
