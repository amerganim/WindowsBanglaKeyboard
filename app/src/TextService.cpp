#include "TextService.h"

#include <fstream>
#include <new>

#include "DisplayAttribute.h"
#include "EditSession.h"
#include "LangBarButton.h"
#include "bnphonetic/Transliterator.h"

namespace {

std::wstring Utf8ToUtf16(const std::string& s) {
  if (s.empty()) return std::wstring();
  int n = MultiByteToWideChar(CP_UTF8, 0, s.data(),
                              static_cast<int>(s.size()), nullptr, 0);
  std::wstring w(static_cast<size_t>(n), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()),
                      w.empty() ? nullptr : &w[0], n);
  return w;
}

// Translate a key event to the ASCII character it produces on the user's
// keyboard layout (respecting Shift/CapsLock, etc.), or 0 if it is not a
// single printable ASCII character. The engine is case-sensitive, so this also
// gives us correct upper/lower case and sign keys ('^', ':', '.').
char TranslateChar(WPARAM vk, LPARAM lParam) {
  BYTE kbState[256];
  if (!GetKeyboardState(kbState)) return 0;
  const UINT scan = static_cast<UINT>((lParam >> 16) & 0xFF);
  const HKL layout = GetKeyboardLayout(0);
  WCHAR buf[8] = {0};
  int n = ToUnicodeEx(static_cast<UINT>(vk), scan, kbState, buf, 8, 0, layout);
  if (n < 0) {
    // Dead key: call again to flush so the next key isn't corrupted.
    ToUnicodeEx(static_cast<UINT>(vk), scan, kbState, buf, 8, 0, layout);
    return 0;
  }
  if (n == 1 && buf[0] >= 0x20 && buf[0] < 0x7F) return static_cast<char>(buf[0]);
  return 0;
}

// Modifier / lock keys produce no character and must never disturb an
// in-progress composition (pressing Shift to type a capital vowel must not
// commit the word).
bool IsModifierKey(WPARAM vk) {
  switch (vk) {
    case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
    case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
    case VK_MENU: case VK_LMENU: case VK_RMENU:
    case VK_CAPITAL: case VK_NUMLOCK: case VK_SCROLL:
    case VK_LWIN: case VK_RWIN:
      return true;
    default:
      return false;
  }
}

// Directory containing this DLL (with trailing backslash), where the bundled
// dictionary.tsv lives.
std::wstring ModuleDir() {
  WCHAR path[MAX_PATH];
  DWORD n = GetModuleFileNameW(g_hInst, path, ARRAYSIZE(path));
  if (n == 0 || n >= ARRAYSIZE(path)) return std::wstring();
  WCHAR* slash = wcsrchr(path, L'\\');
  if (slash == nullptr) return std::wstring();
  return std::wstring(path, slash - path + 1);
}

// Per-user data directory %LOCALAPPDATA%\BanglaPhonetic (no trailing backslash).
std::wstring UserDataDir() {
  WCHAR buf[MAX_PATH];
  DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", buf, ARRAYSIZE(buf));
  if (n == 0 || n >= ARRAYSIZE(buf)) return std::wstring();
  return std::wstring(buf) + L"\\BanglaPhonetic";
}

std::string ReadFileUtf8(const std::wstring& path) {
  std::ifstream f(path.c_str(), std::ios::binary);
  if (!f) return std::string();
  return std::string((std::istreambuf_iterator<char>(f)),
                     std::istreambuf_iterator<char>());
}

void WriteFileUtf8(const std::wstring& path, const std::string& data) {
  std::ofstream f(path.c_str(), std::ios::binary | std::ios::trunc);
  if (f) f.write(data.data(), static_cast<std::streamsize>(data.size()));
}

}  // namespace

CTextService::CTextService()
    : ref_(1),
      thread_mgr_(nullptr),
      client_id_(TF_CLIENTID_NULL),
      composition_(nullptr),
      display_attribute_atom_(TF_INVALID_GUIDATOM),
      langbar_button_(nullptr),
      enabled_(true),
      caret_rect_{},
      caret_rect_valid_(false) {
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
  } else if (IsEqualIID(riid, IID_ITfCompositionSink)) {
    *ppv = static_cast<ITfCompositionSink*>(this);
  } else if (IsEqualIID(riid, IID_ITfDisplayAttributeProvider)) {
    *ppv = static_cast<ITfDisplayAttributeProvider*>(this);
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
  // Switching into the Bangla Phonetic input method always starts in Bangla
  // mode; Ctrl+Shift+B is a within-session convenience, not a sticky setting.
  enabled_ = true;

  // Map our display attribute GUID to an atom for tagging composition ranges.
  ITfCategoryMgr* category_mgr = nullptr;
  if (SUCCEEDED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr,
                                 CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr,
                                 reinterpret_cast<void**>(&category_mgr)))) {
    category_mgr->RegisterGUID(c_guidDisplayAttribute, &display_attribute_atom_);
    category_mgr->Release();
  }

  // Register for keystroke notifications.
  ITfKeystrokeMgr* keystroke_mgr = nullptr;
  if (SUCCEEDED(thread_mgr_->QueryInterface(
          IID_ITfKeystrokeMgr, reinterpret_cast<void**>(&keystroke_mgr)))) {
    keystroke_mgr->AdviseKeyEventSink(client_id_,
                                      static_cast<ITfKeyEventSink*>(this), TRUE);
    keystroke_mgr->Release();
  }

  PreserveToggleKey(true);
  AddLangBarButton();
  LoadDictionaries();
  return S_OK;
}

STDMETHODIMP CTextService::Deactivate() {
  SaveLearned();
  candidates_.Hide();
  RemoveLangBarButton();
  PreserveToggleKey(false);

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
  if (composition_) {
    composition_->Release();
    composition_ = nullptr;
  }
  client_id_ = TF_CLIENTID_NULL;
  display_attribute_atom_ = TF_INVALID_GUIDATOM;
  buffer_.clear();
  return S_OK;
}

// ---- Composition ----
HRESULT CTextService::UpdateComposition(ITfContext* pic) {
  if (pic == nullptr) return E_INVALIDARG;
  const std::wstring text = Utf8ToUtf16(bnphonetic::Transliterate(buffer_));

  CEditSession* session = new (std::nothrow) CEditSession(
      [this, pic, text](TfEditCookie ec) -> HRESULT {
        // Start the composition lazily at the current selection.
        if (composition_ == nullptr) {
          ITfInsertAtSelection* insert_at_sel = nullptr;
          if (FAILED(pic->QueryInterface(
                  IID_ITfInsertAtSelection,
                  reinterpret_cast<void**>(&insert_at_sel))))
            return E_FAIL;
          ITfRange* range = nullptr;
          HRESULT hr = insert_at_sel->InsertTextAtSelection(
              ec, TF_IAS_QUERYONLY, nullptr, 0, &range);
          insert_at_sel->Release();
          if (FAILED(hr) || range == nullptr) return E_FAIL;

          ITfContextComposition* ctx_composition = nullptr;
          if (SUCCEEDED(pic->QueryInterface(
                  IID_ITfContextComposition,
                  reinterpret_cast<void**>(&ctx_composition)))) {
            ctx_composition->StartComposition(
                ec, range, static_cast<ITfCompositionSink*>(this),
                &composition_);
            ctx_composition->Release();
          }
          range->Release();
          if (composition_ == nullptr) return E_FAIL;
        }

        // Replace the composition's text with the latest transliteration.
        ITfRange* range = nullptr;
        if (FAILED(composition_->GetRange(&range)) || range == nullptr)
          return E_FAIL;
        range->SetText(ec, 0, text.c_str(), static_cast<LONG>(text.size()));

        // Tag the range so it renders with our underline display attribute.
        if (display_attribute_atom_ != TF_INVALID_GUIDATOM) {
          ITfProperty* prop = nullptr;
          if (SUCCEEDED(pic->GetProperty(GUID_PROP_ATTRIBUTE, &prop)) && prop) {
            VARIANT var;
            VariantInit(&var);
            var.vt = VT_I4;
            var.lVal = display_attribute_atom_;
            prop->SetValue(ec, range, &var);
            prop->Release();
          }
        }

        // Put the caret at the end of the composition.
        ITfRange* caret = nullptr;
        if (SUCCEEDED(range->Clone(&caret)) && caret) {
          caret->Collapse(ec, TF_ANCHOR_END);
          TF_SELECTION sel;
          sel.range = caret;
          sel.style.ase = TF_AE_NONE;
          sel.style.fInterimChar = FALSE;
          pic->SetSelection(ec, 1, &sel);
          caret->Release();
        }

        // Capture the on-screen rect of the composition to position the
        // candidate window beneath it.
        caret_rect_valid_ = false;
        ITfContextView* view = nullptr;
        if (SUCCEEDED(pic->GetActiveView(&view)) && view) {
          RECT rc;
          BOOL clipped = FALSE;
          if (SUCCEEDED(view->GetTextExt(ec, range, &rc, &clipped))) {
            caret_rect_ = rc;
            caret_rect_valid_ = true;
          }
          view->Release();
        }
        range->Release();
        return S_OK;
      });
  if (session == nullptr) return E_OUTOFMEMORY;

  HRESULT hr_session = S_OK;
  pic->RequestEditSession(client_id_, session,
                          TF_ES_SYNC | TF_ES_READWRITE, &hr_session);
  session->Release();

  RefreshCandidates(pic);
  return hr_session;
}

HRESULT CTextService::EndComposition(ITfContext* pic,
                                     const std::wstring& trailing,
                                     const std::wstring& override_text) {
  buffer_.clear();
  candidates_.Hide();
  if (composition_ == nullptr || pic == nullptr) return S_OK;

  CEditSession* session = new (std::nothrow) CEditSession(
      [this, pic, trailing, override_text](TfEditCookie ec) -> HRESULT {
        if (composition_) {
          ITfRange* end = nullptr;
          if (SUCCEEDED(composition_->GetRange(&end)) && end) {
            // Replace the composing text with the chosen suggestion, if any.
            if (!override_text.empty()) {
              end->SetText(ec, 0, override_text.c_str(),
                           static_cast<LONG>(override_text.size()));
            }
            // Move the caret to the END so anything typed next lands after the
            // word rather than before it.
            end->Collapse(ec, TF_ANCHOR_END);
            TF_SELECTION sel;
            sel.range = end;
            sel.style.ase = TF_AE_END;
            sel.style.fInterimChar = FALSE;
            pic->SetSelection(ec, 1, &sel);
            end->Release();
          }
          composition_->EndComposition(ec);
          composition_->Release();
          composition_ = nullptr;
        }
        // Insert any trailing text (e.g. the space that committed the word)
        // after the now-committed Bangla, and place the caret after it.
        if (!trailing.empty()) {
          ITfInsertAtSelection* insert_at_sel = nullptr;
          if (SUCCEEDED(pic->QueryInterface(
                  IID_ITfInsertAtSelection,
                  reinterpret_cast<void**>(&insert_at_sel)))) {
            ITfRange* range = nullptr;
            if (SUCCEEDED(insert_at_sel->InsertTextAtSelection(
                    ec, 0, trailing.c_str(),
                    static_cast<LONG>(trailing.size()), &range)) &&
                range) {
              range->Collapse(ec, TF_ANCHOR_END);
              TF_SELECTION sel;
              sel.range = range;
              sel.style.ase = TF_AE_NONE;
              sel.style.fInterimChar = FALSE;
              pic->SetSelection(ec, 1, &sel);
              range->Release();
            }
            insert_at_sel->Release();
          }
        }
        return S_OK;
      });
  if (session == nullptr) return E_OUTOFMEMORY;

  HRESULT hr_session = S_OK;
  pic->RequestEditSession(client_id_, session,
                          TF_ES_SYNC | TF_ES_READWRITE, &hr_session);
  session->Release();
  return hr_session;
}

void CTextService::RefreshCandidates(ITfContext* /*pic*/) {
  suggestions_ = suggester_.Suggest(buffer_);
  // Only show the list when there are real alternatives beyond the literal
  // transliteration (element 0).
  if (suggestions_.size() > 1 && caret_rect_valid_) {
    std::vector<std::wstring> items;
    items.reserve(suggestions_.size());
    for (const std::string& s : suggestions_) items.push_back(Utf8ToUtf16(s));
    candidates_.Show(items, caret_rect_.left, caret_rect_.bottom);
  } else {
    candidates_.Hide();
  }
}

HRESULT CTextService::CommitSelected(ITfContext* pic,
                                     const std::wstring& trailing) {
  const int sel = candidates_.Selected();
  std::wstring override_text;
  if (sel > 0 && sel < static_cast<int>(suggestions_.size())) {
    override_text = Utf8ToUtf16(suggestions_[sel]);  // a chosen dictionary word
    suggester_.RecordUsage(suggestions_[sel]);       // learn from the choice
  }
  return EndComposition(pic, trailing, override_text);
}

void CTextService::LoadDictionaries() {
  const std::wstring base = ModuleDir();
  const std::string d = ReadFileUtf8(base + L"dictionary.tsv");
  if (!d.empty()) suggester_.LoadDictionaryData(d);
  const std::string w = ReadFileUtf8(base + L"words.tsv");
  if (!w.empty()) suggester_.LoadWordList(w);

  const std::wstring dir = UserDataDir();
  if (!dir.empty()) {
    const std::string learned = ReadFileUtf8(dir + L"\\learned.tsv");
    if (!learned.empty()) suggester_.LoadLearnedData(learned);
  }
}

void CTextService::SaveLearned() const {
  const std::wstring dir = UserDataDir();
  if (dir.empty()) return;
  CreateDirectoryW(dir.c_str(), nullptr);
  WriteFileUtf8(dir + L"\\learned.tsv", suggester_.DumpLearnedData());
}

// ---- ITfKeyEventSink ----
STDMETHODIMP CTextService::OnSetFocus(BOOL /*fForeground*/) { return S_OK; }

bool CTextService::ShouldEat(WPARAM vk, char ch) const {
  if (!enabled_) return false;  // English mode: pass everything through
  // While the candidate window is open we also consume the navigation keys.
  if (candidates_.IsVisible()) {
    switch (vk) {
      case VK_DOWN: case VK_UP: case VK_TAB: case VK_ESCAPE:
        return true;
      default: break;
    }
  }
  if (vk == VK_BACK) return !buffer_.empty();
  // Space/Enter are consumed only while a word is composing, so they commit it
  // (and outside composition they behave normally for the app).
  if (vk == VK_SPACE || vk == VK_RETURN) return IsComposing();
  // Any character the engine treats as phonetic input (letters, digits, signs).
  return ch != 0 && bnphonetic::IsPhoneticInput(ch);
}

STDMETHODIMP CTextService::OnTestKeyDown(ITfContext* /*pic*/, WPARAM wParam,
                                         LPARAM lParam, BOOL* pfEaten) {
  if (pfEaten == nullptr) return E_INVALIDARG;
  *pfEaten = ShouldEat(wParam, TranslateChar(wParam, lParam));
  return S_OK;
}

STDMETHODIMP CTextService::OnTestKeyUp(ITfContext* /*pic*/, WPARAM /*wParam*/,
                                       LPARAM /*lParam*/, BOOL* pfEaten) {
  if (pfEaten) *pfEaten = FALSE;
  return S_OK;
}

STDMETHODIMP CTextService::OnKeyDown(ITfContext* pic, WPARAM wParam,
                                     LPARAM lParam, BOOL* pfEaten) {
  if (pfEaten) *pfEaten = FALSE;
  // Never let a bare modifier/lock key touch the composition.
  if (IsModifierKey(wParam)) return S_OK;

  // Candidate-window navigation takes priority while it is open.
  if (candidates_.IsVisible()) {
    switch (wParam) {
      case VK_DOWN:
      case VK_TAB:
        candidates_.Move(1);
        if (pfEaten) *pfEaten = TRUE;
        return S_OK;
      case VK_UP:
        candidates_.Move(-1);
        if (pfEaten) *pfEaten = TRUE;
        return S_OK;
      case VK_ESCAPE:
        candidates_.Hide();
        if (pfEaten) *pfEaten = TRUE;
        return S_OK;
      case VK_SPACE:
        CommitSelected(pic, L" ");
        if (pfEaten) *pfEaten = TRUE;
        return S_OK;
      case VK_RETURN:
        CommitSelected(pic, std::wstring());
        if (pfEaten) *pfEaten = TRUE;
        return S_OK;
      default:
        break;  // typing / backspace falls through and refreshes the list
    }
  }

  const char ch = TranslateChar(wParam, lParam);
  const bool eaten = (pic != nullptr) && ShouldEat(wParam, ch);
  if (pfEaten) *pfEaten = eaten ? TRUE : FALSE;

  if (!eaten) {
    // A key we don't handle: if a word is open, commit it so it doesn't dangle.
    if (pic && IsComposing()) EndComposition(pic);
    return S_OK;
  }

  if (wParam == VK_BACK) {
    buffer_.pop_back();
    if (buffer_.empty())
      EndComposition(pic);
    else
      UpdateComposition(pic);
  } else if (wParam == VK_SPACE) {
    // Commit the word and insert the space ourselves (we ate the key).
    EndComposition(pic, L" ");
  } else if (wParam == VK_RETURN) {
    // Commit the word; the Enter is consumed (press Enter again for a newline).
    EndComposition(pic);
  } else {
    // A phonetic input character: add it to the word and update the preview.
    buffer_.push_back(ch);
    UpdateComposition(pic);
  }
  return S_OK;
}

STDMETHODIMP CTextService::OnKeyUp(ITfContext* /*pic*/, WPARAM /*wParam*/,
                                   LPARAM /*lParam*/, BOOL* pfEaten) {
  if (pfEaten) *pfEaten = FALSE;
  return S_OK;
}

STDMETHODIMP CTextService::OnPreservedKey(ITfContext* pic, REFGUID rguid,
                                          BOOL* pfEaten) {
  if (IsEqualGUID(rguid, c_guidToggleKey)) {
    if (composition_ && pic) EndComposition(pic);
    ToggleMode();
    if (pfEaten) *pfEaten = TRUE;
    return S_OK;
  }
  if (pfEaten) *pfEaten = FALSE;
  return S_OK;
}

// ---- ITfCompositionSink ----
STDMETHODIMP CTextService::OnCompositionTerminated(
    TfEditCookie /*ecWrite*/, ITfComposition* /*pComposition*/) {
  // The application (or another service) terminated our composition.
  if (composition_) {
    composition_->Release();
    composition_ = nullptr;
  }
  buffer_.clear();
  candidates_.Hide();
  return S_OK;
}

// ---- ITfDisplayAttributeProvider ----
STDMETHODIMP CTextService::EnumDisplayAttributeInfo(
    IEnumTfDisplayAttributeInfo** ppEnum) {
  if (ppEnum == nullptr) return E_INVALIDARG;
  CEnumDisplayAttributeInfo* e = new (std::nothrow) CEnumDisplayAttributeInfo();
  if (e == nullptr) return E_OUTOFMEMORY;
  *ppEnum = e;
  return S_OK;
}

STDMETHODIMP CTextService::GetDisplayAttributeInfo(
    REFGUID guid, ITfDisplayAttributeInfo** ppInfo) {
  if (ppInfo == nullptr) return E_INVALIDARG;
  *ppInfo = nullptr;
  if (!IsEqualGUID(guid, c_guidDisplayAttribute)) return E_INVALIDARG;
  CDisplayAttributeInfo* info = new (std::nothrow) CDisplayAttributeInfo();
  if (info == nullptr) return E_OUTOFMEMORY;
  *ppInfo = info;
  return S_OK;
}

// ---- Mode, language bar, preserved key, config ----
void CTextService::SetEnabled(bool enabled) {
  if (enabled_ == enabled) return;
  enabled_ = enabled;
  if (langbar_button_) langbar_button_->NotifyUpdate();
}

void CTextService::AddLangBarButton() {
  if (thread_mgr_ == nullptr || langbar_button_ != nullptr) return;
  langbar_button_ = new (std::nothrow) CLangBarButton(this);
  if (langbar_button_ == nullptr) return;

  ITfLangBarItemMgr* mgr = nullptr;
  if (SUCCEEDED(thread_mgr_->QueryInterface(
          IID_ITfLangBarItemMgr, reinterpret_cast<void**>(&mgr)))) {
    mgr->AddItem(langbar_button_);
    mgr->Release();
  }
}

void CTextService::RemoveLangBarButton() {
  if (langbar_button_ == nullptr) return;
  if (thread_mgr_) {
    ITfLangBarItemMgr* mgr = nullptr;
    if (SUCCEEDED(thread_mgr_->QueryInterface(
            IID_ITfLangBarItemMgr, reinterpret_cast<void**>(&mgr)))) {
      mgr->RemoveItem(langbar_button_);
      mgr->Release();
    }
  }
  langbar_button_->Release();
  langbar_button_ = nullptr;
}

void CTextService::PreserveToggleKey(bool add) {
  if (thread_mgr_ == nullptr) return;
  ITfKeystrokeMgr* keystroke_mgr = nullptr;
  if (FAILED(thread_mgr_->QueryInterface(
          IID_ITfKeystrokeMgr, reinterpret_cast<void**>(&keystroke_mgr))))
    return;

  TF_PRESERVEDKEY key;
  key.uVKey = 'B';
  key.uModifiers = TF_MOD_CONTROL | TF_MOD_SHIFT;  // Ctrl+Shift+B
  if (add) {
    const WCHAR desc[] = L"Toggle Bangla/English";
    keystroke_mgr->PreserveKey(client_id_, c_guidToggleKey, &key, desc,
                               ARRAYSIZE(desc) - 1);
  } else {
    keystroke_mgr->UnpreserveKey(c_guidToggleKey, &key);
  }
  keystroke_mgr->Release();
}
