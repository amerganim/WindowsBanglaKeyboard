#include "CandidateWindow.h"

namespace {
const WCHAR kClassName[] = L"BanglaPhoneticCandidateWnd";
const int kPadX = 10;
const int kPadY = 5;
bool g_class_registered = false;

void EnsureClass() {
  if (g_class_registered) return;
  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(wc);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = CandidateWindow::WndProc;
  wc.hInstance = g_hInst;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  wc.lpszClassName = kClassName;
  RegisterClassExW(&wc);
  g_class_registered = true;
}
}  // namespace

CandidateWindow::CandidateWindow()
    : hwnd_(nullptr), font_(nullptr), selected_(0), item_height_(26),
      width_(160) {}

CandidateWindow::~CandidateWindow() {
  if (hwnd_) DestroyWindow(hwnd_);
  if (font_) DeleteObject(font_);
}

bool CandidateWindow::EnsureCreated() {
  if (hwnd_) return true;
  EnsureClass();

  // Use the system Bengali UI font so suggestions render correctly.
  font_ = CreateFontW(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                      CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Nirmala UI");

  hwnd_ = CreateWindowExW(
      WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST, kClassName, L"",
      WS_POPUP | WS_BORDER, 0, 0, width_, item_height_, nullptr, nullptr,
      g_hInst, this);
  return hwnd_ != nullptr;
}

void CandidateWindow::Relayout() {
  HDC dc = GetDC(hwnd_);
  HFONT old = static_cast<HFONT>(SelectObject(dc, font_));
  TEXTMETRICW tm;
  GetTextMetricsW(dc, &tm);
  item_height_ = tm.tmHeight + 2 * kPadY;

  int max_w = 80;
  int n = 1;
  for (const std::wstring& item : items_) {
    std::wstring line = std::to_wstring(n++) + L". " + item;
    SIZE sz;
    if (GetTextExtentPoint32W(dc, line.c_str(),
                              static_cast<int>(line.size()), &sz)) {
      if (sz.cx > max_w) max_w = sz.cx;
    }
  }
  SelectObject(dc, old);
  ReleaseDC(hwnd_, dc);

  width_ = max_w + 2 * kPadX;
  int height = item_height_ * static_cast<int>(items_.size()) + 2;
  SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, width_, height,
               SWP_NOMOVE | SWP_NOACTIVATE);
}

void CandidateWindow::Show(const std::vector<std::wstring>& items, int x, int y) {
  if (!EnsureCreated() || items.empty()) {
    Hide();
    return;
  }
  items_ = items;
  if (selected_ >= Count()) selected_ = 0;
  Relayout();

  // Keep the window on screen (flip above the caret if it would go off-bottom).
  RECT wr;
  GetWindowRect(hwnd_, &wr);
  int h = wr.bottom - wr.top;
  HMONITOR mon = MonitorFromPoint(POINT{x, y}, MONITOR_DEFAULTTONEAREST);
  MONITORINFO mi = {sizeof(mi)};
  GetMonitorInfo(mon, &mi);
  if (y + h > mi.rcWork.bottom) y -= (h + 20);
  if (x + width_ > mi.rcWork.right) x = mi.rcWork.right - width_;
  if (x < mi.rcWork.left) x = mi.rcWork.left;

  SetWindowPos(hwnd_, HWND_TOPMOST, x, y, 0, 0,
               SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
  InvalidateRect(hwnd_, nullptr, TRUE);
}

void CandidateWindow::Hide() {
  if (hwnd_) ShowWindow(hwnd_, SW_HIDE);
  items_.clear();
  selected_ = 0;
}

bool CandidateWindow::IsVisible() const {
  return hwnd_ && IsWindowVisible(hwnd_);
}

void CandidateWindow::SetSelected(int index) {
  if (Count() == 0) return;
  if (index < 0) index = 0;
  if (index >= Count()) index = Count() - 1;
  selected_ = index;
  if (hwnd_) InvalidateRect(hwnd_, nullptr, TRUE);
}

void CandidateWindow::Move(int delta) {
  if (Count() == 0) return;
  selected_ = (selected_ + delta % Count() + Count()) % Count();
  if (hwnd_) InvalidateRect(hwnd_, nullptr, TRUE);
}

void CandidateWindow::OnPaint() {
  PAINTSTRUCT ps;
  HDC dc = BeginPaint(hwnd_, &ps);
  RECT rc;
  GetClientRect(hwnd_, &rc);
  FillRect(dc, &rc, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
  HFONT old = static_cast<HFONT>(SelectObject(dc, font_));
  SetBkMode(dc, TRANSPARENT);

  for (int i = 0; i < Count(); ++i) {
    RECT row{0, i * item_height_, width_, (i + 1) * item_height_};
    const bool sel = (i == selected_);
    if (sel) {
      FillRect(dc, &row, reinterpret_cast<HBRUSH>(COLOR_HIGHLIGHT + 1));
      SetTextColor(dc, GetSysColor(COLOR_HIGHLIGHTTEXT));
    } else {
      SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
    }
    RECT textrc = row;
    textrc.left += kPadX;
    std::wstring line = std::to_wstring(i + 1) + L". " + items_[i];
    DrawTextW(dc, line.c_str(), -1, &textrc,
              DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX);
  }
  SelectObject(dc, old);
  EndPaint(hwnd_, &ps);
}

LRESULT CALLBACK CandidateWindow::WndProc(HWND hwnd, UINT msg, WPARAM wp,
                                          LPARAM lp) {
  if (msg == WM_NCCREATE) {
    auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                      reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    return DefWindowProcW(hwnd, msg, wp, lp);
  }
  auto* self = reinterpret_cast<CandidateWindow*>(
      GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  if (self == nullptr) return DefWindowProcW(hwnd, msg, wp, lp);

  switch (msg) {
    case WM_PAINT:
      self->OnPaint();
      return 0;
    case WM_MOUSEACTIVATE:
      return MA_NOACTIVATE;  // never steal focus
    case WM_DESTROY:
      self->hwnd_ = nullptr;
      return 0;
    default:
      return DefWindowProcW(hwnd, msg, wp, lp);
  }
}
