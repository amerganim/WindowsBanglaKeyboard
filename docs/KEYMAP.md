# Bangla Phonetic — Key Map / Typing Guide

How to type each Bangla letter, sign, digit, and conjunct with this keyboard.

**Important:** the scheme is **case-sensitive** — capital letters mean different
letters (e.g. `t` = ত but `T` = ট). Type a romanized word, then **Space** to
commit it (the space is inserted for you). **Ctrl+Shift+B** switches between
Bangla and English.

---

## Vowels

A vowel typed at the start of a word (or after another vowel) is an
**independent vowel**; typed right after a consonant it becomes a **vowel sign
(kar)**. Both are shown below (the kar is illustrated on ক = `k`).

| Letter | Type | After a consonant (kar) | Example |
|:------:|:----:|:-----------------------:|---------|
| অ | `o` | (inherent — no sign) | `kol` → কল |
| আ | `a` | কা (`ka`) | `kaj` → কাজ |
| ই | `i` | কি (`ki`) | `din` → দিন |
| ঈ | `I` | কী (`kI`) | `nodI` → নদী |
| উ | `u` | কু (`ku`) | `tumi` → তুমি |
| ঊ | `U` | কূ (`kU`) | `mUl` → মূল |
| ঋ | `rri` | কৃ (`krri`) | `rriSi` → ঋষি |
| এ | `e` | কে (`ke`) | `megh` → মেঘ |
| ঐ | `OI` | কৈ (`kOI`) | `OI` → ঐ |
| ও | `O` | কো (`kO`) | `nOka` → নোকা |
| ঔ | `OU` | কৌ (`kOU`) | `nOUka` → নৌকা |

---

## Consonants

| Letter | Type | | Letter | Type | | Letter | Type |
|:------:|:----:|---|:------:|:----:|---|:------:|:----:|
| ক | `k`  | | ট | `T`  | | প | `p` |
| খ | `kh` | | ঠ | `Th` | | ফ | `ph` / `f` |
| গ | `g`  | | ড | `D`  | | ব | `b` |
| ঘ | `gh` | | ঢ | `Dh` | | ভ | `bh` / `v` |
| ঙ | `Ng` | | ণ | `N`  | | ম | `m` |
| চ | `ch` | | ত | `t`  | | য | `z` |
| ছ | `chh`| | থ | `th` | | র | `r` |
| জ | `j`  | | দ | `d`  | | ল | `l` |
| ঝ | `jh` | | ধ | `dh` | | শ | `sh` |
| ঞ | `NG` | | ন | `n`  | | ষ | `S` / `Sh` |
| | | | | | | স | `s` |
| | | | | | | হ | `h` |

Special consonant forms:

| Letter | Type | Name |
|:------:|:----:|------|
| ড় | `R`  | rho (hard r) |
| ঢ় | `Rh` | rho-ha |
| য় | `y` / `Y` | ya with nukta (`noy` → নয়) |
| ৎ | `` t` `` | khanda-ta |

> Note: `j` = জ and `z` = য are different letters. `y`/`Y` = য় (the
> semivowel, as in `noy` → নয়).

> `w` = ব, used mainly to form the **bo-phola** (্ব) in conjuncts:
> `swopno` → স্বপ্ন, `bishwas` → বিশ্বাস.

---

## Signs

| Sign | Type | Name | Example |
|:----:|:----:|------|---------|
| ং | `ng` | anusvara | `bangla` → বাংলা |
| ঁ | `^`  | chandrabindu | `cha^d` → চাঁদ |
| ঃ | `:`  | visarga | `du:kh` → দুঃখ |
| । | `.`  | dari (full stop) | `sheS.` → শেষ। |
| ্ | `` ` `` | hasanta (force half-form) | see ref / ya-phala below |

> Note: in Bangla mode `.` always produces the dari (।). To type a Western
> period, switch to English with **Ctrl+Shift+B**.

---

## Digits

| ০ | ১ | ২ | ৩ | ৪ | ৫ | ৬ | ৭ | ৮ | ৯ |
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| `0` | `1` | `2` | `3` | `4` | `5` | `6` | `7` | `8` | `9` |

---

## Conjuncts (যুক্তাক্ষর / juktakkhor)

To make a conjunct, just type the consonants **with no vowel between them** —
the keyboard joins them with a hasanta (্) automatically.

| Conjunct | Type | Made of |
|:--------:|:----:|---------|
| ক্ক | `kk`  | ক + ক |
| ক্ত | `kt`  | ক + ত |
| ক্ষ | `kSh` | ক + ষ |
| ন্দ | `nd`  | ন + দ |
| ন্ত | `nt`  | ন + ত |
| ম্প | `mp`  | ম + প |
| স্ত | `st`  | স + ত |
| ল্ল | `ll`  | ল + ল |
| ষ্ট | `ShT` | ষ + ট |
| শ্চ | `shch`| শ + চ |

Examples: `sundor` → সুন্দর, `bistarito` → বিস্তারিত, `kShoma` → ক্ষমা.

### Explicit hasanta (`` ` ``) — ref and ya-phala

Type a backtick `` ` `` to force a hasanta (্) between two consonants:

| Want | Type | Result |
|------|:----:|:------:|
| ref (র্ + consonant) | `` r`k `` | র্ক |
| ya-phala (্য) | `` k`z `` | ক্য |

---

## Not yet supported (planned)

- Word suggestions / a candidate window.
- Disambiguating the `ng` digraph (anusvara) from a literal `n`+`g`, and a few
  rare ligatures.

*(ref and ya-phala work automatically — `korm` → কর্ম, `bidza` → বিদ্যা — and
the explicit hasanta `` ` `` is available for edge cases.)*

See [ARCHITECTURE.md](ARCHITECTURE.md) for the engine design.
