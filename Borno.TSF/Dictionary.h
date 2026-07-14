#pragma once
#include <string>

// Bangla word-frequency lookup, backed by an embedded resource
// (Resources/bn_wordfreq.tsv -- see Resources/ATTRIBUTION.md for sourcing
// and the CC BY-SA 4.0 obligations that apply to that data file only).
namespace BornoDictionary {
    // Returns the word's Zipf frequency (roughly log10 of occurrences per
    // billion words -- higher means more common), or a negative value if
    // the word isn't in the dictionary.
    float Lookup(const std::wstring& word);
}
