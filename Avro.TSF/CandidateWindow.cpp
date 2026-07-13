#include "CandidateWindow.h"
#include "Globals.h"
#include "Debug.h"
#include <usp10.h>
#include <cwchar>

#pragma comment(lib, "usp10.lib")

namespace {

constexpr wchar_t kClassName[] = L"AvroTSFCandidateWindow";
constexpr int kPaddingX = 10;
constexpr int kPaddingY = 6;
constexpr int kIndexColumnWidth = 22;

ATOM RegisterWindowClassOnce() {
    static ATOM atom = 0;
    if (atom) return atom;

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = CCandidateWindow::WndProc;
    wc.hInstance = g_hInst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kClassName;
    atom = RegisterClassExW(&wc);
    return atom;
}

// Measures/draws text with Uniscribe (not plain GDI TextOut) so complex-script
// shaping -- Bangla conjuncts and reordering -- renders correctly, with
// automatic font fallback for the Bangla runs even though our selected font
// (Segoe UI) has no Bengali glyphs of its own.
SIZE MeasureText(HDC hdc, const std::wstring &text) {
    SIZE size = { 0, 0 };
    SCRIPT_STRING_ANALYSIS ssa;
    if (SUCCEEDED(ScriptStringAnalyse(hdc, text.c_str(), (int)text.length(),
            (int)(text.length() * 1.5) + 16, -1, SSA_GLYPHS | SSA_FALLBACK, 0,
            nullptr, nullptr, nullptr, nullptr, nullptr, &ssa))) {
        if (const SIZE *sz = ScriptString_pSize(ssa)) size = *sz;
        ScriptStringFree(&ssa);
    }
    return size;
}

void DrawShapedText(HDC hdc, const std::wstring &text, int x, int y) {
    SCRIPT_STRING_ANALYSIS ssa;
    if (SUCCEEDED(ScriptStringAnalyse(hdc, text.c_str(), (int)text.length(),
            (int)(text.length() * 1.5) + 16, -1, SSA_GLYPHS | SSA_FALLBACK, 0,
            nullptr, nullptr, nullptr, nullptr, nullptr, &ssa))) {
        ScriptStringOut(ssa, x, y, 0, nullptr, 0, 0, FALSE);
        ScriptStringFree(&ssa);
    }
}

} // namespace

CCandidateWindow::CCandidateWindow()
    : m_hwnd(nullptr), m_selected(0), m_rowHeight(0), m_hFontText(nullptr), m_hFontIndex(nullptr) {
}

CCandidateWindow::~CCandidateWindow() {
    if (m_hFontText) DeleteObject(m_hFontText);
    if (m_hFontIndex) DeleteObject(m_hFontIndex);
    if (m_hwnd) DestroyWindow(m_hwnd);
}

bool CCandidateWindow::_IsDarkMode() const {
    HKEY hKey;
    bool dark = false;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD value = 1, size = sizeof(value);
        if (RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr, (BYTE*)&value, &size) == ERROR_SUCCESS)
            dark = (value == 0);
        RegCloseKey(hKey);
    }
    return dark;
}

void CCandidateWindow::_EnsureWindow() {
    if (m_hwnd) return;

    RegisterWindowClassOnce();
    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        kClassName, L"",
        WS_POPUP,
        0, 0, 0, 0,
        nullptr, nullptr, g_hInst, this);

    m_hFontText = CreateFontW(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hFontIndex = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    AvroDebug::Log(L"CandidateWindow _EnsureWindow: hwnd=0x%p lastError=%lu", m_hwnd, GetLastError());
}

void CCandidateWindow::_Layout(const RECT &caretRect) {
    HDC hdc = GetDC(m_hwnd);
    HFONT hOldFont = (HFONT)SelectObject(hdc, m_hFontText);

    TEXTMETRICW tm;
    GetTextMetricsW(hdc, &tm);
    m_rowHeight = tm.tmHeight + kPaddingY;

    int maxTextWidth = 0;
    for (const auto &candidate : m_candidates) {
        SIZE sz = MeasureText(hdc, candidate);
        if (sz.cx > maxTextWidth) maxTextWidth = sz.cx;
    }

    SelectObject(hdc, hOldFont);
    ReleaseDC(m_hwnd, hdc);

    int width = kIndexColumnWidth + maxTextWidth + kPaddingX * 2;
    int height = (int)m_candidates.size() * m_rowHeight + kPaddingY;

    int x = caretRect.left;
    int y = caretRect.bottom + 4;

    HMONITOR hMon = MonitorFromRect(&caretRect, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoW(hMon, &mi);

    if (x + width > mi.rcWork.right) x = mi.rcWork.right - width;
    if (x < mi.rcWork.left) x = mi.rcWork.left;
    if (y + height > mi.rcWork.bottom) y = caretRect.top - height - 4;
    if (y < mi.rcWork.top) y = mi.rcWork.top;

    SetWindowPos(m_hwnd, HWND_TOPMOST, x, y, width, height, SWP_NOACTIVATE);

    HRGN rgn = CreateRoundRectRgn(0, 0, width, height, 8, 8);
    SetWindowRgn(m_hwnd, rgn, FALSE); // window now owns rgn; do not delete it
}

void CCandidateWindow::_Paint(HDC hdc) {
    RECT rc;
    GetClientRect(m_hwnd, &rc);

    bool dark = _IsDarkMode();
    COLORREF bg = dark ? RGB(43, 43, 46) : RGB(252, 252, 252);
    COLORREF border = dark ? RGB(70, 70, 74) : RGB(210, 210, 214);
    COLORREF textColor = dark ? RGB(240, 240, 240) : RGB(20, 20, 20);
    COLORREF indexColor = dark ? RGB(150, 150, 155) : RGB(130, 130, 135);
    COLORREF selBg = dark ? RGB(60, 90, 150) : RGB(210, 228, 252);

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

    HBRUSH bgBrush = CreateSolidBrush(bg);
    FillRect(memDC, &rc, bgBrush);
    DeleteObject(bgBrush);

    HPEN borderPen = CreatePen(PS_SOLID, 1, border);
    HPEN oldPen = (HPEN)SelectObject(memDC, borderPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, GetStockObject(NULL_BRUSH));
    Rectangle(memDC, 0, 0, rc.right, rc.bottom);
    SelectObject(memDC, oldBrush);
    SelectObject(memDC, oldPen);
    DeleteObject(borderPen);

    SetBkMode(memDC, TRANSPARENT);

    for (size_t i = 0; i < m_candidates.size(); ++i) {
        int top = (int)i * m_rowHeight + kPaddingY / 2;
        RECT rowRect = { 0, top, rc.right, top + m_rowHeight };

        if (i == m_selected) {
            HBRUSH selBrush = CreateSolidBrush(selBg);
            FillRect(memDC, &rowRect, selBrush);
            DeleteObject(selBrush);
        }

        wchar_t indexStr[4];
        swprintf_s(indexStr, L"%d", (int)(i + 1));

        HFONT oldF = (HFONT)SelectObject(memDC, m_hFontIndex);
        SetTextColor(memDC, indexColor);
        RECT idxRect = { kPaddingX, top, kPaddingX + kIndexColumnWidth, top + m_rowHeight };
        DrawTextW(memDC, indexStr, -1, &idxRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        SelectObject(memDC, m_hFontText);
        SetTextColor(memDC, textColor);
        TEXTMETRICW tm;
        GetTextMetricsW(memDC, &tm);
        int textY = top + (m_rowHeight - tm.tmHeight) / 2;
        DrawShapedText(memDC, m_candidates[i], kPaddingX + kIndexColumnWidth, textY);

        SelectObject(memDC, oldF);
    }

    BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
}

LRESULT CALLBACK CCandidateWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    CCandidateWindow *self;
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW *cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<CCandidateWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<CCandidateWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (msg == WM_PAINT && self) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        self->_Paint(hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    if (msg == WM_ERASEBKGND) {
        return 1; // _Paint always fills the whole client area; avoid the flicker
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void CCandidateWindow::Show(const RECT &caretRect, const std::vector<std::wstring> &candidates, size_t selected) {
    _EnsureWindow();
    m_candidates = candidates;
    m_selected = selected;
    _Layout(caretRect);
    ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
    InvalidateRect(m_hwnd, nullptr, FALSE);

    RECT wr = {};
    GetWindowRect(m_hwnd, &wr);
    AvroDebug::Log(L"Show: caretRect=(%d,%d,%d,%d) windowRect=(%d,%d,%d,%d) visible=%d",
                    caretRect.left, caretRect.top, caretRect.right, caretRect.bottom,
                    wr.left, wr.top, wr.right, wr.bottom, IsWindowVisible(m_hwnd));
}

void CCandidateWindow::UpdateSelection(size_t selected) {
    if (!m_hwnd) return;
    m_selected = selected;
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void CCandidateWindow::Hide() {
    if (m_hwnd) ShowWindow(m_hwnd, SW_HIDE);
}

bool CCandidateWindow::IsVisible() const {
    return m_hwnd && IsWindowVisible(m_hwnd);
}
