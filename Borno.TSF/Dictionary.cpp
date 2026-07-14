#include "Dictionary.h"
#include "Resource.h"
#include "Globals.h"
#include <windows.h>
#include <unordered_map>

namespace BornoDictionary {

namespace {

std::unordered_map<std::wstring, float> LoadFromResource() {
    std::unordered_map<std::wstring, float> m;

    HRSRC hRes = FindResourceW(g_hInst, MAKEINTRESOURCEW(IDR_BN_WORDFREQ), RT_RCDATA);
    if (!hRes) return m;
    HGLOBAL hData = LoadResource(g_hInst, hRes);
    if (!hData) return m;
    DWORD size = SizeofResource(g_hInst, hRes);
    const char* data = static_cast<const char*>(LockResource(hData));
    if (!data || size == 0) return m;

    // Resource is UTF-8 text, no BOM: "word\tzipf_frequency\n" per line.
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, data, (int)size, nullptr, 0);
    std::wstring text(wideLen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, data, (int)size, text.data(), wideLen);

    m.reserve(30000);
    size_t pos = 0;
    while (pos < text.size()) {
        size_t lineEnd = text.find(L'\n', pos);
        if (lineEnd == std::wstring::npos) lineEnd = text.size();
        size_t tab = text.find(L'\t', pos);
        if (tab != std::wstring::npos && tab < lineEnd) {
            std::wstring word = text.substr(pos, tab - pos);
            std::wstring freqStr = text.substr(tab + 1, lineEnd - tab - 1);
            if (!freqStr.empty() && freqStr.back() == L'\r') freqStr.pop_back();
            try {
                m[word] = std::stof(freqStr);
            } catch (...) {
                // malformed line -- skip
            }
        }
        pos = lineEnd + 1;
    }
    return m;
}

const std::unordered_map<std::wstring, float>& GetMap() {
    static const std::unordered_map<std::wstring, float> map = LoadFromResource();
    return map;
}

} // namespace

float Lookup(const std::wstring& word) {
    const auto& map = GetMap();
    auto it = map.find(word);
    return it != map.end() ? it->second : -1.0f;
}

} // namespace BornoDictionary
