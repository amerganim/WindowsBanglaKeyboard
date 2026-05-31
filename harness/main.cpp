// Standalone phonetic harness (Phase 2). Lets you exercise the transliteration
// engine without registering the input method. Two modes:
//
//   bnphonetic_harness "amar nam"      -> prints the conversion of the args
//   bnphonetic_harness                 -> interactive: type a line, see Bangla
//
// On Windows the console output code page is set to UTF-8 so Bangla renders in
// a UTF-8-capable terminal (Windows Terminal). Piped/redirected output is raw
// UTF-8 regardless.

#include <iostream>
#include <string>

#include "bnphonetic/Transliterator.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char** argv) {
#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
#endif

  if (argc > 1) {
    std::string input;
    for (int i = 1; i < argc; ++i) {
      if (i > 1) input += ' ';
      input += argv[i];
    }
    std::cout << bnphonetic::Transliterate(input) << "\n";
    return 0;
  }

  std::cout << "Bangla phonetic harness — type romanized Bangla, Enter to "
               "convert.\n(Ctrl+Z then Enter on Windows, or Ctrl+D, to quit.)\n";
  std::string line;
  while (std::getline(std::cin, line)) {
    std::cout << bnphonetic::Transliterate(line) << "\n";
  }
  return 0;
}
