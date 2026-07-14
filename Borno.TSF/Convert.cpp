#include "Convert.h"
#include "Dictionary.h"
#include <cwchar>
#include <algorithm>
#include <initializer_list>
#include <set>
#include <utility>

namespace BornoCore {

namespace {

enum class TokenKind { Consonant, Vowel, Modifier };

struct Rule {
    const wchar_t* key;
    TokenKind kind;
    const wchar_t* standalone; // consonant/modifier glyph, or vowel's standalone glyph
    const wchar_t* kar;        // vowel sign used right after a consonant (unused otherwise)
};

// Deliberately small and case-sensitive (T/t, D/d, N/n, Sh/sh distinguish
// retroflex vs dental/palatal consonants, matching real Avro convention).
// Cross-checked against the pyAvroPhonetic oracle (tools/oracle/) to fix
// real gaps -- vowel-digraph case sensitivity, which consonant clusters
// actually form conjuncts, digits, and dari punctuation. Still-known gaps:
// "w" handling, nasal assimilation before palatals (shunchi -> should be
// শুঞ্ছি, we give শুন্ছি), and English loanwords like "class".
const Rule kRules[] = {
    // irregular multi-letter consonant clusters (checked before their
    // shorter prefixes so these win the greedy longest-match)
    { L"kkh", TokenKind::Consonant, L"ক্ষ", nullptr },
    { L"gg", TokenKind::Consonant, L"জ্ঞ", nullptr },

    // consonant digraphs (checked before their single-letter prefixes)
    { L"kh", TokenKind::Consonant, L"খ", nullptr },
    { L"gh", TokenKind::Consonant, L"ঘ", nullptr },
    { L"ch", TokenKind::Consonant, L"ছ", nullptr },
    { L"jh", TokenKind::Consonant, L"ঝ", nullptr },
    { L"Th", TokenKind::Consonant, L"ঠ", nullptr },
    { L"Dh", TokenKind::Consonant, L"ঢ", nullptr },
    { L"th", TokenKind::Consonant, L"থ", nullptr },
    { L"dh", TokenKind::Consonant, L"ধ", nullptr },
    { L"ph", TokenKind::Consonant, L"ফ", nullptr },
    { L"bh", TokenKind::Consonant, L"ভ", nullptr },
    { L"Sh", TokenKind::Consonant, L"ষ", nullptr },
    { L"sh", TokenKind::Consonant, L"শ", nullptr },

    // consonants
    { L"K", TokenKind::Consonant, L"ক", nullptr },
    { L"k", TokenKind::Consonant, L"ক", nullptr },
    { L"g", TokenKind::Consonant, L"গ", nullptr },
    { L"c", TokenKind::Consonant, L"চ", nullptr },
    { L"j", TokenKind::Consonant, L"জ", nullptr },
    { L"T", TokenKind::Consonant, L"ট", nullptr },
    { L"D", TokenKind::Consonant, L"ড", nullptr },
    { L"N", TokenKind::Consonant, L"ণ", nullptr },
    { L"t", TokenKind::Consonant, L"ত", nullptr },
    { L"d", TokenKind::Consonant, L"দ", nullptr },
    { L"n", TokenKind::Consonant, L"ন", nullptr },
    { L"p", TokenKind::Consonant, L"প", nullptr },
    { L"f", TokenKind::Consonant, L"ফ", nullptr },
    { L"b", TokenKind::Consonant, L"ব", nullptr },
    { L"v", TokenKind::Consonant, L"ভ", nullptr },
    { L"m", TokenKind::Consonant, L"ম", nullptr },
    { L"z", TokenKind::Consonant, L"জ", nullptr },
    { L"r", TokenKind::Consonant, L"র", nullptr },
    { L"l", TokenKind::Consonant, L"ল", nullptr },
    { L"s", TokenKind::Consonant, L"স", nullptr },
    { L"h", TokenKind::Consonant, L"হ", nullptr },
    // U+09DF precomposed YYA -- verified against the oracle to be the form
    // other Avro implementations emit (this source previously round-tripped
    // it as decomposed YA + nukta through an editor pass; the escape here
    // pins the exact codepoint regardless of toolchain).
    { L"y", TokenKind::Consonant, L"য়", nullptr },

    // vowels: standalone glyph + kar (vowel sign after a consonant).
    // "o" has an empty kar: Bangla consonants carry an inherent /o/ sound,
    // so a plain "o" right after a consonant is silent/invisible.
    // Lowercase "oi"/"ou" are deliberately NOT digraphs here -- cross-checked
    // against the oracle, "oi" parses as separate o+i (অই) and only the
    // capitalized "OI"/"OU" trigger the ঐ/ঔ digraphs.
    { L"OU", TokenKind::Vowel, L"ঔ", L"ৌ" },
    { L"OI", TokenKind::Vowel, L"ঐ", L"ৈ" },
    { L"ee", TokenKind::Vowel, L"ঈ", L"ী" },
    { L"oo", TokenKind::Vowel, L"ঊ", L"ূ" },
    { L"o", TokenKind::Vowel, L"অ", L"" },
    { L"a", TokenKind::Vowel, L"আ", L"া" },
    { L"i", TokenKind::Vowel, L"ই", L"ি" },
    { L"I", TokenKind::Vowel, L"ঈ", L"ী" },
    { L"u", TokenKind::Vowel, L"উ", L"ু" },
    { L"U", TokenKind::Vowel, L"ঊ", L"ূ" },
    { L"e", TokenKind::Vowel, L"এ", L"ে" },
    { L"O", TokenKind::Vowel, L"ও", L"ো" },

    // modifiers: fixed glyph, never take a kar, never chain into a conjunct
    { L"ng", TokenKind::Modifier, L"ং", nullptr },
    { L":", TokenKind::Modifier, L"ঃ", nullptr },
    { L"^", TokenKind::Modifier, L"ঁ", nullptr },
};

const Rule* FindRule(const std::wstring& s, size_t pos) {
    const Rule* best = nullptr;
    size_t bestLen = 0;
    for (const auto& rule : kRules) {
        size_t len = wcslen(rule.key);
        if (len <= bestLen) continue;
        if (s.compare(pos, len, rule.key) == 0) {
            best = &rule;
            bestLen = len;
        }
    }
    return best;
}

// Which consonant-key pairs actually form a conjunct when adjacent with no
// vowel between them (e.g. "kkhoma" -> ক্ষমা needs k+kh to conjunct, but
// "korchi" -> করছি must NOT conjunct r+ch). Bangla only conjuncts a curated
// set of clusters, not every adjacent pair -- this table of 203 pairs was
// derived empirically by exhaustively testing all ~1100 consonant-key
// combinations against the pyAvroPhonetic oracle (tools/oracle/conjunct_matrix.tsv)
// and recording which ones it renders with a hasant. Facts about which
// clusters conjunct, not copied code/data -- see the licensing note in
// tools/oracle/README.md.
bool FormsConjunct(const std::wstring& prev, const std::wstring& curr) {
    static const std::set<std::pair<std::wstring, std::wstring>> kConjunctPairs = {
        {L"k", L"k"}, {L"k", L"kh"}, {L"k", L"T"}, {L"k", L"Th"}, {L"k", L"t"}, {L"k", L"th"},
        {L"K", L"s"}, {L"K", L"Sh"}, {L"K", L"y"},
        {L"k", L"r"}, {L"k", L"l"}, {L"k", L"Sh"}, {L"k", L"s"}, {L"k", L"y"}, {L"kh", L"r"},
        {L"kh", L"y"}, {L"g", L"g"}, {L"g", L"gh"}, {L"g", L"N"}, {L"g", L"dh"}, {L"g", L"n"},
        {L"g", L"m"}, {L"g", L"r"}, {L"g", L"l"}, {L"g", L"y"}, {L"gh", L"n"}, {L"gh", L"r"},
        {L"gh", L"y"}, {L"c", L"c"}, {L"c", L"ch"}, {L"c", L"r"}, {L"c", L"y"}, {L"ch", L"r"},
        {L"ch", L"y"}, {L"j", L"j"}, {L"j", L"jh"}, {L"j", L"r"}, {L"j", L"y"}, {L"jh", L"r"},
        {L"jh", L"y"}, {L"T", L"T"}, {L"T", L"Th"}, {L"T", L"m"}, {L"T", L"r"}, {L"T", L"y"},
        {L"Th", L"r"}, {L"Th", L"y"}, {L"D", L"D"}, {L"D", L"Dh"}, {L"D", L"r"}, {L"D", L"y"},
        {L"Dh", L"r"}, {L"Dh", L"y"}, {L"N", L"T"}, {L"N", L"Th"}, {L"N", L"D"}, {L"N", L"Dh"},
        {L"N", L"N"}, {L"N", L"n"}, {L"N", L"m"}, {L"N", L"r"}, {L"N", L"y"}, {L"t", L"t"},
        {L"t", L"th"}, {L"t", L"n"}, {L"t", L"m"}, {L"t", L"r"}, {L"t", L"y"}, {L"th", L"r"},
        {L"th", L"y"}, {L"d", L"g"}, {L"d", L"gh"}, {L"d", L"d"}, {L"d", L"dh"}, {L"d", L"v"},
        {L"d", L"bh"}, {L"d", L"m"}, {L"d", L"r"}, {L"d", L"y"}, {L"dh", L"n"}, {L"dh", L"m"},
        {L"dh", L"r"}, {L"dh", L"y"}, {L"n", L"k"}, {L"n", L"kh"}, {L"n", L"gh"}, {L"n", L"c"},
        {L"n", L"ch"}, {L"n", L"j"}, {L"n", L"jh"}, {L"n", L"T"}, {L"n", L"Th"}, {L"n", L"D"},
        {L"n", L"Dh"}, {L"n", L"t"}, {L"n", L"th"}, {L"n", L"d"}, {L"n", L"dh"}, {L"n", L"n"},
        {L"n", L"m"}, {L"n", L"r"}, {L"n", L"s"}, {L"n", L"y"}, {L"p", L"T"}, {L"p", L"Th"},
        {L"p", L"t"}, {L"p", L"th"}, {L"p", L"n"}, {L"p", L"p"}, {L"p", L"ph"}, {L"p", L"r"},
        {L"p", L"l"}, {L"p", L"s"}, {L"p", L"y"}, {L"f", L"r"}, {L"f", L"l"}, {L"f", L"y"},
        {L"ph", L"r"}, {L"ph", L"l"}, {L"ph", L"y"}, {L"b", L"j"}, {L"b", L"jh"}, {L"b", L"d"},
        {L"b", L"dh"}, {L"b", L"b"}, {L"b", L"bh"}, {L"b", L"r"}, {L"b", L"l"}, {L"b", L"y"},
        {L"v", L"r"}, {L"v", L"l"}, {L"v", L"y"}, {L"bh", L"r"}, {L"bh", L"l"}, {L"bh", L"y"},
        {L"m", L"th"}, {L"m", L"n"}, {L"m", L"p"}, {L"m", L"f"}, {L"m", L"ph"}, {L"m", L"b"},
        {L"m", L"v"}, {L"m", L"bh"}, {L"m", L"m"}, {L"m", L"r"}, {L"m", L"l"}, {L"m", L"y"},
        {L"z", L"r"}, {L"z", L"y"}, {L"r", L"y"}, {L"l", L"k"}, {L"l", L"g"}, {L"l", L"T"},
        {L"l", L"Th"}, {L"l", L"D"}, {L"l", L"Dh"}, {L"l", L"dh"}, {L"l", L"p"}, {L"l", L"b"},
        {L"l", L"v"}, {L"l", L"bh"}, {L"l", L"m"}, {L"l", L"r"}, {L"l", L"l"}, {L"l", L"y"},
        {L"Sh", L"k"}, {L"Sh", L"kh"}, {L"Sh", L"T"}, {L"Sh", L"Th"}, {L"Sh", L"N"}, {L"Sh", L"p"},
        {L"Sh", L"f"}, {L"Sh", L"ph"}, {L"Sh", L"m"}, {L"Sh", L"r"}, {L"Sh", L"y"}, {L"sh", L"c"},
        {L"sh", L"ch"}, {L"sh", L"t"}, {L"sh", L"th"}, {L"sh", L"n"}, {L"sh", L"m"}, {L"sh", L"r"},
        {L"sh", L"l"}, {L"sh", L"y"}, {L"s", L"k"}, {L"s", L"kh"}, {L"s", L"T"}, {L"s", L"Th"},
        {L"s", L"t"}, {L"s", L"th"}, {L"s", L"n"}, {L"s", L"p"}, {L"s", L"f"}, {L"s", L"ph"},
        {L"s", L"m"}, {L"s", L"r"}, {L"s", L"l"}, {L"s", L"y"}, {L"h", L"N"}, {L"h", L"n"},
        {L"h", L"m"}, {L"h", L"r"}, {L"h", L"l"}, {L"h", L"y"}, {L"y", L"y"},
    };
    const std::wstring normalizedPrev = prev == L"K" ? L"k" : prev;
    return kConjunctPairs.count({ normalizedPrev, curr }) > 0;
}

const wchar_t kBengaliDigits[10] = { L'০', L'১', L'২', L'৩', L'৪', L'৫', L'৬', L'৭', L'৮', L'৯' };

} // namespace

std::wstring Convert(const std::wstring& input) {
    std::wstring out;
    out.reserve(input.size() * 2);
    bool afterConsonant = false;
    std::wstring lastConsonantKey;

    size_t i = 0;
    while (i < input.size()) {
        wchar_t c = input[i];

        if (c >= L'0' && c <= L'9') {
            out.push_back(kBengaliDigits[c - L'0']);
            afterConsonant = false;
            ++i;
            continue;
        }

        // "." -> দাঁড়ি, ".." -> double দাঁড়ি, "..." (or more) passes through
        // unchanged (recognized as an ellipsis, per the oracle).
        if (c == L'.') {
            size_t runStart = i;
            while (i < input.size() && input[i] == L'.') ++i;
            size_t runLen = i - runStart;
            if (runLen == 1) out += L"।";
            else if (runLen == 2) out += L"।।";
            else out.append(runLen, L'.');
            afterConsonant = false;
            continue;
        }

        const Rule* rule = FindRule(input, i);
        if (!rule) {
            out.push_back(c);
            afterConsonant = false;
            ++i;
            continue;
        }

        switch (rule->kind) {
        case TokenKind::Consonant:
            if (afterConsonant && FormsConjunct(lastConsonantKey, rule->key)) {
                // Homorganic nasal assimilation: a dental ন about to conjunct
                // with a following velar or palatal stop takes that stop's
                // own nasal instead -- cross-checked against the oracle:
                // "ank" -> আঙ্ক, "anc" -> আঞ্চ, but "ant"/"anm" do not change.
                if (lastConsonantKey == L"n" && !out.empty()) {
                    static const std::set<std::wstring> kVelars = { L"k", L"kh", L"g", L"gh" };
                    static const std::set<std::wstring> kPalatals = { L"c", L"ch", L"j", L"jh" };
                    if (kVelars.count(rule->key)) out.back() = L'ঙ';       // ঙ
                    else if (kPalatals.count(rule->key)) out.back() = L'ঞ'; // ঞ
                }
                out += L"্"; // hasant: the shaping engine forms the conjunct
            }
            out += rule->standalone;
            afterConsonant = true;
            lastConsonantKey = rule->key;
            break;
        case TokenKind::Vowel:
            out += afterConsonant ? rule->kar : rule->standalone;
            afterConsonant = false;
            break;
        case TokenKind::Modifier:
            out += rule->standalone;
            afterConsonant = false;
            break;
        }
        i += wcslen(rule->key);
    }
    return out;
}

std::vector<std::wstring> GetCandidates(const std::wstring& input) {
    std::wstring primary = Convert(input);
    std::vector<std::wstring> out;
    out.push_back(primary);
    if (primary.empty()) return out;

    // Re-convert phonetic alternatives instead of replacing rendered
    // codepoints directly. Kar placement is context-sensitive.
    struct AlternativeKeys {
        const wchar_t *key;
        std::initializer_list<const wchar_t*> alternatives;
    };
    static const AlternativeKeys kAlternatives[] = {
        { L"s",  { L"sh", L"Sh" } },
        { L"sh", { L"s", L"Sh" } },
        { L"Sh", { L"s", L"sh" } },
        { L"i",  { L"I", L"ee" } },
        { L"I",  { L"i", L"ee" } },
        { L"ee", { L"i", L"I" } },
        { L"u",  { L"U", L"oo" } },
        { L"U",  { L"u", L"oo" } },
        { L"oo", { L"u", L"U" } },
    };

    for (size_t pos = 0; pos < input.size();) {
        const Rule *rule = FindRule(input, pos);
        if (!rule) {
            ++pos;
            continue;
        }
        for (const auto &alternative : kAlternatives) {
            if (std::wstring(rule->key) != alternative.key) continue;
            for (const wchar_t *replacement : alternative.alternatives) {
                std::wstring variantInput = input;
                variantInput.replace(pos, wcslen(rule->key), replacement);
                std::wstring variant = Convert(variantInput);
                if (std::find(out.begin(), out.end(), variant) == out.end())
                    out.push_back(std::move(variant));
            }
            break;
        }
        pos += wcslen(rule->key);
    }

    // Rank and cap after all positions have been considered.
    std::stable_sort(out.begin(), out.end(),
        [](const std::wstring& a, const std::wstring& b) {
            return BornoDictionary::Lookup(a) > BornoDictionary::Lookup(b);
        });
    if (out.size() > 4) out.resize(4);
    return out;

    // Legacy rendered-codepoint candidate generation retained for reference.
#if 0
    // Single Bangla letters/signs that sound alike, so a phonetic guess for
    // one is just as legitimate a spelling as any other in the class.
    static const std::vector<std::vector<wchar_t>> kEquivClasses = {
        { L'স', L'শ', L'ষ' }, // dental / palatal / retroflex sibilant
        { L'ই', L'ঈ' },       // hrasva / dirgho i (standalone)
        { L'ি', L'ী' },       // hrasva / dirgho i (kar)
        { L'উ', L'ঊ' },       // hrasva / dirgho u (standalone)
        { L'ু', L'ূ' },       // hrasva / dirgho u (kar)
    };

    static const size_t kMaxCandidates = 4;

    // Generate every single-position substitution across the whole word
    // first, uncapped -- a fix needed at a later position (e.g. "bishesh"
    // -> বিশেষ needs the *second* শ swapped to ষ) must not be missed just
    // because earlier positions already filled the old generation cap.
    // Ranking and truncation happen after, once every candidate exists.
    for (size_t pos = 0; pos < primary.size(); ++pos) {
        for (const auto& cls : kEquivClasses) {
            auto it = std::find(cls.begin(), cls.end(), primary[pos]);
            if (it == cls.end()) continue;

            for (wchar_t alt : cls) {
                if (alt == *it) continue;
                std::wstring variant = primary;
                variant[pos] = alt;
                if (std::find(out.begin(), out.end(), variant) == out.end())
                    out.push_back(variant);
            }
            break; // this position already matched a class; don't test the rest
        }
    }

    // Rank real dictionary words first (by frequency); non-dictionary
    // mechanical variants sink to the back but aren't dropped outright,
    // since our 25k-word dictionary is necessarily incomplete.
    std::stable_sort(out.begin(), out.end(),
        [](const std::wstring& a, const std::wstring& b) {
            return BornoDictionary::Lookup(a) > BornoDictionary::Lookup(b);
        });

    if (out.size() > kMaxCandidates) out.resize(kMaxCandidates);

    return out;
#endif
}

} // namespace BornoCore
