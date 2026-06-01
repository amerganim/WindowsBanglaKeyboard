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

  // 'ng' is anusvara ং (attaches to the preceding cluster); 'Ng' is the
  // velar-nasal letter ঙ.
  Check("rong", "রং");
  Check("bangla", "বাংলা");

  // Combining marks and the vocalic vowel ঋ.
  Check("cha^d", "চাঁদ");   // chandrabindu
  Check("rriSi", "ঋষি");    // ঋ + ষ + ি
  Check("a^", "আঁ");        // chandrabindu after independent vowel
  Check("a:", "আঃ");        // visarga
  Check(".", "।");          // dari (Bangla full stop)
  Check("Sesh.", "ষেশ।");   // word followed by dari

  // য (z), য় (y) and জ (j) are distinct.
  Check("z", "য");
  Check("noy", "নয়");
  Check("kaj", "কাজ");

  // Explicit hasanta (`) for ref/ya-phala, and khanda-ta (t`).
  Check("r`k", "র্ক");      // ref
  Check("k`z", "ক্য");      // ya-phala
  Check("t`", "ৎ");         // khanda-ta

  // Capital vowels render as kar after a consonant (engine is correct; the
  // app must not commit the word when Shift is pressed).
  Check("tU", "তূ");
  Check("tO", "তো");

  // 'w' as bo-phola via the conjunct mechanism.
  Check("swopno", "স্বপ্ন");
  Check("bishwas", "বিশ্বাস");

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
