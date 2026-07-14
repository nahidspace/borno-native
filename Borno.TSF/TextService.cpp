#include "TextService.h"
#include "EditSession.h"
#include "Convert.h"
#include "Globals.h"
#include "Debug.h"
#include <new>

CTextService::CTextService()
    : m_cRef(1), m_pThreadMgr(nullptr), m_tfClientId(TF_CLIENTID_NULL), m_insertedLength(0),
      m_selectedCandidate(0) {
    InterlockedIncrement(&g_cRefDll);
}

CTextService::~CTextService() {
    InterlockedDecrement(&g_cRefDll);
}

STDMETHODIMP CTextService::QueryInterface(REFIID riid, void **ppvObj) {
    if (!ppvObj) return E_INVALIDARG;
    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfTextInputProcessor))
        *ppvObj = static_cast<ITfTextInputProcessor*>(this);
    else if (IsEqualIID(riid, IID_ITfKeyEventSink))
        *ppvObj = static_cast<ITfKeyEventSink*>(this);

    if (!*ppvObj) return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CTextService::AddRef() {
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CTextService::Release() {
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) delete this;
    return cRef;
}

STDMETHODIMP CTextService::Activate(ITfThreadMgr *pThreadMgr, TfClientId tfClientId) {
    m_pThreadMgr = pThreadMgr;
    m_pThreadMgr->AddRef();
    m_tfClientId = tfClientId;

    ITfKeystrokeMgr *pKeystrokeMgr = nullptr;
    if (SUCCEEDED(m_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void**)&pKeystrokeMgr))) {
        pKeystrokeMgr->AdviseKeyEventSink(m_tfClientId, this, TRUE);
        pKeystrokeMgr->Release();
    }
    return S_OK;
}

STDMETHODIMP CTextService::Deactivate() {
    ITfKeystrokeMgr *pKeystrokeMgr = nullptr;
    if (m_pThreadMgr && SUCCEEDED(m_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void**)&pKeystrokeMgr))) {
        pKeystrokeMgr->UnadviseKeyEventSink(m_tfClientId);
        pKeystrokeMgr->Release();
    }

    if (m_pThreadMgr) {
        m_pThreadMgr->Release();
        m_pThreadMgr = nullptr;
    }
    m_tfClientId = TF_CLIENTID_NULL;
    m_candidateWindow.Hide();
    return S_OK;
}

STDMETHODIMP CTextService::OnSetFocus(BOOL fForeground) {
    if (!fForeground) _EndComposition();
    return S_OK;
}

BOOL CTextService::_IsComposableKey(WPARAM wParam) const {
    if (wParam >= 'A' && wParam <= 'Z') return TRUE;
    if (wParam == VK_BACK) return !m_rawBuffer.empty();
    if (m_candidateWindow.IsVisible()) {
        if (wParam == VK_UP || wParam == VK_DOWN || wParam == VK_RETURN ||
            wParam == VK_TAB || wParam == VK_ESCAPE) return TRUE;
        if (wParam >= '1' && wParam <= '9') return TRUE;
    }
    return FALSE;
}

namespace {
bool IsModifierKey(WPARAM wParam) {
    return wParam == VK_SHIFT || wParam == VK_LSHIFT || wParam == VK_RSHIFT ||
           wParam == VK_CONTROL || wParam == VK_LCONTROL || wParam == VK_RCONTROL ||
           wParam == VK_MENU || wParam == VK_LMENU || wParam == VK_RMENU ||
           wParam == VK_LWIN || wParam == VK_RWIN;
}
}

void CTextService::_ShowSelectedCandidate(ITfContext *pic) {
    const std::wstring &text = m_candidates[m_selectedCandidate];
    BornoDebug::Log(L"_ShowSelectedCandidate buffer='%s' candidateCount=%zu chosen='%s'",
                    m_rawBuffer.c_str(), m_candidates.size(), text.c_str());

    RECT caretRect = {};
    _ReplaceComposition(pic, text, &caretRect);
    m_insertedLength = (LONG)text.length();

    if (m_candidates.size() > 1) {
        BornoDebug::Log(L"Calling CandidateWindow.Show with %zu candidates", m_candidates.size());
        m_candidateWindow.Show(caretRect, m_candidates, m_selectedCandidate);
    } else {
        BornoDebug::Log(L"Only 1 candidate, hiding window");
        m_candidateWindow.Hide();
    }
}

void CTextService::_ReplaceComposition(ITfContext *pic, const std::wstring &text, RECT *outCaretRect) {
    RECT caretRect = {};
    CEditSession *pSession = new (std::nothrow) CEditSession(
        pic, text, m_insertedLength, &caretRect);
    if (!pSession) return;

    HRESULT hrSession = S_OK;
    HRESULT hrRequest = pic->RequestEditSession(
        m_tfClientId, pSession, TF_ES_SYNC | TF_ES_READWRITE, &hrSession);
    pSession->Release();
    BornoDebug::Log(L"ReplaceComposition hrRequest=0x%08X hrSession=0x%08X caretRect=(%d,%d,%d,%d)",
                   (unsigned)hrRequest, (unsigned)hrSession,
                   caretRect.left, caretRect.top, caretRect.right, caretRect.bottom);

    if (SUCCEEDED(hrRequest) && SUCCEEDED(hrSession))
        m_insertedLength = (LONG)text.length();
    if (outCaretRect) *outCaretRect = caretRect;
}

void CTextService::_CommitCandidate(ITfContext *pic, size_t index) {
    const std::wstring &text = m_candidates[index];

    CEditSession *pSession = new (std::nothrow) CEditSession(pic, text, m_insertedLength);
    if (pSession) {
        HRESULT hrSession = S_OK;
        pic->RequestEditSession(m_tfClientId, pSession, TF_ES_SYNC | TF_ES_READWRITE, &hrSession);
        pSession->Release();
    }

    _EndComposition();
}

void CTextService::_EndComposition() {
    m_rawBuffer.clear();
    m_insertedLength = 0;
    m_candidates.clear();
    m_selectedCandidate = 0;
    m_candidateWindow.Hide();
}

STDMETHODIMP CTextService::OnTestKeyDown(ITfContext *, WPARAM wParam, LPARAM, BOOL *pfEaten) {
    if (!pfEaten) return E_INVALIDARG;
    *pfEaten = _IsComposableKey(wParam);
    return S_OK;
}

STDMETHODIMP CTextService::OnTestKeyUp(ITfContext *, WPARAM wParam, LPARAM, BOOL *pfEaten) {
    if (!pfEaten) return E_INVALIDARG;
    *pfEaten = _IsComposableKey(wParam);
    return S_OK;
}

STDMETHODIMP CTextService::OnKeyUp(ITfContext *, WPARAM, LPARAM, BOOL *pfEaten) {
    if (pfEaten) *pfEaten = FALSE;
    return S_OK;
}

STDMETHODIMP CTextService::OnPreservedKey(ITfContext *, REFGUID, BOOL *pfEaten) {
    if (pfEaten) *pfEaten = FALSE;
    return S_OK;
}

STDMETHODIMP CTextService::OnKeyDown(ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten) {
    if (!pfEaten) return E_INVALIDARG;

    BOOL composable = _IsComposableKey(wParam);
    *pfEaten = composable;

    if (!composable) {
        // Modifier keys arrive as separate key events before the letter they
        // modify. They must not terminate the active phonetic buffer, or
        // typing Shift+O turns "brO" into "br" followed by "O".
        if (IsModifierKey(wParam)) return S_OK;

        // Any other key we don't handle ends the current word.
        _EndComposition();
        return S_OK;
    }

    bool candidateKey = m_candidateWindow.IsVisible() &&
        (wParam == VK_UP || wParam == VK_DOWN || wParam == VK_RETURN ||
         wParam == VK_TAB || wParam == VK_ESCAPE || (wParam >= '1' && wParam <= '9'));

    if (candidateKey) {
        switch (wParam) {
        case VK_UP:
            m_selectedCandidate = (m_selectedCandidate == 0)
                ? m_candidates.size() - 1 : m_selectedCandidate - 1;
            _ShowSelectedCandidate(pic);
            break;
        case VK_DOWN:
            m_selectedCandidate = (m_selectedCandidate + 1) % m_candidates.size();
            _ShowSelectedCandidate(pic);
            break;
        case VK_RETURN:
        case VK_TAB:
            _CommitCandidate(pic, m_selectedCandidate);
            break;
        case VK_ESCAPE:
            _EndComposition();
            break;
        default: { // '1'-'9'
            size_t idx = (size_t)(wParam - '1');
            if (idx < m_candidates.size()) _CommitCandidate(pic, idx);
            break;
        }
        }
        return S_OK;
    }

    if (wParam == VK_BACK) {
        m_rawBuffer.pop_back();
    } else {
        // The TSF is a phonetic keyboard: capture the virtual key itself,
        // rather than asking the currently active layout to translate it.
        // ToUnicodeEx can otherwise return Bengali (or another locale's
        // character), which cannot be fed back through the ASCII converter.
        if (wParam < 'A' || wParam > 'Z') {
            *pfEaten = FALSE;
            return S_OK;
        }
        bool shifted = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        m_rawBuffer.push_back((wchar_t)(shifted ? wParam : (wParam + ('a' - 'A'))));
    }

    if (m_rawBuffer.empty()) {
        // The final backspace must remove the composition already present in
        // the document. Clearing only the buffer leaves a stale Bangla glyph
        // behind and makes the next word start after invisible state.
        if (m_insertedLength > 0)
            _ReplaceComposition(pic, L"");
        _EndComposition();
        return S_OK;
    }

    m_candidates = BornoCore::GetCandidates(m_rawBuffer);
    m_selectedCandidate = 0;
    _ShowSelectedCandidate(pic);
    return S_OK;
}
