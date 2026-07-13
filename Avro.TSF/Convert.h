#pragma once
#include <string>
#include <vector>

// Avro.Core placeholder. This is a small demonstration rule set for the TSF
// skeleton (V2 of the roadmap), not the production Avro Phonetic ruleset.
// TODO: replace with the ported OmicronLab Avro Phonetic rules + golden tests.
namespace AvroCore {
    std::wstring Convert(const std::wstring& input);

    // Primary conversion plus spelling-variant alternates, primary first.
    // Alternates come from genuine Bangla orthographic redundancy (the three
    // sibilants স/শ/ষ, and the hrasva/dirgho i and u pairs, all sound alike),
    // which is exactly why a phonetic typist needs to pick from a list.
    std::vector<std::wstring> GetCandidates(const std::wstring& input);
}
