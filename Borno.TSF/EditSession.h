#pragma once
#include <windows.h>
#include <msctf.h>
#include <string>

// Replaces the last `replaceLength` UTF-16 units before the caret with `text`.
// Passing replaceLength == 0 is a plain insert; passing an empty `text` with
// replaceLength > 0 is a plain delete (used for backspace-during-composition).
class CEditSession : public ITfEditSession {
public:
    // pOutCaretRect, if non-null, is filled with the screen rect of the caret
    // after the replacement -- used to position the candidate popup. Valid
    // only because RequestEditSession(TF_ES_SYNC) completes inline, so the
    // caller's stack-local RECT is still alive when DoEditSession runs.
    CEditSession(ITfContext *pContext, const std::wstring &text, LONG replaceLength, RECT *pOutCaretRect = nullptr);

    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    STDMETHODIMP DoEditSession(TfEditCookie ec);

private:
    ~CEditSession();

    LONG m_cRef;
    ITfContext *m_pContext;
    std::wstring m_text;
    LONG m_replaceLength;
    RECT *m_pOutCaretRect;
};
