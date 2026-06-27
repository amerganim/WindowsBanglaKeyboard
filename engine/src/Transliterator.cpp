#include "bnphonetic/Transliterator.h"

#include <memory>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

// NOTE: This file contains Bangla UTF-8 string literals. It MUST be compiled
// with UTF-8 source encoding (MSVC: /utf-8, set in the root CMakeLists.txt).
//
// This is the phonetic rule set: a curated, self-consistent subset of an
// Avro-style scheme. It demonstrates the architecture (greedy longest-match
// tokenizer + stateful assembler) and is correct for the cases covered by the
// test suite, including consonants/conjuncts, dependent/independent vowels,
// anusvara (ng → ং) vs. the velar-nasal letter (Ng → ঙ), chandrabindu (^),
// visarga (:) and Bangla digits. The table grows toward full Avro Phonetic
// parity in later iterations (e.g. ya-phala/ref handling, ৎ, more diphthongs).

namespace bnphonetic {
namespace {

enum class Kind {
  Consonant,  // carries the inherent vowel; `main` is the base glyph.
  Vowel,      // `main` = independent form, `kar` = dependent sign.
  Direct,     // emitted verbatim regardless of context (digits, etc.).
};

struct Unit {
  Kind kind;
  std::string main;
  std::string kar;  // only meaningful for Vowel; "" means inherent vowel.
};

// The hasanta / virama that joins consonants into a conjunct (juktakkhor).
const char* const kHasanta = "্";  // ্

const std::unordered_map<std::string, Unit>& Table() {
  static const std::unordered_map<std::string, Unit> t = {
      // ---- Consonants (longest keys must exist for greedy matching) ----
      {"k", {Kind::Consonant, "ক", ""}},
      {"kh", {Kind::Consonant, "খ", ""}},
      {"g", {Kind::Consonant, "গ", ""}},
      {"gh", {Kind::Consonant, "ঘ", ""}},
      {"Ng", {Kind::Consonant, "ঙ", ""}},  // velar-nasal letter ঙ
      {"NG", {Kind::Consonant, "ঞ", ""}},  // letter ঞ (ny)
      {"c", {Kind::Consonant, "চ", ""}},
      {"ch", {Kind::Consonant, "চ", ""}},
      {"chh", {Kind::Consonant, "ছ", ""}},
      {"j", {Kind::Consonant, "জ", ""}},
      {"jh", {Kind::Consonant, "ঝ", ""}},
      {"T", {Kind::Consonant, "ট", ""}},
      {"Th", {Kind::Consonant, "ঠ", ""}},
      {"D", {Kind::Consonant, "ড", ""}},
      {"Dh", {Kind::Consonant, "ঢ", ""}},
      {"N", {Kind::Consonant, "ণ", ""}},
      {"t", {Kind::Consonant, "ত", ""}},
      {"th", {Kind::Consonant, "থ", ""}},
      {"d", {Kind::Consonant, "দ", ""}},
      {"dh", {Kind::Consonant, "ধ", ""}},
      {"n", {Kind::Consonant, "ন", ""}},
      {"p", {Kind::Consonant, "প", ""}},
      {"ph", {Kind::Consonant, "ফ", ""}},
      {"f", {Kind::Consonant, "ফ", ""}},
      {"b", {Kind::Consonant, "ব", ""}},
      {"w", {Kind::Consonant, "ব", ""}},  // bo-phola use: `swopno` -> স্বপ্ন
      {"bh", {Kind::Consonant, "ভ", ""}},
      {"v", {Kind::Consonant, "ভ", ""}},
      {"m", {Kind::Consonant, "ম", ""}},
      {"z", {Kind::Consonant, "য", ""}},  // antastha ya (য); জ is `j`
      {"r", {Kind::Consonant, "র", ""}},
      {"l", {Kind::Consonant, "ল", ""}},
      {"sh", {Kind::Consonant, "শ", ""}},
      {"S", {Kind::Consonant, "ষ", ""}},
      {"Sh", {Kind::Consonant, "ষ", ""}},
      {"s", {Kind::Consonant, "স", ""}},
      {"h", {Kind::Consonant, "হ", ""}},
      {"R", {Kind::Consonant, "ড়", ""}},
      {"Rh", {Kind::Consonant, "ঢ়", ""}},
      {"y", {Kind::Consonant, "য়", ""}},  // ya with nukta (য়)
      {"Y", {Kind::Consonant, "য়", ""}},  // alias of `y` -> য়

      // ---- Vowels: {independent, dependent-sign} ----
      {"o", {Kind::Vowel, "অ", ""}},  // inherent vowel: no kar after consonant
      {"a", {Kind::Vowel, "আ", "া"}},
      {"rri", {Kind::Vowel, "ঋ", "ৃ"}},
      {"i", {Kind::Vowel, "ই", "ি"}},
      {"I", {Kind::Vowel, "ঈ", "ী"}},
      {"u", {Kind::Vowel, "উ", "ু"}},
      {"U", {Kind::Vowel, "ঊ", "ূ"}},
      {"e", {Kind::Vowel, "এ", "ে"}},
      {"O", {Kind::Vowel, "ও", "ো"}},
      {"OI", {Kind::Vowel, "ঐ", "ৈ"}},
      {"OU", {Kind::Vowel, "ঔ", "ৌ"}},

      // ---- Combining marks (attach to the preceding cluster; emitted
      // verbatim and do not start a consonant context) ----
      {"ng", {Kind::Direct, "ং", ""}},   // anusvara (e.g. বাংলা)
      {"^", {Kind::Direct, "ঁ", ""}},   // chandrabindu (e.g. চাঁদ)
      {":", {Kind::Direct, "ঃ", ""}},   // visarga
      {".", {Kind::Direct, "।", ""}},   // dari (Bangla full stop)
      {"`", {Kind::Direct, "্", ""}},   // explicit hasanta (for ref/ya-phala)
      {"t`", {Kind::Direct, "ৎ", ""}},  // khanda-ta (U+09CE)

      // ---- Digits ----
      {"0", {Kind::Direct, "০", ""}},
      {"1", {Kind::Direct, "১", ""}},
      {"2", {Kind::Direct, "২", ""}},
      {"3", {Kind::Direct, "৩", ""}},
      {"4", {Kind::Direct, "৪", ""}},
      {"5", {Kind::Direct, "৫", ""}},
      {"6", {Kind::Direct, "৬", ""}},
      {"7", {Kind::Direct, "৭", ""}},
      {"8", {Kind::Direct, "৮", ""}},
      {"9", {Kind::Direct, "৯", ""}},
  };
  return t;
}

constexpr int kMaxKeyLen = 3;  // longest key currently is "chh".

// ---- Smart-conjunct support ----

const char* const kRa = "র";
const char* const kYa = "য";

// Built-in juktakkhor (conjunct) list (source: bn.wikibooks বাংলা যুক্তাক্ষর).
// Smart mode only joins consonants that begin one of these.
const char* const kBuiltinConjuncts = R"(
ক্ক ক্ট ক্ট্র ক্ত ক্ত্র ক্ন ক্ব ক্ম ক্য ক্র ক্ল ক্ষ ক্ষ্ণ ক্ষ্ব ক্ষ্ম ক্ষ্ম্য ক্ষ্য ক্স
খ্য খ্র
গ্ণ গ্ধ গ্ধ্য গ্ধ্র গ্ন গ্ন্য গ্ব গ্ম গ্য গ্র গ্র্য গ্ল
ঘ্ন ঘ্য ঘ্র
ঙ্ক ঙ্ক্ত ঙ্ক্য ঙ্ক্ষ ঙ্খ ঙ্খ্য ঙ্গ ঙ্গ্য ঙ্ঘ ঙ্ঘ্য ঙ্ঘ্র ঙ্ম
চ্চ চ্ছ চ্ছ্ব চ্ছ্র চ্ঞ চ্ব চ্য
জ্জ জ্জ্ব জ্ঝ জ্ঞ জ্ব জ্য জ্র
ঞ্চ ঞ্ছ ঞ্জ ঞ্ঝ
ট্ট ট্ব ট্ম ট্য ট্র
ড্ড ড্ব ড্ম ড্য ড্র
ঢ্য ঢ্র
ণ্ট ণ্ঠ ণ্ঠ্য ণ্ড ণ্ড্য ণ্ড্র ণ্ঢ ণ্ণ ণ্ব ণ্ম ণ্য
ত্ত ত্ত্র ত্ত্ব ত্ত্য ত্থ ত্ন ত্ব ত্ম ত্ম্য ত্য ত্র ত্র্য
থ্ব থ্য থ্র
দ্গ দ্ঘ দ্দ দ্দ্ব দ্ধ দ্ব দ্ভ দ্ভ্র দ্ম দ্য দ্র দ্র্য
ধ্ন ধ্ব ধ্ম ধ্য ধ্র
ন্ট ন্ট্র ন্ঠ ন্ড ন্ড্র ন্ত ন্ত্ব ন্ত্য ন্ত্র ন্ত্র্য ন্থ ন্থ্র ন্দ ন্দ্য ন্দ্ব ন্দ্র ন্ধ ন্ধ্য ন্ধ্র ন্ন ন্ব ন্ম ন্য
প্ট প্ত প্ন প্প প্য প্র প্র্য প্ল প্স
ফ্র ফ্ল
ব্জ ব্দ ব্ধ ব্ব ব্য ব্র ব্ল
ভ্ব ভ্য ভ্র ভ্ল
ম্ন ম্প ম্প্র ম্ফ ম্ব ম্ব্র ম্ভ ম্ভ্র ম্ম ম্য ম্র ম্ল
য্য
র্ক র্ক্য র্গ র্গ্য র্ঘ্য র্ঙ্গ র্চ্য র্জ্য র্জ্জ র্জ্ঞ র্ণ্য র্ত্য র্থ্য র্ব্য র্ম্য র্শ্য র্ষ্য র্হ্য র্খ র্গ্র র্ঘ র্চ র্ছ র্জ র্ঝ র্ট র্ড র্ণ র্ত র্ত্ম র্ত্র র্থ র্দ র্দ্ব র্দ্র র্ধ র্ধ্ব র্ন র্প র্ফ র্ব র্ভ র্ম র্য র্ল র্শ র্শ্ব র্ষ র্ষ্ট র্ষ্ণ র্ষ্ণ্য র্স র্হ র্ঢ্য
ল্ক ল্ক্য ল্গ ল্ট ল্ড ল্প ল্ফ ল্ব ল্ভ ল্ম ল্য ল্ল
শ্চ শ্ছ শ্ন শ্ব শ্ম শ্য শ্র শ্ল
ষ্ক ষ্ক্ব ষ্ক্র ষ্ট ষ্ট্য ষ্ট্র ষ্ঠ ষ্ঠ্য ষ্ণ ষ্ণ্ব ষ্প ষ্প্র ষ্ফ ষ্ব ষ্ম ষ্য
স্ক স্ক্র স্খ স্ট স্ট্র স্ত স্ত্ব স্ত্য স্ত্র স্থ স্থ্য স্ন স্ন্য স্প স্প্র স্প্ল স্ফ স্ব স্ম স্য স্র স্ল
হ্ণ হ্ন হ্ব হ্ম হ্য হ্র হ্ল
)";

std::string JoinGlyphs(const std::vector<std::string>& v, const std::string& sep) {
  std::string r;
  for (size_t i = 0; i < v.size(); ++i) {
    if (i) r += sep;
    r += v[i];
  }
  return r;
}

std::vector<std::string> SplitOnHasanta(const std::string& s) {
  std::vector<std::string> parts;
  const std::string h = kHasanta;
  size_t start = 0, idx;
  while ((idx = s.find(h, start)) != std::string::npos) {
    parts.push_back(s.substr(start, idx - start));
    start = idx + h.size();
  }
  parts.push_back(s.substr(start));
  return parts;
}

// Whitespace-separated tokens from a conjunct list, skipping '#' comment lines.
std::vector<std::string> TokenizeConjunctList(const std::string& text) {
  std::vector<std::string> tokens;
  std::istringstream in(text);
  std::string line;
  while (std::getline(in, line)) {
    size_t a = line.find_first_not_of(" \t\r\n");
    if (a == std::string::npos || line[a] == '#') continue;
    std::istringstream ls(line);
    std::string tok;
    while (ls >> tok) tokens.push_back(tok);
  }
  return tokens;
}

// Build the set of valid cluster prefixes (consonant-glyph sequences, no
// hasanta) from a conjunct list — e.g. ক্ষ্ম contributes "কষ" and "কষম".
std::unordered_set<std::string> BuildPrefixes(const std::string& text) {
  std::unordered_set<std::string> consonants;
  for (const auto& kv : Table())
    if (kv.second.kind == Kind::Consonant) consonants.insert(kv.second.main);

  std::unordered_set<std::string> prefixes;
  for (const std::string& tok : TokenizeConjunctList(text)) {
    const auto parts = SplitOnHasanta(tok);
    if (parts.size() < 2) continue;
    bool pure = true;
    for (const auto& p : parts)
      if (consonants.find(p) == consonants.end()) { pure = false; break; }
    if (!pure) continue;  // skip ৎ-forms etc.
    std::string acc;
    for (size_t k = 0; k < parts.size(); ++k) {
      acc += parts[k];
      if (k >= 1) prefixes.insert(acc);
    }
  }
  return prefixes;
}

std::mutex& ConjMutex() {
  static std::mutex m;
  return m;
}
std::shared_ptr<const std::unordered_set<std::string>>& ClusterRef() {
  static std::shared_ptr<const std::unordered_set<std::string>> p;
  return p;
}
std::shared_ptr<const std::unordered_set<std::string>> Clusters() {
  std::lock_guard<std::mutex> lk(ConjMutex());
  if (!ClusterRef())
    ClusterRef() = std::make_shared<const std::unordered_set<std::string>>(
        BuildPrefixes(kBuiltinConjuncts));
  return ClusterRef();
}

// Pronunciation spellings (smart mode): `gg` -> জ্ঞ (jNG), but NOT when the first
// g belongs to the ঙ digraph "Ng"; and `n` before চ/ছ/জ/ঝ -> ঞ (NG).
std::string ApplyPhoneticSpellings(const std::string& s) {
  std::string a;
  a.reserve(s.size());
  for (size_t i = 0; i < s.size();) {
    if (i + 1 < s.size() && s[i] == 'g' && s[i + 1] == 'g' &&
        !(i > 0 && s[i - 1] == 'N')) {
      a += "jNG";
      i += 2;
    } else {
      a += s[i++];
    }
  }
  std::string b;
  b.reserve(a.size());
  for (size_t i = 0; i < a.size();) {
    if (a[i] == 'n' && i + 1 <= a.size()) {
      if (a.compare(i + 1, 3, "chh") == 0) { b += "NGchh"; i += 4; continue; }
      if (a.compare(i + 1, 2, "ch") == 0)  { b += "NGch";  i += 3; continue; }
      if (a.compare(i + 1, 2, "jh") == 0)  { b += "NGjh";  i += 3; continue; }
      if (a.compare(i + 1, 1, "j") == 0)   { b += "NGj";   i += 2; continue; }
    }
    b += a[i++];
  }
  return b;
}

struct Seg {
  const Unit* unit;
  std::string text;  // passthrough text when unit == nullptr
};

std::vector<Seg> Tokenize(const std::string& latin) {
  std::vector<Seg> segs;
  const auto& table = Table();
  const size_t n = latin.size();

  // Greedy longest match at i; when `lower` is set the candidate is lowercased
  // before lookup (the uppercase-fallback pass).
  auto match_at = [&](size_t i, bool lower) -> std::pair<const Unit*, int> {
    for (int len = kMaxKeyLen; len >= 1; --len) {
      if (i + static_cast<size_t>(len) > n) continue;
      std::string key = latin.substr(i, len);
      if (lower)
        for (char& ch : key)
          if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
      auto it = table.find(key);
      if (it != table.end()) return {&it->second, len};
    }
    return {nullptr, 0};
  };

  for (size_t i = 0; i < n;) {
    auto m = match_at(i, /*lower=*/false);
    // An uppercase letter with no scheme meaning falls back to its lowercase
    // key (so `C`->চ), and otherwise passes through as lowercase — never as a
    // leaked uppercase English letter.
    if (m.first == nullptr && latin[i] >= 'A' && latin[i] <= 'Z')
      m = match_at(i, /*lower=*/true);

    if (m.first == nullptr) {
      char c = latin[i];
      if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
      segs.push_back({nullptr, std::string(1, c)});
      ++i;
    } else {
      segs.push_back({m.first, std::string()});
      i += static_cast<size_t>(m.second);
    }
  }
  return segs;
}

std::string Assemble(const std::vector<Seg>& segs, bool use_smart,
                     const std::unordered_set<std::string>* clusters) {
  std::string out;
  bool prev_consonant = false;      // plain-mode state
  std::vector<std::string> pending; // current consonant cluster (smart mode)

  auto flush = [&]() {
    if (!pending.empty()) {
      out += JoinGlyphs(pending, kHasanta);
      pending.clear();
    }
  };

  for (const Seg& seg : segs) {
    const Unit* unit = seg.unit;
    if (unit == nullptr) {
      if (use_smart) flush();
      out += seg.text;
      prev_consonant = false;
      continue;
    }
    switch (unit->kind) {
      case Kind::Consonant:
        if (use_smart) {
          if (!pending.empty()) {
            const std::string candidate = JoinGlyphs(pending, "") + unit->main;
            const bool is_ref = pending.size() == 1 && pending[0] == kRa;
            const bool is_phala = unit->main == kYa || unit->main == kRa;
            if (!is_ref && !is_phala &&
                clusters->find(candidate) == clusters->end())
              flush();
          }
          pending.push_back(unit->main);
        } else {
          if (prev_consonant) out += kHasanta;
          out += unit->main;
          prev_consonant = true;
        }
        break;
      case Kind::Vowel:
        if (use_smart) {
          if (!pending.empty()) {
            flush();
            out += unit->kar;  // kar attaches to the cluster
          } else {
            out += unit->main;  // independent vowel
          }
        } else {
          out += prev_consonant ? unit->kar : unit->main;
          prev_consonant = false;
        }
        break;
      case Kind::Direct:
        if (use_smart) flush();
        out += unit->main;
        prev_consonant = false;
        break;
    }
  }
  if (use_smart) flush();
  return out;
}

}  // namespace

std::string Transliterate(const std::string& latin, bool smart) {
  std::shared_ptr<const std::unordered_set<std::string>> clusters;
  bool use_smart = smart;
  if (use_smart) {
    clusters = Clusters();
    use_smart = clusters && !clusters->empty();
  }
  const std::string source = use_smart ? ApplyPhoneticSpellings(latin) : latin;
  return Assemble(Tokenize(source), use_smart,
                  use_smart ? clusters.get() : nullptr);
}

std::string Transliterate(const std::string& latin) {
  return Transliterate(latin, false);
}

void SetConjunctList(const std::string& list_text) {
  auto s = std::make_shared<const std::unordered_set<std::string>>(
      BuildPrefixes(list_text));
  std::lock_guard<std::mutex> lk(ConjMutex());
  ClusterRef() = s;
}

std::vector<std::string> BuiltinConjuncts() {
  return TokenizeConjunctList(kBuiltinConjuncts);
}

bool IsPhoneticInput(char c) {
  if (c >= 'a' && c <= 'z') return true;
  if (c >= 'A' && c <= 'Z') return true;
  if (c >= '0' && c <= '9') return true;
  return c == '^' || c == ':' || c == '.' || c == '`';
}

}  // namespace bnphonetic
