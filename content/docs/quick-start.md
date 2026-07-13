---
title: Quick start
weight: 1
lead: Build the keyboard, register it with Windows, and type your first Bangla word.
---

## Requirements

- Windows 10 or later
- Visual Studio 2022 or Build Tools with the v143 C++ toolset
- Windows 10 SDK

## Build

From the repository root, open PowerShell and run:

```powershell
.\scripts\build.ps1 Debug
```

For an optimized build, use `Release` instead.

## Register

Registration is machine-wide and requires an elevated PowerShell session:

```powershell
.\scripts\register.ps1 Debug
```

Open **Settings → Time & language → Language & region → Bangla → Language options**, add **Avro Phonetic**, and select it from the language indicator. Restart the target app after rebuilding so it unloads any older DLL.

## Try it

Open Notepad and type:

```text
ami       → আমি
bangla    → বাংলা
kShomota  → ক্ষমতা
brO       → ব্রো
```

Remove the registration with `.\scripts\unregister.ps1 Debug`.
