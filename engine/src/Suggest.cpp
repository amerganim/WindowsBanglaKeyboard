#include "bnphonetic/Transliterator.h"

#include <algorithm>
#include <cctype>

// Word-suggestion dictionary. Each entry maps a romanized key (what the user
// types) to the correct Bangla word and a rough frequency weight (higher =
// more common). Suggestions are matched by case-insensitive prefix on the key
// and returned most-frequent first, so this serves both the simple
// completion list and the frequency-ranked behaviour.
//
// Compiled with /utf-8 (Bangla string literals). This is a curated starter
// list; it is meant to grow.

namespace bnphonetic {
namespace {

struct DictEntry {
  const char* key;
  const char* bangla;
  int freq;
};

const DictEntry kDict[] = {
    // pronouns / very common
    {"ami", "আমি", 100},        {"amar", "আমার", 95},
    {"amake", "আমাকে", 70},     {"amader", "আমাদের", 70},
    {"tumi", "তুমি", 95},       {"tomar", "তোমার", 85},
    {"tomake", "তোমাকে", 65},   {"tomader", "তোমাদের", 60},
    {"apni", "আপনি", 80},       {"apnar", "আপনার", 70},
    {"se", "সে", 85},           {"tar", "তার", 80},
    {"tara", "তারা", 70},       {"tader", "তাদের", 60},
    {"ora", "ওরা", 50},         {"era", "এরা", 50},
    {"eta", "এটা", 70},         {"ota", "ওটা", 55},
    {"ei", "এই", 80},           {"oi", "ওই", 55},
    {"ja", "যা", 50},           {"jodi", "যদি", 65},

    // question words
    {"ki", "কি", 90},           {"kothay", "কোথায়", 70},
    {"keno", "কেন", 75},        {"kemon", "কেমন", 75},
    {"kobe", "কবে", 55},        {"ke", "কে", 70},
    {"koto", "কত", 65},         {"kon", "কোন", 65},
    {"kivabe", "কীভাবে", 55},

    // common verbs / states
    {"bhalo", "ভালো", 90},      {"bhalobasa", "ভালোবাসা", 70},
    {"kharap", "খারাপ", 55},    {"achho", "আছো", 55},
    {"achhi", "আছি", 70},       {"achhe", "আছে", 75},
    {"korbo", "করব", 60},       {"kori", "করি", 60},
    {"kora", "করা", 65},        {"korte", "করতে", 60},
    {"holo", "হলো", 60},        {"hobe", "হবে", 70},
    {"hoyechhe", "হয়েছে", 55},  {"jabo", "যাব", 55},
    {"jai", "যাই", 55},         {"khabo", "খাব", 50},
    {"dekha", "দেখা", 55},      {"bola", "বলা", 55},
    {"jana", "জানা", 50},       {"chai", "চাই", 60},

    // greetings / time
    {"dhonnobad", "ধন্যবাদ", 80}, {"shubho", "শুভ", 60},
    {"sokal", "সকাল", 55},      {"bikal", "বিকাল", 45},
    {"rat", "রাত", 55},         {"din", "দিন", 60},
    {"aj", "আজ", 65},           {"kal", "কাল", 55},
    {"ekhon", "এখন", 65},       {"tahole", "তাহলে", 55},
    {"abar", "আবার", 60},

    // common nouns
    {"bangla", "বাংলা", 85},    {"bangladesh", "বাংলাদেশ", 75},
    {"desh", "দেশ", 60},        {"manush", "মানুষ", 65},
    {"bondhu", "বন্ধু", 65},    {"poribar", "পরিবার", 55},
    {"baba", "বাবা", 65},       {"ma", "মা", 70},
    {"bhai", "ভাই", 65},        {"bon", "বোন", 55},
    {"chhele", "ছেলে", 55},     {"meye", "মেয়ে", 55},
    {"boi", "বই", 60},          {"pani", "পানি", 55},
    {"jol", "জল", 50},          {"khabar", "খাবার", 55},
    {"bari", "বাড়ি", 60},       {"basa", "বাসা", 50},
    {"shohor", "শহর", 50},      {"gram", "গ্রাম", 50},
    {"prithibi", "পৃথিবী", 50}, {"akash", "আকাশ", 50},
    {"somoy", "সময়", 65},       {"kaj", "কাজ", 65},
    {"taka", "টাকা", 60},       {"nam", "নাম", 60},
    {"kotha", "কথা", 65},       {"prem", "প্রেম", 50},
    {"jibon", "জীবন", 60},      {"mon", "মন", 60},
    {"chokh", "চোখ", 50},       {"hat", "হাত", 50},
    {"matha", "মাথা", 50},      {"phul", "ফুল", 50},

    // number words
    {"ek", "এক", 65},           {"dui", "দুই", 60},
    {"tin", "তিন", 55},         {"char", "চার", 55},
    {"panch", "পাঁচ", 50},      {"chhoy", "ছয়", 45},
    {"sat", "সাত", 45},         {"at", "আট", 45},
    {"noy", "নয়", 45},          {"dosh", "দশ", 45},

    // adjectives / adverbs
    {"boro", "বড়", 60},         {"choto", "ছোট", 55},
    {"notun", "নতুন", 60},      {"purono", "পুরনো", 45},
    {"sundor", "সুন্দর", 70},   {"onek", "অনেক", 70},
    {"ektu", "একটু", 55},       {"sob", "সব", 65},
    {"kichu", "কিছু", 65},      {"aro", "আরও", 55},
    {"taratari", "তাড়াতাড়ি", 45}, {"dhire", "ধীরে", 40},
};

char Lower(char c) {
  return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

// True if `key` starts with `lower_prefix` (the prefix is already lowercased;
// the key is lowercased on the fly).
bool KeyHasPrefix(const char* key, const std::string& lower_prefix) {
  for (size_t i = 0; i < lower_prefix.size(); ++i) {
    if (key[i] == '\0' || Lower(key[i]) != lower_prefix[i]) return false;
  }
  return true;
}

}  // namespace

std::vector<std::string> Suggest(const std::string& prefix, size_t max_results) {
  std::vector<std::string> out;
  if (prefix.empty() || max_results == 0) return out;

  // Element 0: exactly what the user typed.
  out.push_back(Transliterate(prefix));

  std::string lower_prefix;
  lower_prefix.reserve(prefix.size());
  for (char c : prefix) lower_prefix.push_back(Lower(c));

  std::vector<const DictEntry*> matches;
  for (const DictEntry& e : kDict) {
    if (KeyHasPrefix(e.key, lower_prefix)) matches.push_back(&e);
  }
  std::stable_sort(matches.begin(), matches.end(),
                   [](const DictEntry* a, const DictEntry* b) {
                     return a->freq > b->freq;
                   });

  for (const DictEntry* e : matches) {
    if (out.size() >= max_results) break;
    std::string word = e->bangla;
    if (std::find(out.begin(), out.end(), word) == out.end())
      out.push_back(std::move(word));
  }
  return out;
}

}  // namespace bnphonetic
