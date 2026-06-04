#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace bnphonetic {

// Provides Bangla word suggestions for a romanized prefix, ranked by a static
// dictionary frequency plus a per-user "learned" count (words the user picks
// often float to the top). The engine performs no file I/O: the host reads the
// bundled dictionary and the user's learned data as UTF-8 text and passes them
// in, and reads back the learned data to persist it.
class Suggester {
 public:
  Suggester();  // seeds the small built-in dictionary

  // Add dictionary entries from UTF-8 TSV text: "roman<TAB>bangla<TAB>freq"
  // per line (freq optional). Matched by romanized prefix — good for mapping a
  // user's spelling (even imperfect) to a correct word. Adds to what's loaded.
  void LoadDictionaryData(const std::string& tsv);

  // Add a Bangla word list from UTF-8 TSV text: "bangla<TAB>freq" per line
  // (anything after the first tab that isn't a number is ignored, so a raw
  // pronunciation lexicon "bangla<TAB>phonemes" also works). Matched by Bangla
  // prefix against the transliteration of what the user typed — good for broad
  // word completion. Adds to what's loaded.
  void LoadWordList(const std::string& tsv);

  // Load / dump the learned counts as UTF-8 TSV "bangla<TAB>count" per line.
  void LoadLearnedData(const std::string& tsv);
  std::string DumpLearnedData() const;

  // Record that the user committed `bangla` (boosts it in future ranking).
  void RecordUsage(const std::string& bangla);

  // Suggestions for `prefix`: element 0 is always the literal transliteration
  // of `prefix`; the rest are dictionary words whose romanization starts with
  // `prefix` (case-insensitive), best-ranked first, de-duplicated.
  std::vector<std::string> Suggest(const std::string& prefix,
                                   size_t max_results = 9) const;

 private:
  struct Entry {
    std::string key;     // romanization (match target)
    std::string bangla;  // suggestion shown/committed
    int freq;            // static frequency weight
  };
  struct Word {
    std::string bangla;  // matched by Bangla prefix
    int freq;
  };
  std::vector<Entry> entries_;          // roman-keyed dictionary
  std::vector<Word> words_;             // Bangla word list, sorted by `bangla`
  std::unordered_map<std::string, int> learned_;  // bangla -> use count
};

}  // namespace bnphonetic
