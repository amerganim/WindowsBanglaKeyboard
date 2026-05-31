// Phase 1 engine tests. Compiled with /utf-8 (see root CMakeLists.txt) so the
// expected Bangla literals are UTF-8. Returns non-zero if any case fails.

#include "bnphonetic/Transliterator.h"

#include <iostream>
#include <string>

namespace {

int g_total = 0;
int g_failures = 0;

void Check(const std::string& input, const std::string& expected) {
  ++g_total;
  const std::string got = bnphonetic::Transliterate(input);
  if (got != expected) {
    ++g_failures;
    std::cerr << "FAIL: \"" << input << "\" -> \"" << got
              << "\"  (expected \"" << expected << "\")\n";
  } else {
    std::cout << "ok  : \"" << input << "\" -> \"" << got << "\"\n";
  }
}

}  // namespace

int main() {
  // Independent vowel at word start, kar after a consonant.
  Check("ami", "আমি");
  Check("amar", "আমার");
  Check("tumi", "তুমি");
  Check("kobita", "কবিতা");

  // 'ng' as the velar-nasal consonant glyph (ঙ). (Anusvara ং, as in বাংলা, and
  // disambiguating ng from n+g are reserved for the rule-set expansion; see
  // docs/ARCHITECTURE.md.)
  Check("rong", "রঙ");

  // Inherent vowel 'o' after a consonant produces no kar; following vowel is
  // independent.
  Check("boi", "বই");
  Check("onek", "অনেক");

  // Conjuncts via auto-hasanta when consonants are adjacent.
  Check("kk", "ক্ক");
  Check("sundor", "সুন্দর");

  // Multi-char consonant keys (longest-match) and aspirates.
  Check("Dhaka", "ঢাকা");
  Check("bhai", "ভাই");

  // Bangla digits.
  Check("2024", "২০২৪");

  // Unknown characters (space/punctuation) pass through and reset context.
  Check("ami tumi", "আমি তুমি");

  std::cout << "\n" << (g_total - g_failures) << "/" << g_total
            << " checks passed\n";
  return g_failures == 0 ? 0 : 1;
}
