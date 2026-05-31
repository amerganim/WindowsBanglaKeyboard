#pragma once

#include "Globals.h"

// Display attribute info for the in-progress (composing) text: a dotted
// underline so the user can see the uncommitted Bangla before it is committed.
// The text service exposes these via ITfDisplayAttributeProvider and tags the
// composition range with GUID_PROP_ATTRIBUTE.
class CDisplayAttributeInfo : public ITfDisplayAttributeInfo {
 public:
  CDisplayAttributeInfo();

  STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
  STDMETHODIMP_(ULONG) AddRef() override;
  STDMETHODIMP_(ULONG) Release() override;

  STDMETHODIMP GetGUID(GUID* pguid) override;
  STDMETHODIMP GetDescription(BSTR* pbstrDesc) override;
  STDMETHODIMP GetAttributeInfo(TF_DISPLAYATTRIBUTE* pda) override;
  STDMETHODIMP SetAttributeInfo(const TF_DISPLAYATTRIBUTE* pda) override;
  STDMETHODIMP Reset() override;

 private:
  ~CDisplayAttributeInfo() = default;
  LONG ref_;
};

// Single-element enumerator over the display attribute(s) above.
class CEnumDisplayAttributeInfo : public IEnumTfDisplayAttributeInfo {
 public:
  CEnumDisplayAttributeInfo();

  STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
  STDMETHODIMP_(ULONG) AddRef() override;
  STDMETHODIMP_(ULONG) Release() override;

  STDMETHODIMP Clone(IEnumTfDisplayAttributeInfo** ppEnum) override;
  STDMETHODIMP Next(ULONG ulCount, ITfDisplayAttributeInfo** rgInfo,
                    ULONG* pcFetched) override;
  STDMETHODIMP Reset() override;
  STDMETHODIMP Skip(ULONG ulCount) override;

 private:
  ~CEnumDisplayAttributeInfo() = default;
  LONG ref_;
  ULONG index_;  // 0 = not yet returned, 1 = exhausted
};
