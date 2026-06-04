#include "bnphonetic/Suggester.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>

#include "bnphonetic/Transliterator.h"

// Word suggestions. The small built-in list below is the always-available
// fallback; a larger bundled dictionary.tsv is loaded on top of it at runtime.
// Compiled with /utf-8 (Bangla literals).

namespace bnphonetic {
namespace {

struct BuiltinEntry {
  const char* key;
  const char* bangla;
  int freq;
};

const BuiltinEntry kBuiltin[] = {
    {"ami", "আমি", 100},        {"amar", "আমার", 95},
    {"amake", "আমাকে", 70},     {"amader", "আমাদের", 70},
    {"tumi", "তুমি", 95},       {"tomar", "তোমার", 85},
    {"tomake", "তোমাকে", 65},   {"apni", "আপনি", 80},
    {"apnar", "আপনার", 70},     {"se", "সে", 85},
    {"tar", "তার", 80},         {"tara", "তারা", 70},
    {"eta", "এটা", 70},         {"ei", "এই", 80},
    {"ki", "কি", 90},           {"kothay", "কোথায়", 70},
    {"keno", "কেন", 75},        {"kemon", "কেমন", 75},
    {"ke", "কে", 70},           {"koto", "কত", 65},
    {"bhalo", "ভালো", 90},      {"bhalobasa", "ভালোবাসা", 70},
    {"achhi", "আছি", 70},       {"achhe", "আছে", 75},
    {"kora", "করা", 65},        {"hobe", "হবে", 70},
    {"dhonnobad", "ধন্যবাদ", 80}, {"aj", "আজ", 65},
    {"ekhon", "এখন", 65},       {"bangla", "বাংলা", 85},
    {"bangladesh", "বাংলাদেশ", 75}, {"manush", "মানুষ", 65},
    {"bondhu", "বন্ধু", 65},    {"baba", "বাবা", 65},
    {"ma", "মা", 70},           {"bhai", "ভাই", 65},
    {"somoy", "সময়", 65},       {"kaj", "কাজ", 65},
    {"taka", "টাকা", 60},       {"nam", "নাম", 60},
    {"kotha", "কথা", 65},       {"jibon", "জীবন", 60},
    {"sundor", "সুন্দর", 70},   {"onek", "অনেক", 70},
    {"sob", "সব", 65},          {"kichu", "কিছু", 65},
};

char Lower(char c) {
  return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

bool KeyHasPrefix(const std::string& key, const std::string& lower_prefix) {
  if (key.size() < lower_prefix.size()) return false;
  for (size_t i = 0; i < lower_prefix.size(); ++i) {
    if (Lower(key[i]) != lower_prefix[i]) return false;
  }
  return true;
}

std::string Trim(const std::string& s) {
  size_t a = s.find_first_not_of(" \t\r\n");
  if (a == std::string::npos) return std::string();
  size_t b = s.find_last_not_of(" \t\r\n");
  return s.substr(a, b - a + 1);
}

// Each committed use is worth this much static-frequency weight.
const int kLearnWeight = 40;

}  // namespace

Suggester::Suggester() {
  entries_.reserve(sizeof(kBuiltin) / sizeof(kBuiltin[0]));
  for (const BuiltinEntry& e : kBuiltin)
    entries_.push_back({e.key, e.bangla, e.freq});
}

void Suggester::LoadDictionaryData(const std::string& tsv) {
  std::istringstream in(tsv);
  std::string line;
  while (std::getline(in, line)) {
    std::string t = Trim(line);
    if (t.empty() || t[0] == '#') continue;
    size_t p1 = t.find('\t');
    if (p1 == std::string::npos) continue;
    size_t p2 = t.find('\t', p1 + 1);
    std::string key = Trim(t.substr(0, p1));
    std::string bangla = Trim(p2 == std::string::npos ? t.substr(p1 + 1)
                                                      : t.substr(p1 + 1, p2 - p1 - 1));
    int freq = 30;
    if (p2 != std::string::npos) {
      std::string f = Trim(t.substr(p2 + 1));
      if (!f.empty()) freq = std::atoi(f.c_str());
    }
    if (!key.empty() && !bangla.empty()) entries_.push_back({key, bangla, freq});
  }
}

void Suggester::LoadWordList(const std::string& tsv) {
  std::istringstream in(tsv);
  std::string line;
  while (std::getline(in, line)) {
    std::string t = Trim(line);
    if (t.empty() || t[0] == '#') continue;
    size_t p = t.find('\t');
    std::string bangla = Trim(p == std::string::npos ? t : t.substr(0, p));
    int freq = 8;  // default weight for generic lexicon words
    if (p != std::string::npos) {
      std::string f = Trim(t.substr(p + 1));
      // Only treat a purely-numeric second field as a frequency (so a raw
      // "bangla<TAB>phonemes" lexicon still loads, ignoring the phonemes).
      if (!f.empty() && f.find_first_not_of("0123456789") == std::string::npos)
        freq = std::atoi(f.c_str());
    }
    if (!bangla.empty()) words_.push_back({bangla, freq});
  }
  std::sort(words_.begin(), words_.end(),
            [](const Word& a, const Word& b) { return a.bangla < b.bangla; });
}

void Suggester::LoadLearnedData(const std::string& tsv) {
  std::istringstream in(tsv);
  std::string line;
  while (std::getline(in, line)) {
    std::string t = Trim(line);
    if (t.empty() || t[0] == '#') continue;
    size_t p = t.find('\t');
    if (p == std::string::npos) continue;
    std::string bangla = Trim(t.substr(0, p));
    int count = std::atoi(Trim(t.substr(p + 1)).c_str());
    if (!bangla.empty() && count > 0) learned_[bangla] = count;
  }
}

std::string Suggester::DumpLearnedData() const {
  std::string out;
  for (const auto& kv : learned_) {
    out += kv.first;
    out += '\t';
    out += std::to_string(kv.second);
    out += '\n';
  }
  return out;
}

void Suggester::RecordUsage(const std::string& bangla) {
  if (!bangla.empty()) ++learned_[bangla];
}

std::vector<std::string> Suggester::Suggest(const std::string& prefix,
                                            size_t max_results) const {
  std::vector<std::string> out;
  if (prefix.empty() || max_results == 0) return out;

  const std::string literal = Transliterate(prefix);  // element 0: as typed
  out.push_back(literal);

  std::string lower_prefix;
  lower_prefix.reserve(prefix.size());
  for (char c : prefix) lower_prefix.push_back(Lower(c));

  // Collect candidate Bangla words and their best static score. A word may
  // come from the roman dictionary (matched by romanized prefix) and/or the
  // word list (matched by Bangla prefix of the transliteration).
  std::unordered_map<std::string, int> cand;

  for (const Entry& e : entries_) {
    if (!KeyHasPrefix(e.key, lower_prefix)) continue;
    auto it = cand.find(e.bangla);
    if (it == cand.end() || e.freq > it->second) cand[e.bangla] = e.freq;
  }

  // Bangla-prefix completion: words starting with the transliteration. words_
  // is sorted, so binary-search the start of the range.
  if (!literal.empty()) {
    auto lo = std::lower_bound(
        words_.begin(), words_.end(), literal,
        [](const Word& w, const std::string& p) { return w.bangla < p; });
    for (auto it = lo; it != words_.end(); ++it) {
      if (it->bangla.compare(0, literal.size(), literal) != 0) break;
      auto c = cand.find(it->bangla);
      if (c == cand.end() || it->freq > c->second) cand[it->bangla] = it->freq;
    }
  }

  struct Scored {
    const std::string* bangla;
    int score;
  };
  std::vector<Scored> scored;
  scored.reserve(cand.size());
  for (const auto& kv : cand) {
    if (kv.first == literal) continue;  // already element 0
    int score = kv.second;
    auto l = learned_.find(kv.first);
    if (l != learned_.end()) score += l->second * kLearnWeight;
    scored.push_back({&kv.first, score});
  }
  std::stable_sort(scored.begin(), scored.end(),
                   [](const Scored& a, const Scored& b) {
                     if (a.score != b.score) return a.score > b.score;
                     return *a.bangla < *b.bangla;  // stable, deterministic
                   });

  for (const Scored& s : scored) {
    if (out.size() >= max_results) break;
    out.push_back(*s.bangla);
  }
  return out;
}

// Back-compat free function used by tests: a default suggester (built-in
// dictionary only, no learned data).
std::vector<std::string> Suggest(const std::string& prefix, size_t max_results) {
  static const Suggester kDefault;
  return kDefault.Suggest(prefix, max_results);
}

}  // namespace bnphonetic
