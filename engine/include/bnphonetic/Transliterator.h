#pragma once

#include <string>
#include <vector>

namespace bnphonetic {

// Converts a phonetic Latin (romanized) string into Bangla Unicode text.
//
// Input is treated as UTF-8 (ASCII in practice for the Latin keys). Output is
// UTF-8 encoded Bangla. The transliteration is stateful within the call: it
// tracks whether the previous emitted unit was a consonant so that vowels
// render as dependent signs (kar) after consonants and as independent vowels
// otherwise, and consecutive consonants form conjuncts via hasanta (U+09CD).
//
// This function is pure and has no Windows/COM dependencies.
std::string Transliterate(const std::string& latin);

// Smart-conjunct overload. When `smart` is true, two adjacent consonants are
// joined into a conjunct (hasanta) only if the cluster is a real juktakkhor
// (ref র্ and ya-/ra-phala still join productively); otherwise each consonant
// keeps its inherent vowel and stays separate (so `zkhn` -> যখন, not য্খ্ন). It
// also applies pronunciation spellings (`gg` -> জ্ঞ, `n` before চ/ছ/জ/ঝ -> ঞ).
// Uses the built-in conjunct list unless SetConjunctList() overrides it.
std::string Transliterate(const std::string& latin, bool smart);

// Replace the conjunct list used by smart mode (UTF-8, whitespace-separated
// conjuncts; lines starting with '#' ignored). Optional — a built-in list ships
// with the engine. Thread-safe.
void SetConjunctList(const std::string& list_text);

// The built-in conjunct list as individual tokens (for tests/inspection).
std::vector<std::string> BuiltinConjuncts();

// Returns true if `c` is a character the transliterator consumes as phonetic
// input (an ASCII letter, digit, or one of the sign keys such as '.', '^',
// ':'). The input method uses this to decide which keystrokes to buffer into
// the current word vs. let through to the application.
bool IsPhoneticInput(char c);

// Word suggestions for the current romanized `prefix`, as UTF-8 Bangla strings.
// Element 0 is always Transliterate(prefix) (what the user literally typed) so
// it can always be committed; the rest are dictionary words whose romanization
// starts with `prefix`, most-frequent first, de-duplicated. Returns at most
// `max_results` entries; returns empty for an empty prefix.
std::vector<std::string> Suggest(const std::string& prefix,
                                 size_t max_results = 9);

}  // namespace bnphonetic
