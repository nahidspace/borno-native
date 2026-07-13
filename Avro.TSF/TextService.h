#pragma once
#include <windows.h>
#include <msctf.h>
#include <string>
#include <vector>
#include "CandidateWindow.h"

class CTextService : public ITfTextInputProcessor, public ITfKeyEventSink {
public:
    CTextService();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // ITfTextInputProcessor
    STDMETHODIMP Activate(ITfThreadMgr *pThreadMgr, TfClientId tfClientId);
    STDMETHODIMP Deactivate();

    // ITfKeyEventSink
    STDMETHODIMP OnSetFocus(BOOL fForeground);
    STDMETHODIMP OnTestKeyDown(ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnKeyDown(ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnTestKeyUp(ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnKeyUp(ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnPreservedKey(ITfContext *pic, REFGUID rguid, BOOL *pfEaten);

private:
    ~CTextService();

    // Depends only on wParam and current buffer/candidate-window state, so
    // OnTestKeyDown and OnKeyDown (called back-to-back for the same physical
    // key, with no mutation between them) always agree on whether it's eaten.
    BOOL _IsComposableKey(WPARAM wParam) const;

    void _ShowSelectedCandidate(ITfContext *pic);
    void _CommitCandidate(ITfContext *pic, size_t index);
    void _ReplaceComposition(ITfContext *pic, const std::wstring &text, RECT *outCaretRect = nullptr);
    void _EndComposition();

    LONG m_cRef;
    ITfThreadMgr *m_pThreadMgr;
    TfClientId m_tfClientId;

    std::wstring m_rawBuffer;  // raw ASCII typed so far for the current word
    LONG m_insertedLength;     // UTF-16 length of the Bangla currently shown for it

    std::vector<std::wstring> m_candidates;
    size_t m_selectedCandidate;
    CCandidateWindow m_candidateWindow;
};
