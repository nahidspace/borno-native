#pragma once
#include <windows.h>
#include <unknwn.h>

class CClassFactory : public IClassFactory {
public:
    CClassFactory();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IClassFactory
    STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);
    STDMETHODIMP LockServer(BOOL fLock);

private:
    ~CClassFactory();
    LONG m_cRef;
};
