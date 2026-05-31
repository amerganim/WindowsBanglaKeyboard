#pragma once

#include <string>

#include "Globals.h"

// The Text Input Processor itself. Implements the activation entry points and a
// keystroke sink. In this Phase 0 skeleton the keystroke sink feeds the
// phonetic engine and traces the result; consuming keys and committing Bangla
// into the document is Phase 3.
class CTextService : public ITfTextInputProcessorEx,
                     public ITfKeyEventSink {
 public:
  CTextService();

  // IUnknown
  STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
  STDMETHODIMP_(ULONG) AddRef() override;
  STDMETHODIMP_(ULONG) Release() override;

  // ITfTextInputProcessor
  STDMETHODIMP Activate(ITfThreadMgr* ptim, TfClientId tid) override;
  STDMETHODIMP Deactivate() override;

  // ITfTextInputProcessorEx
  STDMETHODIMP ActivateEx(ITfThreadMgr* ptim, TfClientId tid,
                          DWORD dwFlags) override;

  // ITfKeyEventSink
  STDMETHODIMP OnSetFocus(BOOL fForeground) override;
  STDMETHODIMP OnTestKeyDown(ITfContext* pic, WPARAM wParam, LPARAM lParam,
                             BOOL* pfEaten) override;
  STDMETHODIMP OnTestKeyUp(ITfContext* pic, WPARAM wParam, LPARAM lParam,
                           BOOL* pfEaten) override;
  STDMETHODIMP OnKeyDown(ITfContext* pic, WPARAM wParam, LPARAM lParam,
                         BOOL* pfEaten) override;
  STDMETHODIMP OnKeyUp(ITfContext* pic, WPARAM wParam, LPARAM lParam,
                       BOOL* pfEaten) override;
  STDMETHODIMP OnPreservedKey(ITfContext* pic, REFGUID rguid,
                              BOOL* pfEaten) override;

 private:
  ~CTextService();

  LONG ref_;
  ITfThreadMgr* thread_mgr_;
  TfClientId client_id_;
  std::string buffer_;  // accumulated Latin keys for the current word
};
