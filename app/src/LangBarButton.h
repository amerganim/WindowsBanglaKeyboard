#pragma once

#include "Globals.h"

class CTextService;

// A button on the Windows language bar / input indicator that shows the current
// mode (Bangla "বাং" vs English "Eng") and toggles it when clicked. It mirrors
// the same state as the Ctrl+Shift+B preserved key.
class CLangBarButton : public ITfLangBarItemButton, public ITfSource {
 public:
  explicit CLangBarButton(CTextService* owner);

  // IUnknown
  STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
  STDMETHODIMP_(ULONG) AddRef() override;
  STDMETHODIMP_(ULONG) Release() override;

  // ITfLangBarItem
  STDMETHODIMP GetInfo(TF_LANGBARITEMINFO* pInfo) override;
  STDMETHODIMP GetStatus(DWORD* pdwStatus) override;
  STDMETHODIMP Show(BOOL fShow) override;
  STDMETHODIMP GetTooltipString(BSTR* pbstrToolTip) override;

  // ITfLangBarItemButton
  STDMETHODIMP OnClick(TfLBIClick click, POINT pt, const RECT* prcArea) override;
  STDMETHODIMP InitMenu(ITfMenu* pMenu) override;
  STDMETHODIMP OnMenuSelect(UINT wID) override;
  STDMETHODIMP GetIcon(HICON* phIcon) override;
  STDMETHODIMP GetText(BSTR* pbstrText) override;

  // ITfSource
  STDMETHODIMP AdviseSink(REFIID riid, IUnknown* punk, DWORD* pdwCookie) override;
  STDMETHODIMP UnadviseSink(DWORD dwCookie) override;

  // Called by the text service when the mode changes so the button redraws.
  void NotifyUpdate();

 private:
  ~CLangBarButton();

  LONG ref_;
  CTextService* owner_;          // not owned (parent text service)
  ITfLangBarItemSink* sink_;     // single update sink
};
