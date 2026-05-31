#pragma once

#include <string>

#include "Globals.h"

class CLangBarButton;

// The Text Input Processor. It captures keystrokes, runs the phonetic engine,
// and shows the running result as an underlined TSF composition that is
// committed into the document on space/enter (Phase 3).
class CTextService : public ITfTextInputProcessorEx,
                     public ITfKeyEventSink,
                     public ITfCompositionSink,
                     public ITfDisplayAttributeProvider {
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

  // ITfCompositionSink
  STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite,
                                       ITfComposition* pComposition) override;

  // ITfDisplayAttributeProvider
  STDMETHODIMP EnumDisplayAttributeInfo(
      IEnumTfDisplayAttributeInfo** ppEnum) override;
  STDMETHODIMP GetDisplayAttributeInfo(
      REFGUID guid, ITfDisplayAttributeInfo** ppInfo) override;

  // Bangla/English mode, shared by the language-bar button and the
  // Ctrl+Shift+B preserved key.
  bool IsEnabled() const { return enabled_; }
  void SetEnabled(bool enabled);
  void ToggleMode() { SetEnabled(!enabled_); }

 private:
  ~CTextService();

  // Composition lifecycle (each runs inside an edit session).
  HRESULT UpdateComposition(ITfContext* pic);  // start if needed, set text
  HRESULT EndComposition(ITfContext* pic);      // finalize, keep committed text
  bool IsComposing() const { return composition_ != nullptr; }

  // Language-bar button, preserved key, and persisted config.
  void AddLangBarButton();
  void RemoveLangBarButton();
  void PreserveToggleKey(bool add);
  void LoadConfig();
  void SaveConfig() const;

  LONG ref_;
  ITfThreadMgr* thread_mgr_;
  TfClientId client_id_;
  ITfComposition* composition_;  // current composition, or null
  TfGuidAtom display_attribute_atom_;
  CLangBarButton* langbar_button_;
  bool enabled_;        // true = transliterate to Bangla; false = pass English
  std::string buffer_;  // accumulated Latin keys for the current word
};
