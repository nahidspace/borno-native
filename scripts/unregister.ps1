#Requires -Version 5.1
# Unregisters BornoTSF.dll by calling its DllUnregisterServer export directly.
# Needs an elevated console, same as register.ps1.
# Usage: .\unregister.ps1 [Debug|Release]
$ErrorActionPreference = "Stop"

$config = if ($args.Count -gt 0) { $args[0] } else { "Debug" }
$dllPath = Join-Path $PSScriptRoot "..\Borno.TSF\x64\$config\BornoTSF.dll"
if (-not (Test-Path $dllPath)) { throw "Not found: $dllPath" }
$dll = (Resolve-Path $dllPath).Path

$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltinRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "Unregistering needs an elevated (Administrator) console. Re-launching with UAC..."
    Start-Process powershell -Verb RunAs -ArgumentList @("-NoExit", "-File", "`"$PSCommandPath`"", $config)
    exit
}

Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class BornoNative {
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
    $hModule = [BornoNative]::LoadLibrary($dllPath)
    if ($hModule -eq [IntPtr]::Zero) {
        $err = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
        throw "LoadLibrary('$dllPath') failed, Win32 error $err"
    }
    try {
        $procAddr = [BornoNative]::GetProcAddress($hModule, $exportName)
        if ($procAddr -eq [IntPtr]::Zero) { throw "GetProcAddress('$exportName') failed" }
        $delegate = [Runtime.InteropServices.Marshal]::GetDelegateForFunctionPointer($procAddr, [VoidToHResult])
        return $delegate.Invoke()
    } finally {
        [BornoNative]::FreeLibrary($hModule) | Out-Null
    }
}

Write-Host "Unregistering $dll ..."
$hr = Invoke-DllExport $dll "DllUnregisterServer"
if ($hr -ne 0) {
    $hrHex = "0x{0:X8}" -f $hr
    Write-Host "DllUnregisterServer returned $hrHex" -ForegroundColor Red
    throw "Unregistration failed with HRESULT $hrHex"
}

Write-Host "Unregistered. Remove 'Borno Native' from Settings > Time & language if it's still listed there."
