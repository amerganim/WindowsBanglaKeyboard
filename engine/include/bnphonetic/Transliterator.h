#pragma once

#include <string>

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

// Returns true if `c` is a character the transliterator consumes as phonetic
// input (an ASCII letter, digit, or one of the sign keys such as '.', '^',
// ':'). The input method uses this to decide which keystrokes to buffer into
// the current word vs. let through to the application.
bool IsPhoneticInput(char c);

}  // namespace bnphonetic
