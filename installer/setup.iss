; Inno Setup script for Amader Bangla Keyboard.
; Builds a single Setup.exe that installs the TSF input method, registering the
; DLL(s) that match the PC's architecture (native via System32, 32-bit via
; SysWOW64). Suitable for direct download, winget, and Microsoft Store (Win32).
;
; Build:  ISCC.exe /DMyAppVersion=0.10.1 installer\setup.iss
; (run scripts\package.bat first so dist\ contains the DLLs + data files)

#ifndef MyAppVersion
  #define MyAppVersion "0.0.0"
#endif
#define MyAppName "Amader Bangla Keyboard"
#define MyAppPublisher "WindowsBanglaKeyboard"
#define MyAppUrl "https://github.com/amerganim/WindowsBanglaKeyboard"

[Setup]
AppId={{A3F1C2E4-5B6D-4E7F-9A0B-1C2D3E4F5A6B}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppUrl}
AppSupportURL={#MyAppUrl}
DefaultDirName={autopf}\BanglaPhonetic
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
DisableDirPage=yes
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\BanglaPhonetic_x64.dll
PrivilegesRequired=admin
ArchitecturesAllowed=x86compatible x64compatible arm64
ArchitecturesInstallIn64BitMode=x64compatible arm64
WizardStyle=modern
Compression=lzma2
SolidCompression=yes
; SourceDir is the repo root, so "dist\..." paths below resolve.
SourceDir={#SourcePath}\..
OutputDir=release
OutputBaseFilename=AmaderBanglaKeyboard-Setup-{#MyAppVersion}

[Files]
; Native DLL for the running architecture.
Source: "dist\BanglaPhonetic_x64.dll";   DestDir: "{app}"; Check: IsX64Win;   Flags: ignoreversion restartreplace uninsrestartdelete
Source: "dist\BanglaPhonetic_arm64.dll"; DestDir: "{app}"; Check: IsArm64Win; Flags: ignoreversion restartreplace uninsrestartdelete skipifsourcedoesntexist
; 32-bit DLL is installed everywhere (covers x86 apps on 64-bit/ARM64 Windows).
Source: "dist\BanglaPhonetic_x86.dll";   DestDir: "{app}"; Flags: ignoreversion restartreplace uninsrestartdelete
; Data + guide.
Source: "dist\dictionary.tsv"; DestDir: "{app}"; Flags: ignoreversion
Source: "dist\words.tsv";      DestDir: "{app}"; Flags: ignoreversion
Source: "dist\KEYMAP.html";    DestDir: "{app}"; Flags: ignoreversion
Source: "dist\LICENSE";        DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName} Typing Guide"; Filename: "{app}\KEYMAP.html"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"

[Run]
; Register with the matching-architecture regsvr32 (calls DllRegisterServer).
Filename: "{sys}\regsvr32.exe";      Parameters: "/s ""{app}\BanglaPhonetic_x64.dll""";   Check: IsX64Win;   Flags: runhidden waituntilterminated
Filename: "{sys}\regsvr32.exe";      Parameters: "/s ""{app}\BanglaPhonetic_arm64.dll""";  Check: Arm64DllReg; Flags: runhidden waituntilterminated
Filename: "{syswow64}\regsvr32.exe"; Parameters: "/s ""{app}\BanglaPhonetic_x86.dll""";    Check: Is64Win;    Flags: runhidden waituntilterminated
Filename: "{sys}\regsvr32.exe";      Parameters: "/s ""{app}\BanglaPhonetic_x86.dll""";    Check: IsX86Win;   Flags: runhidden waituntilterminated
; Offer to open the typing guide after a non-silent install.
Filename: "{app}\KEYMAP.html"; Description: "Open the typing guide"; Flags: postinstall shellexec skipifsilent nowait

[UninstallRun]
Filename: "{sys}\regsvr32.exe";      Parameters: "/u /s ""{app}\BanglaPhonetic_x64.dll""";   Check: IsX64Win;   RunOnceId: "UnregX64";   Flags: runhidden waituntilterminated
Filename: "{sys}\regsvr32.exe";      Parameters: "/u /s ""{app}\BanglaPhonetic_arm64.dll""";  Check: Arm64DllReg; RunOnceId: "UnregArm64"; Flags: runhidden waituntilterminated
Filename: "{syswow64}\regsvr32.exe"; Parameters: "/u /s ""{app}\BanglaPhonetic_x86.dll""";    Check: Is64Win;    RunOnceId: "UnregX86Wow"; Flags: runhidden waituntilterminated
Filename: "{sys}\regsvr32.exe";      Parameters: "/u /s ""{app}\BanglaPhonetic_x86.dll""";    Check: IsX86Win;   RunOnceId: "UnregX86";   Flags: runhidden waituntilterminated

[Code]
function IsArm64Win: Boolean;
begin
  Result := ProcessorArchitecture = paArm64;
end;

function IsX64Win: Boolean;
begin
  Result := ProcessorArchitecture = paX64;
end;

function Is64Win: Boolean;
begin
  Result := IsX64Win or IsArm64Win;
end;

function IsX86Win: Boolean;
begin
  Result := ProcessorArchitecture = paX86;
end;

// Register the ARM64 DLL only if it was actually packaged (ARM64 build present).
function Arm64DllReg: Boolean;
begin
  Result := IsArm64Win and FileExists(ExpandConstant('{app}\BanglaPhonetic_arm64.dll'));
end;
