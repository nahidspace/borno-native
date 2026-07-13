#pragma once
#include <windows.h>
#include <vector>
#include <string>

// A small, self-drawn, non-activating popup that lists spelling/word
// candidates near the caret -- deliberately NOT built on TSF's native
// ITfCandidateListUIElement machinery (poorly documented, historically
// inconsistent across host apps); a plain owned window we fully control
// is simpler and matches what most lightweight IMEs actually do.
class CCandidateWindow {
public:
    CCandidateWindow();
    ~CCandidateWindow();

    void Show(const RECT &caretRect, const std::vector<std::wstring> &candidates, size_t selected);
    void UpdateSelection(size_t selected);
    void Hide();
    bool IsVisible() const;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    void _EnsureWindow();
    void _Paint(HDC hdc);
    void _Layout(const RECT &caretRect);
    bool _IsDarkMode() const;

    HWND m_hwnd;
    std::vector<std::wstring> m_candidates;
    size_t m_selected;
    int m_rowHeight;
    HFONT m_hFontText;
    HFONT m_hFontIndex;
};
