#include "Debug.h"
#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include <string>

namespace AvroDebug {

void Log(const wchar_t* fmt, ...) {
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    std::wstring logPath = std::wstring(tempPath) + L"AvroTSF-debug.log";

    FILE* f = nullptr;
    if (_wfopen_s(&f, logPath.c_str(), L"a, ccs=UTF-8") != 0 || !f) return;

    wchar_t buf[1024];
    va_list args;
    va_start(args, fmt);
    vswprintf_s(buf, fmt, args);
    va_end(args);

    SYSTEMTIME st;
    GetLocalTime(&st);
    fwprintf(f, L"[%02d:%02d:%02d.%03d pid=%lu] %s\n",
              st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, GetCurrentProcessId(), buf);
    fclose(f);
}

} // namespace AvroDebug
