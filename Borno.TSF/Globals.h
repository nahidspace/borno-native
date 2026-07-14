#pragma once
#include <windows.h>

extern const CLSID CLSID_BornoTextService;
extern const GUID  GUID_Profile_BornoNative;

extern HINSTANCE g_hInst;
extern LONG g_cRefDll;

#define BORNO_TEXTSERVICE_DESC L"Borno Native"
#define BORNO_LANGID MAKELANGID(LANG_BENGALI, SUBLANG_BENGALI_BANGLADESH)
