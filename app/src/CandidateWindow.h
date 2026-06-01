#pragma once

#include <string>
#include <vector>

#include "Globals.h"

// A small popup that lists Bangla word suggestions while composing. It never
// takes focus (WS_EX_NOACTIVATE) — the text service drives selection from its
// key handler and reads back the chosen item.
class CandidateWindow {
 public:
  CandidateWindow();
  ~CandidateWindow();

  bool EnsureCreated();
  void Show(const std::vector<std::wstring>& items, int screen_x, int screen_y);
  void Hide();
  bool IsVisible() const;

  int Count() const { return static_cast<int>(items_.size()); }
  int Selected() const { return selected_; }
  void SetSelected(int index);
  void Move(int delta);  // wraps around

  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

 private:
  void OnPaint();
  void Relayout();  // size the window to fit items_

  HWND hwnd_;
  HFONT font_;
  std::vector<std::wstring> items_;
  int selected_;
  int item_height_;
  int width_;
};
