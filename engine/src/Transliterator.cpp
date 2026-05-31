#include "bnphonetic/Transliterator.h"

#include <unordered_map>

// NOTE: This file contains Bangla UTF-8 string literals. It MUST be compiled
// with UTF-8 source encoding (MSVC: /utf-8, set in the root CMakeLists.txt).
//
// This is the Phase 1 rule set: a deliberately curated, self-consistent subset
// of an Avro-style phonetic scheme. It demonstrates the architecture (greedy
// longest-match tokenizer + stateful assembler) and is correct for the cases
// covered by the test suite. The rule table is expected to grow toward full
// Avro Phonetic parity in later iterations.

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
      {"ng", {Kind::Consonant, "ঙ", ""}},
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
      {"bh", {Kind::Consonant, "ভ", ""}},
      {"v", {Kind::Consonant, "ভ", ""}},
      {"m", {Kind::Consonant, "ম", ""}},
      {"z", {Kind::Consonant, "জ", ""}},
      {"r", {Kind::Consonant, "র", ""}},
      {"l", {Kind::Consonant, "ল", ""}},
      {"sh", {Kind::Consonant, "শ", ""}},
      {"S", {Kind::Consonant, "ষ", ""}},
      {"Sh", {Kind::Consonant, "ষ", ""}},
      {"s", {Kind::Consonant, "স", ""}},
      {"h", {Kind::Consonant, "হ", ""}},
      {"R", {Kind::Consonant, "ড়", ""}},
      {"Rh", {Kind::Consonant, "ঢ়", ""}},
      {"y", {Kind::Consonant, "য়", ""}},
      {"Y", {Kind::Consonant, "য", ""}},

      // ---- Vowels: {independent, dependent-sign} ----
      {"o", {Kind::Vowel, "অ", ""}},  // inherent vowel: no kar after consonant
      {"a", {Kind::Vowel, "আ", "া"}},
      {"i", {Kind::Vowel, "ই", "ি"}},
      {"I", {Kind::Vowel, "ঈ", "ী"}},
      {"u", {Kind::Vowel, "উ", "ু"}},
      {"U", {Kind::Vowel, "ঊ", "ূ"}},
      {"e", {Kind::Vowel, "এ", "ে"}},
      {"O", {Kind::Vowel, "ও", "ো"}},
      {"OI", {Kind::Vowel, "ঐ", "ৈ"}},
      {"OU", {Kind::Vowel, "ঔ", "ৌ"}},

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

}  // namespace

std::string Transliterate(const std::string& latin) {
  const auto& table = Table();
  std::string out;
  bool prev_consonant = false;
  const size_t n = latin.size();

  for (size_t i = 0; i < n;) {
    const Unit* unit = nullptr;
    int matched_len = 0;

    for (int len = kMaxKeyLen; len >= 1; --len) {
      if (i + static_cast<size_t>(len) > n) continue;
      auto it = table.find(latin.substr(i, len));
      if (it != table.end()) {
        unit = &it->second;
        matched_len = len;
        break;
      }
    }

    if (unit == nullptr) {
      // Unknown character (space, punctuation, untranslated letter): pass it
      // through verbatim and reset consonant context.
      out.push_back(latin[i]);
      prev_consonant = false;
      ++i;
      continue;
    }

    switch (unit->kind) {
      case Kind::Consonant:
        if (prev_consonant) out += kHasanta;  // form a conjunct
        out += unit->main;
        prev_consonant = true;
        break;
      case Kind::Vowel:
        // After a consonant a vowel becomes a dependent sign (empty for the
        // inherent vowel); otherwise it is an independent vowel.
        out += prev_consonant ? unit->kar : unit->main;
        prev_consonant = false;
        break;
      case Kind::Direct:
        out += unit->main;
        prev_consonant = false;
        break;
    }

    i += static_cast<size_t>(matched_len);
  }

  return out;
}

}  // namespace bnphonetic
