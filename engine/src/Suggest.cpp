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

  out.push_back(Transliterate(prefix));  // element 0: exactly what was typed

  std::string lower_prefix;
  lower_prefix.reserve(prefix.size());
  for (char c : prefix) lower_prefix.push_back(Lower(c));

  struct Scored {
    const Entry* e;
    int score;
  };
  std::vector<Scored> matches;
  for (const Entry& e : entries_) {
    if (!KeyHasPrefix(e.key, lower_prefix)) continue;
    int score = e.freq;
    auto it = learned_.find(e.bangla);
    if (it != learned_.end()) score += it->second * kLearnWeight;
    matches.push_back({&e, score});
  }
  std::stable_sort(matches.begin(), matches.end(),
                   [](const Scored& a, const Scored& b) {
                     return a.score > b.score;
                   });

  for (const Scored& m : matches) {
    if (out.size() >= max_results) break;
    if (std::find(out.begin(), out.end(), m.e->bangla) == out.end())
      out.push_back(m.e->bangla);
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
