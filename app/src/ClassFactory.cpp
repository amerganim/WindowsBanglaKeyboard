#include <new>

#include "Globals.h"
#include "TextService.h"

// Minimal COM class factory for CTextService.
class CClassFactory : public IClassFactory {
 public:
  CClassFactory() : ref_(1) { DllAddRef(); }

  STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
    if (ppv == nullptr) return E_INVALIDARG;
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IClassFactory)) {
      *ppv = static_cast<IClassFactory*>(this);
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

  STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid,
                              void** ppv) override {
    if (ppv == nullptr) return E_INVALIDARG;
    *ppv = nullptr;
    if (pUnkOuter != nullptr) return CLASS_E_NOAGGREGATION;
    CTextService* svc = new (std::nothrow) CTextService();
    if (svc == nullptr) return E_OUTOFMEMORY;
    HRESULT hr = svc->QueryInterface(riid, ppv);
    svc->Release();
    return hr;
  }
  STDMETHODIMP LockServer(BOOL fLock) override {
    if (fLock)
      DllAddRef();
    else
      DllRelease();
    return S_OK;
  }

 private:
  ~CClassFactory() { DllRelease(); }
  LONG ref_;
};

HRESULT CreateClassFactory(REFCLSID rclsid, REFIID riid, void** ppv) {
  if (!IsEqualCLSID(rclsid, c_clsidBanglaPhonetic)) return CLASS_E_CLASSNOTAVAILABLE;
  CClassFactory* factory = new (std::nothrow) CClassFactory();
  if (factory == nullptr) return E_OUTOFMEMORY;
  HRESULT hr = factory->QueryInterface(riid, ppv);
  factory->Release();
  return hr;
}
