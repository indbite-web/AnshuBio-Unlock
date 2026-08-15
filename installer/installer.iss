; =====================================================================
; AnshuBio Unlock - Inno Setup Script
; Publisher: AnshuCore
; Application ID: com.anshucore.bio
; Target: Windows 10 & 11 (x64)
; =====================================================================

#define MyAppName "AnshuBio Unlock"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "AnshuCore"
#define MyAppURL "https://anshucore.com"
#define MyAppExeName "AnshuBioUnlock.exe"

[Setup]
AppId={{D37E6B91-5A2C-4E7B-9A3E-8D2F4C1B5E7A}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\AnshuBio Unlock
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=..\dist
OutputBaseFilename=AnshuBioUnlock-Setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "autostart"; Description: "Start AnshuBio Unlock automatically on Windows startup"; GroupDescription: "Startup:"

[Files]
Source: "..\build\bin\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\bin\AnshuBioUnlockService.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\bin\AnshuBioSessionMonitor.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\bin\AnshuBioCredentialProvider.dll"; DestDir: "{sys}"; Flags: restartreplace uninsrestartdelete
Source: "..\build\bin\*.dll"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\resources\*"; DestDir: "{app}\resources"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
; Windows Autostart Run Key (Silent Background Boot)
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "AnshuBioUnlock"; ValueData: """{app}\{#MyAppExeName}"" --background"; Flags: uninsdeletevalue; Tasks: autostart

; Register Credential Provider in HKLM
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\Credential Providers\{{B36E9B9A-5827-463F-8C37-67AB12E09B10}"; ValueType: string; ValueName: ""; ValueData: "AnshuBioCredentialProvider"; Flags: uninsdeletekey

; Register InprocServer32 COM Class in HKCR
Root: HKCR; Subkey: "CLSID\{{B36E9B9A-5827-463F-8C37-67AB12E09B10}"; ValueType: string; ValueName: ""; ValueData: "AnshuBioCredentialProvider"; Flags: uninsdeletekey
Root: HKCR; Subkey: "CLSID\{{B36E9B9A-5827-463F-8C37-67AB12E09B10}\InprocServer32"; ValueType: string; ValueName: ""; ValueData: "{sys}\AnshuBioCredentialProvider.dll"; Flags: uninsdeletekey
Root: HKCR; Subkey: "CLSID\{{B36E9B9A-5827-463F-8C37-67AB12E09B10}\InprocServer32"; ValueType: string; ValueName: "ThreadingModel"; ValueData: "Apartment"; Flags: uninsdeletevalue

[Run]
; Install Windows Background Service
Filename: "{app}\AnshuBioUnlockService.exe"; Parameters: "--install"; Flags: runhidden waituntilterminated

; Authorize Windows Firewall Rules
Filename: "netsh"; Parameters: "advfirewall firewall add rule name=""AnshuBio Unlock (Wi-Fi)"" dir=in action=allow protocol=TCP localport=42425"; Flags: runhidden
Filename: "netsh"; Parameters: "advfirewall firewall add rule name=""AnshuBio Unlock (Discovery)"" dir=in action=allow protocol=UDP localport=42424"; Flags: runhidden

; Launch Application post-install
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
; Stop and uninstall Windows Service
Filename: "{app}\AnshuBioUnlockService.exe"; Parameters: "--uninstall"; Flags: runhidden waituntilterminated

; Remove Windows Firewall Rules
Filename: "netsh"; Parameters: "advfirewall firewall delete rule name=""AnshuBio Unlock (Wi-Fi)"""; Flags: runhidden
Filename: "netsh"; Parameters: "advfirewall firewall delete rule name=""AnshuBio Unlock (Discovery)"""; Flags: runhidden
