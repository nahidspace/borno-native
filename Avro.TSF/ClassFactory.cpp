#include "ClassFactory.h"
#include "TextService.h"
#include "Globals.h"
#include <new>

CClassFactory::CClassFactory() : m_cRef(1) {
    InterlockedIncrement(&g_cRefDll);
}

CClassFactory::~CClassFactory() {
    InterlockedDecrement(&g_cRefDll);
}

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void **ppvObj) {
    if (!ppvObj) return E_INVALIDARG;
    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory)) {
        *ppvObj = static_cast<IClassFactory*>(this);
    } else {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef() {
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CClassFactory::Release() {
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) delete this;
    return cRef;
}

STDMETHODIMP CClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj) {
    if (!ppvObj) return E_INVALIDARG;
    *ppvObj = nullptr;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    CTextService *pTextService = new (std::nothrow) CTextService();
    if (!pTextService) return E_OUTOFMEMORY;

    HRESULT hr = pTextService->QueryInterface(riid, ppvObj);
    pTextService->Release();
    return hr;
}

STDMETHODIMP CClassFactory::LockServer(BOOL fLock) {
    if (fLock) InterlockedIncrement(&g_cRefDll);
    else InterlockedDecrement(&g_cRefDll);
    return S_OK;
}
