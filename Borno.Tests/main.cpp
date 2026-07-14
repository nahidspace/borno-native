// Console test harness: reads one phonetic word per line from argv[1],
// runs it through BornoCore::Convert, writes "word\tresult" lines (UTF-8) to
// argv[2]. Used to diff our engine's output against the pyAvroPhonetic
// reference oracle for cross-verification -- not shipped with the product.
//
// Reads/writes UTF-8 via explicit Win32 MultiByteToWideChar/WideCharToMultiByte
// rather than std::wifstream/wofstream + <codecvt>: that deprecated codecvt_utf8
// facet was observed inserting a spurious mid-stream BOM before non-ASCII
// segments in this MSVC STL, corrupting the very output this tool exists to
// produce accurately.
#include "../Borno.TSF/Convert.h"
#include <windows.h>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>

namespace {

std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
    return w;
}

std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}

std::wstring TrimCr(std::wstring s) {
    while (!s.empty() && (s.back() == L'\r' || s.back() == L'\n')) s.pop_back();
    return s;
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc < 3) {
        std::wcerr << L"usage: BornoTests.exe <input-words.txt> <output.tsv>\n";
        return 1;
    }

    std::ifstream in(argv[1], std::ios::binary);
    std::ofstream out(argv[2], std::ios::binary);

    std::string byteLine;
    while (std::getline(in, byteLine)) {
        std::wstring line = TrimCr(Utf8ToWide(byteLine));
        if (line.empty() || line[0] == L'#') continue;
        std::wstring converted = BornoCore::Convert(line);
        out << WideToUtf8(line) << "\t" << WideToUtf8(converted);

        auto candidates = BornoCore::GetCandidates(line);
        for (const auto& c : candidates) out << "\t" << WideToUtf8(c);
        out << "\n";
    }
    return 0;
}
