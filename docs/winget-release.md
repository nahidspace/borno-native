# WinGet release process

The GitHub Actions workflow creates a real x64 Inno Setup installer whenever
a `vMAJOR.MINOR.PATCH` tag is pushed. It also publishes the three WinGet YAML
manifests and the installer SHA-256 as release assets.

## Release a version

```powershell
git tag v0.1.0
git push origin v0.1.0
```

After the workflow completes, download the `winget` manifest folder from the
GitHub release and validate it with the WinGet manifest tooling. Submit that
folder as a pull request to `microsoft/winget-pkgs`; the public `winget`
source is populated from that community repository.

```powershell
winget validate --manifest .\dist\winget
```

Once Microsoft merges the manifest, users can install it with:

```powershell
winget install --id BornoNative.BornoNative --exact
```

The installer requests administrator privileges because TSF registration is
machine-wide. Close applications using the keyboard before upgrading so the
DLL is not locked.
