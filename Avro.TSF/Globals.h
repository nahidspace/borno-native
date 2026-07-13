#pragma once
#include <windows.h>

extern const CLSID CLSID_AvroTextService;
extern const GUID  GUID_Profile_AvroPhonetic;

extern HINSTANCE g_hInst;
extern LONG g_cRefDll;

#define AVRO_TEXTSERVICE_DESC L"Avro Phonetic"
#define AVRO_LANGID MAKELANGID(LANG_BENGALI, SUBLANG_BENGALI_BANGLADESH)
