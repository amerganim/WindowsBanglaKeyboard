#include "DisplayAttribute.h"

#include <new>

// ---- CDisplayAttributeInfo ----

CDisplayAttributeInfo::CDisplayAttributeInfo() : ref_(1) {}

STDMETHODIMP CDisplayAttributeInfo::QueryInterface(REFIID riid, void** ppv) {
  if (ppv == nullptr) return E_INVALIDARG;
  if (IsEqualIID(riid, IID_IUnknown) ||
      IsEqualIID(riid, IID_ITfDisplayAttributeInfo)) {
    *ppv = static_cast<ITfDisplayAttributeInfo*>(this);
    AddRef();
    return S_OK;
  }
  *ppv = nullptr;
  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CDisplayAttributeInfo::AddRef() {
  return InterlockedIncrement(&ref_);
}

STDMETHODIMP_(ULONG) CDisplayAttributeInfo::Release() {
  LONG c = InterlockedDecrement(&ref_);
  if (c == 0) delete this;
  return c;
}

STDMETHODIMP CDisplayAttributeInfo::GetGUID(GUID* pguid) {
  if (pguid == nullptr) return E_INVALIDARG;
  *pguid = c_guidDisplayAttribute;
  return S_OK;
}

STDMETHODIMP CDisplayAttributeInfo::GetDescription(BSTR* pbstrDesc) {
  if (pbstrDesc == nullptr) return E_INVALIDARG;
  *pbstrDesc = SysAllocString(L"Amader Bangla Keyboard composing text");
  return *pbstrDesc ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CDisplayAttributeInfo::GetAttributeInfo(TF_DISPLAYATTRIBUTE* pda) {
  if (pda == nullptr) return E_INVALIDARG;
  TF_DISPLAYATTRIBUTE da = {};
  da.crText.type = TF_CT_NONE;
  da.crBk.type = TF_CT_NONE;
  da.crLine.type = TF_CT_NONE;
  da.lsStyle = TF_LS_DOT;  // dotted underline
  da.fBoldLine = FALSE;
  da.bAttr = TF_ATTR_INPUT;
  *pda = da;
  return S_OK;
}

STDMETHODIMP CDisplayAttributeInfo::SetAttributeInfo(
    const TF_DISPLAYATTRIBUTE* /*pda*/) {
  return E_NOTIMPL;  // fixed attribute
}

STDMETHODIMP CDisplayAttributeInfo::Reset() { return S_OK; }

// ---- CEnumDisplayAttributeInfo ----

CEnumDisplayAttributeInfo::CEnumDisplayAttributeInfo() : ref_(1), index_(0) {}

STDMETHODIMP CEnumDisplayAttributeInfo::QueryInterface(REFIID riid, void** ppv) {
  if (ppv == nullptr) return E_INVALIDARG;
  if (IsEqualIID(riid, IID_IUnknown) ||
      IsEqualIID(riid, IID_IEnumTfDisplayAttributeInfo)) {
    *ppv = static_cast<IEnumTfDisplayAttributeInfo*>(this);
    AddRef();
    return S_OK;
  }
  *ppv = nullptr;
  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CEnumDisplayAttributeInfo::AddRef() {
  return InterlockedIncrement(&ref_);
}

STDMETHODIMP_(ULONG) CEnumDisplayAttributeInfo::Release() {
  LONG c = InterlockedDecrement(&ref_);
  if (c == 0) delete this;
  return c;
}

STDMETHODIMP CEnumDisplayAttributeInfo::Clone(
    IEnumTfDisplayAttributeInfo** ppEnum) {
  if (ppEnum == nullptr) return E_INVALIDARG;
  CEnumDisplayAttributeInfo* clone = new (std::nothrow) CEnumDisplayAttributeInfo();
  if (clone == nullptr) return E_OUTOFMEMORY;
  clone->index_ = index_;
  *ppEnum = clone;
  return S_OK;
}

STDMETHODIMP CEnumDisplayAttributeInfo::Next(ULONG ulCount,
                                             ITfDisplayAttributeInfo** rgInfo,
                                             ULONG* pcFetched) {
  ULONG fetched = 0;
  while (fetched < ulCount && index_ < 1) {
    CDisplayAttributeInfo* info = new (std::nothrow) CDisplayAttributeInfo();
    if (info == nullptr) {
      if (pcFetched) *pcFetched = fetched;
      return E_OUTOFMEMORY;
    }
    rgInfo[fetched] = info;
    ++fetched;
    ++index_;
  }
  if (pcFetched) *pcFetched = fetched;
  return (fetched == ulCount) ? S_OK : S_FALSE;
}

STDMETHODIMP CEnumDisplayAttributeInfo::Reset() {
  index_ = 0;
  return S_OK;
}

STDMETHODIMP CEnumDisplayAttributeInfo::Skip(ULONG ulCount) {
  index_ += ulCount;
  if (index_ > 1) index_ = 1;
  return S_OK;
}
