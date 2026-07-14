#Requires -Version 5.1
# Builds Borno.TSF. Usage: .\build.ps1 [Debug|Release]
$ErrorActionPreference = "Stop"

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$msbuild = & $vswhere -latest -products * -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
if (-not $msbuild) { throw "MSBuild not found. Is Visual Studio / Build Tools installed?" }

$proj = Join-Path $PSScriptRoot "..\Borno.TSF\BornoTSF.vcxproj"
$config = if ($args.Count -gt 0) { $args[0] } else { "Debug" }

& $msbuild $proj /p:Configuration=$config /p:Platform=x64 /nologo /v:minimal
if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE" }

Write-Host "Built: $(Resolve-Path (Join-Path $PSScriptRoot "..\Borno.TSF\x64\$config\BornoTSF.dll"))"
