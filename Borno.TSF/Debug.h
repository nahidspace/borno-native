#pragma once

// Temporary file-based tracing for diagnosing the candidate window in a live
// host process we can't attach a debugger to. Appends to %TEMP%\BornoTSF-debug.log.
namespace BornoDebug {
    void Log(const wchar_t* fmt, ...);
}
