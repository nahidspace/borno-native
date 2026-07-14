#include "EditSession.h"
#include "Debug.h"

CEditSession::CEditSession(ITfContext *pContext, const std::wstring &text, LONG replaceLength, RECT *pOutCaretRect)
    : m_cRef(1), m_pContext(pContext), m_text(text), m_replaceLength(replaceLength), m_pOutCaretRect(pOutCaretRect) {
    m_pContext->AddRef();
}

CEditSession::~CEditSession() {
    m_pContext->Release();
}

STDMETHODIMP CEditSession::QueryInterface(REFIID riid, void **ppvObj) {
    if (!ppvObj) return E_INVALIDARG;
    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfEditSession))
        *ppvObj = static_cast<ITfEditSession*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CEditSession::AddRef() {
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CEditSession::Release() {
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) delete this;
    return cRef;
}

STDMETHODIMP CEditSession::DoEditSession(TfEditCookie ec) {
    TF_SELECTION sel;
    ULONG fetched = 0;
    if (FAILED(m_pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &sel, &fetched)) || fetched != 1)
        return S_OK;

    ITfRange *pRange = sel.range;

    pRange->Collapse(ec, TF_ANCHOR_END);
    if (m_replaceLength > 0) {
        LONG shifted = 0;
        pRange->ShiftStart(ec, -m_replaceLength, &shifted, nullptr);
    }

    pRange->SetText(ec, 0, m_text.c_str(), (LONG)m_text.length());
    pRange->Collapse(ec, TF_ANCHOR_END);

    TF_SELECTION newSel;
    newSel.range = pRange;
    newSel.style.ase = TF_AE_NONE;
    newSel.style.fInterimChar = FALSE;
    m_pContext->SetSelection(ec, 1, &newSel);

    if (m_pOutCaretRect) {
        ITfContextView *pView = nullptr;
        HRESULT hrView = m_pContext->GetActiveView(&pView);
        if (SUCCEEDED(hrView)) {
            BOOL fClipped = FALSE;
            HRESULT hrExt = pView->GetTextExt(ec, pRange, m_pOutCaretRect, &fClipped);
            BornoDebug::Log(L"GetTextExt hr=0x%08X rect=(%d,%d,%d,%d) clipped=%d",
                            (unsigned)hrExt, m_pOutCaretRect->left, m_pOutCaretRect->top,
                            m_pOutCaretRect->right, m_pOutCaretRect->bottom, fClipped);
            pView->Release();
        } else {
            BornoDebug::Log(L"GetActiveView FAILED hr=0x%08X", (unsigned)hrView);
        }
    }

    pRange->Release();
    return S_OK;
}
