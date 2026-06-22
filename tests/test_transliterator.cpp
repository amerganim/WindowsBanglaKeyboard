// Phase 1 engine tests. Compiled with /utf-8 (see root CMakeLists.txt) so the
// expected Bangla literals are UTF-8. Returns non-zero if any case fails.

#include "bnphonetic/Transliterator.h"
#include "bnphonetic/Suggester.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
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

void CheckSmart(const std::string& input, const std::string& expected) {
  ++g_total;
  const std::string got = bnphonetic::Transliterate(input, /*smart=*/true);
  if (got != expected) {
    ++g_failures;
    std::cerr << "FAIL(smart): \"" << input << "\" -> \"" << got
              << "\"  (expected \"" << expected << "\")\n";
  } else {
    std::cout << "ok  : smart \"" << input << "\" -> \"" << got << "\"\n";
  }
}

// --- Decode UTF-8 into Unicode code points (for the reverse-romanizer). ---
std::vector<uint32_t> DecodeUtf8(const std::string& s) {
  std::vector<uint32_t> cps;
  size_t i = 0;
  while (i < s.size()) {
    unsigned char c = static_cast<unsigned char>(s[i]);
    uint32_t cp;
    int extra;
    if (c < 0x80) { cp = c; extra = 0; }
    else if ((c >> 5) == 0x6) { cp = c & 0x1F; extra = 1; }
    else if ((c >> 4) == 0xE) { cp = c & 0x0F; extra = 2; }
    else { cp = c & 0x07; extra = 3; }
    ++i;
    for (int k = 0; k < extra && i < s.size(); ++k, ++i)
      cp = (cp << 6) | (static_cast<unsigned char>(s[i]) & 0x3F);
    cps.push_back(cp);
  }
  return cps;
}

// Reverse of the engine: Bangla -> canonical romanization that maps back. Used to
// build conjunct round-trip tests from the engine's built-in juktakkhor list.
std::string ToRoman(const std::string& bangla) {
  static const std::map<uint32_t, const char*> kCons = {
      {0x0995,"k"},{0x0996,"kh"},{0x0997,"g"},{0x0998,"gh"},{0x0999,"Ng"},
      {0x099A,"ch"},{0x099B,"chh"},{0x099C,"j"},{0x099D,"jh"},{0x099E,"NG"},
      {0x099F,"T"},{0x09A0,"Th"},{0x09A1,"D"},{0x09A2,"Dh"},{0x09A3,"N"},
      {0x09A4,"t"},{0x09A5,"th"},{0x09A6,"d"},{0x09A7,"dh"},{0x09A8,"n"},
      {0x09AA,"p"},{0x09AB,"ph"},{0x09AC,"b"},{0x09AD,"bh"},{0x09AE,"m"},
      {0x09AF,"z"},{0x09B0,"r"},{0x09B2,"l"},{0x09B6,"sh"},{0x09B7,"S"},
      {0x09B8,"s"},{0x09B9,"h"},{0x09DC,"R"},{0x09DD,"Rh"},{0x09DF,"y"}};
  static const std::map<uint32_t, const char*> kIndep = {
      {0x0985,"o"},{0x0986,"a"},{0x0987,"i"},{0x0988,"I"},{0x0989,"u"},
      {0x098A,"U"},{0x098B,"rri"},{0x098F,"e"},{0x0990,"OI"},{0x0993,"O"},{0x0994,"OU"}};
  static const std::map<uint32_t, const char*> kKar = {
      {0x09BE,"a"},{0x09BF,"i"},{0x09C0,"I"},{0x09C1,"u"},{0x09C2,"U"},
      {0x09C3,"rri"},{0x09C7,"e"},{0x09C8,"OI"},{0x09CB,"O"},{0x09CC,"OU"}};
  static const std::map<uint32_t, const char*> kSign = {
      {0x0982,"ng"},{0x0981,"^"},{0x0983,":"},{0x0964,"."}};
  const uint32_t HASANTA = 0x09CD, KHANDA_TA = 0x09CE;

  const auto cps = DecodeUtf8(bangla);
  std::string r;
  size_t i = 0;
  while (i < cps.size()) {
    const uint32_t ch = cps[i];
    auto cit = kCons.find(ch);
    if (cit != kCons.end()) {
      r += cit->second;
      const uint32_t next = (i + 1 < cps.size()) ? cps[i + 1] : 0;
      auto kit = kKar.find(next);
      if (kit != kKar.end()) { r += kit->second; i += 2; }
      else if (next == HASANTA) {
        if (i + 2 < cps.size() && cps[i + 2] == KHANDA_TA) r += "`";
        i += 2;
      } else { r += "o"; i += 1; }  // inherent vowel
      continue;
    }
    auto vit = kIndep.find(ch);
    if (vit != kIndep.end()) { r += vit->second; ++i; continue; }
    auto sit = kSign.find(ch);
    if (sit != kSign.end()) { r += sit->second; ++i; continue; }
    if (ch >= 0x09E6 && ch <= 0x09EF) { r += static_cast<char>('0' + (ch - 0x09E6)); ++i; continue; }
    if (ch == KHANDA_TA) { r += "t`"; ++i; continue; }
    if (ch < 0x80) r += static_cast<char>(ch);
    ++i;
  }
  return r;
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

  // Suggester: Bangla word-list completion by transliterated prefix.
  {
    bnphonetic::Suggester s;
    s.LoadWordList("আকাশ\t50\nআকাশগঙ্গা\t10\n# comment\nঅন্য\t5\n");
    auto v = s.Suggest("aka");  // Transliterate("aka") = আকা
    ++g_total;
    if (!Contains(v, "আকাশ") || !Contains(v, "আকাশগঙ্গা")) {
      ++g_failures;
      std::cerr << "FAIL: word-list prefix completion\n";
    } else {
      std::cout << "ok  : word-list prefix completion\n";
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

  // ---- Smart conjunct mode ----
  // Invalid clusters stay separate (each consonant keeps its inherent vowel).
  CheckSmart("zkhn", "যখন");
  CheckSmart("zokhon", "যখন");
  // Real conjuncts still form.
  CheckSmart("kSh", "ক্ষ");
  CheckSmart("kt", "ক্ত");
  CheckSmart("ntr", "ন্ত্র");
  CheckSmart("str", "স্ত্র");
  CheckSmart("kShm", "ক্ষ্ম");
  // ref and ya-/ra-phala join productively even if the exact cluster isn't listed.
  CheckSmart("rk", "র্ক");
  CheckSmart("rok", "রক");
  CheckSmart("fz", "ফ্য");
  CheckSmart("kz", "ক্য");
  CheckSmart("fr", "ফ্র");
  // Pronunciation spellings.
  CheckSmart("biggan", "বিজ্ঞান");
  CheckSmart("onchol", "অঞ্চল");
  CheckSmart("panjabi", "পাঞ্জাবি");
  // The ঙ digraph must not be mangled by the gg->জ্ঞ shortcut.
  CheckSmart("rNggo", "র্ঙ্গ");
  // Whole words unaffected.
  CheckSmart("bistarito", "বিস্তারিত");
  CheckSmart("bangla", "বাংলা");

  // Every built-in conjunct round-trips in BOTH smart and plain mode.
  {
    const auto conjuncts = bnphonetic::BuiltinConjuncts();
    ++g_total;
    if (conjuncts.size() < 200) {
      ++g_failures;
      std::cerr << "FAIL: built-in conjunct list too small ("
                << conjuncts.size() << ")\n";
    } else {
      int smart_fail = 0, plain_fail = 0;
      for (const std::string& j : conjuncts) {
        const std::string roman = ToRoman(j);
        if (bnphonetic::Transliterate(roman, true) != j) {
          if (++smart_fail <= 8)
            std::cerr << "FAIL(smart conjunct): \"" << j << "\" <- \"" << roman
                      << "\" -> \"" << bnphonetic::Transliterate(roman, true) << "\"\n";
        }
        if (bnphonetic::Transliterate(roman, false) != j) ++plain_fail;
      }
      if (smart_fail || plain_fail) {
        ++g_failures;
        std::cerr << "FAIL: conjunct round-trip smart=" << smart_fail
                  << " plain=" << plain_fail << " of " << conjuncts.size() << "\n";
      } else {
        std::cout << "ok  : all " << conjuncts.size()
                  << " conjuncts round-trip (smart + plain)\n";
      }
    }
  }

  // Forgiving (ambiguity-tolerant) suggestions: a phonetically ambiguous
  // spelling still finds the word (short i -> ঈ, s -> শ, th -> ঠ, t -> ট).
  {
    bnphonetic::Suggester s;
    s.LoadWordList("নদী\t50\nআকাশ\t40\nমাঠ\t30\nছাত্র\t20\n");
    auto v1 = s.Suggest("nodi", 9, true);   // নদি -> fuzzy নদী
    auto v2 = s.Suggest("akas", 9, true);   // আকাস -> fuzzy আকাশ
    auto v3 = s.Suggest("math", 9, true);   // মাথ  -> fuzzy মাঠ
    auto v4 = s.Suggest("chhatro", 9, true);  // exact still works
    ++g_total;
    if (Contains(v1, "নদী") && Contains(v2, "আকাশ") && Contains(v3, "মাঠ") &&
        Contains(v4, "ছাত্র")) {
      std::cout << "ok  : forgiving suggestions (i/I, s/শ, th/ঠ) + exact\n";
    } else {
      ++g_failures;
      std::cerr << "FAIL: forgiving suggestions\n";
    }
    // Exact matches still rank ahead of fuzzy ones.
    bnphonetic::Suggester s2;
    s2.LoadWordList("আকাশছোঁয়া\t90\nআকাষ\t10\n");  // exact prefix vs fuzzy
    auto v5 = s2.Suggest("akash", 9, true);  // literal আকাশ
    ++g_total;
    // exact (আকাশছোঁয়া starts with আকাশ) must come before fuzzy (আকাষ).
    auto pa = std::find(v5.begin(), v5.end(), "আকাশছোঁয়া");
    auto pf = std::find(v5.begin(), v5.end(), "আকাষ");
    if (pa != v5.end() && (pf == v5.end() || pa < pf)) {
      std::cout << "ok  : exact suggestion ranks before fuzzy\n";
    } else {
      ++g_failures;
      std::cerr << "FAIL: exact-before-fuzzy ranking\n";
    }
  }

  std::cout << "\n" << (g_total - g_failures) << "/" << g_total
            << " checks passed\n";
  return g_failures == 0 ? 0 : 1;
}
