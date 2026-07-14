#include "Register.h"
#include "Globals.h"
#include <msctf.h>
#include <objbase.h>
#include <shlwapi.h>
#include <string>

#pragma comment(lib, "shlwapi.lib")

namespace {

std::wstring GuidToString(REFGUID guid) {
    WCHAR buf[64] = {};
    StringFromGUID2(guid, buf, 64);
    return buf;
}

HRESULT RegisterClsid() {
    WCHAR modulePath[MAX_PATH] = {};
    GetModuleFileNameW(g_hInst, modulePath, MAX_PATH);

    std::wstring keyPath = L"CLSID\\" + GuidToString(CLSID_BornoTextService);

    HKEY hKey = nullptr;
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, keyPath.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS)
        return E_FAIL;
    RegSetValueExW(hKey, nullptr, 0, REG_SZ, (const BYTE*)BORNO_TEXTSERVICE_DESC,
                   (DWORD)(wcslen(BORNO_TEXTSERVICE_DESC) + 1) * sizeof(WCHAR));
    RegCloseKey(hKey);

    std::wstring inprocPath = keyPath + L"\\InprocServer32";
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, inprocPath.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS)
        return E_FAIL;
    RegSetValueExW(hKey, nullptr, 0, REG_SZ, (const BYTE*)modulePath, (DWORD)(wcslen(modulePath) + 1) * sizeof(WCHAR));
    RegSetValueExW(hKey, L"ThreadingModel", 0, REG_SZ, (const BYTE*)L"Apartment",
                   (DWORD)(wcslen(L"Apartment") + 1) * sizeof(WCHAR));
    RegCloseKey(hKey);

    return S_OK;
}

void UnregisterClsid() {
    std::wstring keyPath = L"CLSID\\" + GuidToString(CLSID_BornoTextService);
    SHDeleteKeyW(HKEY_CLASSES_ROOT, keyPath.c_str());
}

} // namespace

namespace BornoRegister {

HRESULT RegisterServer() {
    HRESULT hr = RegisterClsid();
    if (FAILED(hr)) return hr;

    WCHAR modulePath[MAX_PATH] = {};
    GetModuleFileNameW(g_hInst, modulePath, MAX_PATH);

    HRESULT hrCoInit = CoInitialize(nullptr);

    ITfInputProcessorProfiles *pProfiles = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_ITfInputProcessorProfiles, (void**)&pProfiles))) {
        pProfiles->Register(CLSID_BornoTextService);
        pProfiles->AddLanguageProfile(CLSID_BornoTextService, BORNO_LANGID, GUID_Profile_BornoNative,
                                       BORNO_TEXTSERVICE_DESC, (ULONG)wcslen(BORNO_TEXTSERVICE_DESC),
                                       modulePath, (ULONG)wcslen(modulePath), 0);
        pProfiles->Release();
    }

    ITfCategoryMgr *pCategoryMgr = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_ITfCategoryMgr, (void**)&pCategoryMgr))) {
        pCategoryMgr->RegisterCategory(CLSID_BornoTextService, GUID_TFCAT_TIP_KEYBOARD, CLSID_BornoTextService);
        pCategoryMgr->Release();
    }

    if (SUCCEEDED(hrCoInit)) CoUninitialize();
    return S_OK;
}

HRESULT UnregisterServer() {
    HRESULT hrCoInit = CoInitialize(nullptr);

    ITfCategoryMgr *pCategoryMgr = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_ITfCategoryMgr, (void**)&pCategoryMgr))) {
        pCategoryMgr->UnregisterCategory(CLSID_BornoTextService, GUID_TFCAT_TIP_KEYBOARD, CLSID_BornoTextService);
        pCategoryMgr->Release();
    }

    ITfInputProcessorProfiles *pProfiles = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_ITfInputProcessorProfiles, (void**)&pProfiles))) {
        pProfiles->RemoveLanguageProfile(CLSID_BornoTextService, BORNO_LANGID, GUID_Profile_BornoNative);
        pProfiles->Unregister(CLSID_BornoTextService);
        pProfiles->Release();
    }

    if (SUCCEEDED(hrCoInit)) CoUninitialize();

    UnregisterClsid();
    return S_OK;
}

} // namespace BornoRegister
