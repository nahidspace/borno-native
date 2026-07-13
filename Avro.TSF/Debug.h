#pragma once

// Temporary file-based tracing for diagnosing the candidate window in a live
// host process we can't attach a debugger to. Appends to %TEMP%\AvroTSF-debug.log.
namespace AvroDebug {
    void Log(const wchar_t* fmt, ...);
}
