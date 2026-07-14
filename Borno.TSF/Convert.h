#pragma once
#include <string>
#include <vector>

// Borno.Core phonetic rule set for the native TSF service.
namespace BornoCore {
    std::wstring Convert(const std::wstring& input);

    // Primary conversion plus spelling-variant alternates, primary first.
    // Alternates come from genuine Bangla orthographic redundancy (the three
    // sibilants স/শ/ষ, and the hrasva/dirgho i and u pairs, all sound alike),
    // which is exactly why a phonetic typist needs to pick from a list.
    std::vector<std::wstring> GetCandidates(const std::wstring& input);
}
