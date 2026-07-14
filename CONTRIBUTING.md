# Contributing

Borno Native's original source code and documentation are licensed under the
Apache License, Version 2.0. The bundled Bangla word-frequency resource has
separate CC BY-SA 4.0 terms; read `NOTICE` and
`Borno.TSF/Resources/ATTRIBUTION.md` before changing or redistributing it.

## Development setup

- Use Visual Studio 2022 or Build Tools with the v143 C++ toolset and the
  Windows 10 SDK.
- Build with `scripts/build.ps1 Debug` or `scripts/build.ps1 Release`.
- Build the console regression harness from `Borno.Tests/BornoTests.vcxproj`.
- Run the harness against `tools/oracle/wordlist.txt` and compare its UTF-8
  output with `tools/oracle/ours.tsv`.
- Test TSF changes in a fresh Notepad process. Registration is machine-wide
  and requires an elevated PowerShell session.

## Change guidelines

- Keep the phonetic engine independently implemented; do not copy GPL code or
  data from the oracle into `Borno.TSF`.
- Add a focused regression input whenever a conversion rule changes.
- Preserve UTF-8 source/resource encoding and use `/utf-8` when compiling.
- Keep TSF edit sessions synchronous, bounded to the active composition, and
  safe when the host selection or context is unavailable.
- Do not commit generated `x64` build output, debug logs, installed virtual
  environments, or machine-specific registry exports.
- Update attribution and `NOTICE` when adding a third-party asset.

## Pull requests

Describe the behavior change, include the regression case and verification
command, and call out any host-app or registration requirements. Contributions
submitted for inclusion are accepted under Apache-2.0 unless explicitly
marked otherwise.
