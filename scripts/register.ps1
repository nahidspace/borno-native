#Requires -Version 5.1
# Registers AvroTSF.dll as a Windows TSF text service by calling its
# DllRegisterServer export directly (rather than through regsvr32, which
# swallows the real HRESULT/reason on failure behind a black-box exit code).
# This writes to HKEY_CLASSES_ROOT (machine-wide COM registration), so it
# needs an elevated console -- this script re-launches itself elevated
# (via a normal UAC prompt) if it isn't already running as Administrator.
# Usage: .\register.ps1 [Debug|Release]
$ErrorActionPreference = "Stop"

$config = if ($args.Count -gt 0) { $args[0] } else { "Debug" }
$dllPath = Join-Path $PSScriptRoot "..\Avro.TSF\x64\$config\AvroTSF.dll"
if (-not (Test-Path $dllPath)) { throw "Not built yet: $dllPath. Run build.ps1 first." }
$dll = (Resolve-Path $dllPath).Path

$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltinRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "Registering a TSF text service needs an elevated (Administrator) console. Re-launching with UAC..."
    Start-Process powershell -Verb RunAs -ArgumentList @("-NoExit", "-File", "`"$PSCommandPath`"", $config)
    exit
}

Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class AvroNative {
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern IntPtr LoadLibrary(string path);
    [DllImport("kernel32.dll")]
    public static extern IntPtr GetProcAddress(IntPtr hModule, string procName);
    [DllImport("kernel32.dll")]
    public static extern bool FreeLibrary(IntPtr hModule);
}
[UnmanagedFunctionPointer(CallingConvention.StdCall)]
public delegate int VoidToHResult();
"@

function Invoke-DllExport([string]$dllPath, [string]$exportName) {
    $hModule = [AvroNative]::LoadLibrary($dllPath)
    if ($hModule -eq [IntPtr]::Zero) {
        $err = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
        throw "LoadLibrary('$dllPath') failed, Win32 error $err"
    }
    try {
        $procAddr = [AvroNative]::GetProcAddress($hModule, $exportName)
        if ($procAddr -eq [IntPtr]::Zero) { throw "GetProcAddress('$exportName') failed" }
        $delegate = [Runtime.InteropServices.Marshal]::GetDelegateForFunctionPointer($procAddr, [VoidToHResult])
        return $delegate.Invoke()
    } finally {
        [AvroNative]::FreeLibrary($hModule) | Out-Null
    }
}

Write-Host "Registering $dll ..."
$hr = Invoke-DllExport $dll "DllRegisterServer"
if ($hr -ne 0) {
    $hrHex = "0x{0:X8}" -f $hr
    Write-Host "DllRegisterServer returned $hrHex" -ForegroundColor Red
    if ($hr -eq -2147024891) { Write-Host "(E_ACCESSDENIED -- registry write was denied even though this console should be elevated. Check the window title bar says 'Administrator'.)" }
    if ($hr -eq -2147467259) { Write-Host "(E_FAIL -- a registry/COM step failed; see Register.cpp for which step returns E_FAIL.)" }
    throw "Registration failed with HRESULT $hrHex"
}

Write-Host ""
Write-Host "Registered. To enable it:"
Write-Host "  Settings > Time & language > Language & region"
Write-Host "  Add a language (or open 'Bangla' if already added) > Language options > Add a keyboard"
Write-Host "  Pick 'Avro Phonetic', then switch to it with Win+Space and type into Notepad."
