#pragma once

#include <string>
#include <vector>

#include "CandidateWindow.h"
#include "Globals.h"
#include "bnphonetic/Suggester.h"

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
  // Finalize the composition (keeping the committed text). Optionally replace
  // the committed text with `override_text` (used to commit a chosen
  // suggestion) and/or insert `trailing` (e.g. a space) right after it.
  HRESULT EndComposition(ITfContext* pic,
                         const std::wstring& trailing = std::wstring(),
                         const std::wstring& override_text = std::wstring());
  bool IsComposing() const { return composition_ != nullptr; }

  // Recompute suggestions for the current buffer and show/hide the candidate
  // window near the caret; commit the currently selected suggestion.
  void RefreshCandidates(ITfContext* pic);
  HRESULT CommitSelected(ITfContext* pic, const std::wstring& trailing);

  // True if we will consume this key (kept identical between OnTestKeyDown and
  // OnKeyDown, as TSF requires). `ch` is the key's translated character (0 if
  // none).
  bool ShouldEat(WPARAM vk, char ch) const;

  // Language-bar button + preserved key.
  void AddLangBarButton();
  void RemoveLangBarButton();
  void PreserveToggleKey(bool add);

  // Suggestion dictionary + per-user learned frequencies.
  void LoadDictionaries();   // bundled dictionary.tsv + user learned.tsv
  void SaveLearned() const;  // persist learned.tsv

  LONG ref_;
  ITfThreadMgr* thread_mgr_;
  TfClientId client_id_;
  ITfComposition* composition_;  // current composition, or null
  TfGuidAtom display_attribute_atom_;
  CLangBarButton* langbar_button_;
  bool enabled_;        // true = transliterate to Bangla; false = pass English
  std::string buffer_;  // accumulated Latin keys for the current word

  CandidateWindow candidates_;
  bnphonetic::Suggester suggester_;
  std::vector<std::string> suggestions_;  // UTF-8, index-aligned with the window
  RECT caret_rect_;                        // screen rect of the composition
  bool caret_rect_valid_;
};
