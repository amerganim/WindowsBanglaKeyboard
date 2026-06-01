// Phase 1 engine tests. Compiled with /utf-8 (see root CMakeLists.txt) so the
// expected Bangla literals are UTF-8. Returns non-zero if any case fails.

#include "bnphonetic/Transliterator.h"
#include "bnphonetic/Suggester.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

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

// Suggestion checks.
bool Contains(const std::vector<std::string>& v, const std::string& s) {
  return std::find(v.begin(), v.end(), s) != v.end();
}

void CheckFirst(const std::string& prefix, const std::string& expected) {
  ++g_total;
  auto v = bnphonetic::Suggest(prefix);
  if (v.empty() || v[0] != expected) {
    ++g_failures;
    std::cerr << "FAIL: Suggest(\"" << prefix << "\")[0] = \""
              << (v.empty() ? std::string("<none>") : v[0])
              << "\"  (expected \"" << expected << "\")\n";
  } else {
    std::cout << "ok  : Suggest(\"" << prefix << "\")[0] -> \"" << v[0] << "\"\n";
  }
}

void CheckSuggests(const std::string& prefix, const std::string& expected) {
  ++g_total;
  auto v = bnphonetic::Suggest(prefix);
  if (!Contains(v, expected)) {
    ++g_failures;
    std::cerr << "FAIL: Suggest(\"" << prefix << "\") does not contain \""
              << expected << "\"\n";
  } else {
    std::cout << "ok  : Suggest(\"" << prefix << "\") contains \"" << expected
              << "\"\n";
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

  // Suggestions: element 0 is always the literal transliteration; dictionary
  // words sharing the prefix follow.
  CheckFirst("ami", "আমি");
  CheckSuggests("ama", "আমার");
  CheckSuggests("ama", "আমাকে");
  CheckSuggests("bhal", "ভালো");
  CheckSuggests("bhal", "ভালোবাসা");
  CheckSuggests("dhonnobad", "ধন্যবাদ");
  if (!bnphonetic::Suggest("").empty()) {
    ++g_failures;
    std::cerr << "FAIL: Suggest(\"\") should be empty\n";
  }
  ++g_total;

  // Suggester: loading a dictionary entry makes it suggestible.
  {
    bnphonetic::Suggester s;
    s.LoadDictionaryData("notunshobdo\tনতুনশব্দ\t99\n# comment line\n");
    ++g_total;
    auto v = s.Suggest("notunsh");
    if (!Contains(v, "নতুনশব্দ")) {
      ++g_failures;
      std::cerr << "FAIL: loaded dictionary word not suggested\n";
    } else {
      std::cout << "ok  : loaded dictionary word suggested\n";
    }
  }

  // Suggester: learned usage promotes a word above a more-frequent default.
  {
    bnphonetic::Suggester s;
    for (int i = 0; i < 5; ++i) s.RecordUsage("আমাকে");
    auto v = s.Suggest("ama");
    ++g_total;
    // v[0] is the literal; v[1] should now be the learned word.
    if (v.size() < 2 || v[1] != "আমাকে") {
      ++g_failures;
      std::cerr << "FAIL: learned word not promoted (got "
                << (v.size() < 2 ? "<none>" : v[1]) << ")\n";
    } else {
      std::cout << "ok  : learned word promoted to top suggestion\n";
    }
    // Learned data round-trips through dump/load.
    bnphonetic::Suggester s2;
    s2.LoadLearnedData(s.DumpLearnedData());
    auto v2 = s2.Suggest("ama");
    ++g_total;
    if (v2.size() < 2 || v2[1] != "আমাকে") {
      ++g_failures;
      std::cerr << "FAIL: learned data did not round-trip\n";
    } else {
      std::cout << "ok  : learned data round-trips\n";
    }
  }

  std::cout << "\n" << (g_total - g_failures) << "/" << g_total
            << " checks passed\n";
  return g_failures == 0 ? 0 : 1;
}
