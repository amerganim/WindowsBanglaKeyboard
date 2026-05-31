#pragma once

#include <functional>

#include "Globals.h"

// A generic ITfEditSession that runs a callback inside the document's edit
// context. TSF only grants read/write access to a document from within an edit
// session, so all composition manipulation happens through one of these.
class CEditSession : public ITfEditSession {
 public:
  using Callback = std::function<HRESULT(TfEditCookie)>;

  explicit CEditSession(Callback fn) : ref_(1), fn_(std::move(fn)) {}

  STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
    if (ppv == nullptr) return E_INVALIDARG;
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfEditSession)) {
      *ppv = static_cast<ITfEditSession*>(this);
      AddRef();
      return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
  }
  STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&ref_); }
  STDMETHODIMP_(ULONG) Release() override {
    LONG c = InterlockedDecrement(&ref_);
    if (c == 0) delete this;
    return c;
  }

  STDMETHODIMP DoEditSession(TfEditCookie ec) override { return fn_(ec); }

 private:
  ~CEditSession() = default;
  LONG ref_;
  Callback fn_;
};
