#define AppName "Borno Native"
#define AppVersion GetEnv("APP_VERSION")
#if AppVersion == ""
  #define AppVersion "0.1.0"
#endif
#define AppPublisher "Borno Native contributors"
#define AppURL "https://github.com/nahidspace/borno-native"

[Setup]
AppId={{C2FD6C0D-7A5D-49AF-B6F0-1E28B2A2D4B5}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#AppURL}/releases
DefaultDirName={autopf}\Borno Native
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\dist
OutputBaseFilename=BornoNative-{#AppVersion}-x64
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
LicenseFile=..\LICENSE
UninstallDisplayIcon={app}\AvroTSF.dll
ChangesEnvironment=no

[Files]
Source: "..\Avro.TSF\x64\Release\AvroTSF.dll"; DestDir: "{app}"; Flags: ignoreversion restartreplace
Source: "..\LICENSE"; DestDir: "{app}"; Flags: isreadme
Source: "..\NOTICE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Avro.TSF\Resources\ATTRIBUTION.md"; DestDir: "{app}"; Flags: ignoreversion

[Run]
Filename: "{sys}\regsvr32.exe"; Parameters: "/s \"{app}\AvroTSF.dll\""; StatusMsg: "Registering the Borno Native keyboard..."; Flags: runhidden waituntilterminated

[UninstallRun]
Filename: "{sys}\regsvr32.exe"; Parameters: "/s /u \"{app}\AvroTSF.dll\""; RunOnceId: "UnregisterBornoNative"; Flags: runhidden waituntilterminated

[UninstallDelete]
Type: files; Name: "{app}\AvroTSF.dll"
