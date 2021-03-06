; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "SyncTERM"
#define MyAppVersion "1.1"
#define MyAppPublisher "SyncTERM"
#define MyAppURL "http://www.syncterm.net/"
#define MyAppExeName "syncterm.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{4039074B-5A5B-4B2C-888C-3F857234B773}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputBaseFilename=setup
Compression=lzma
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "systemlist"; Description: "Use the Official Synchronet BBS List as your System BBS List"; GroupDescription: "Options:";

[Files]
Source: "c:\bin\syncterm.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\bin\SDL2.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "c:\bin\fonts\*"; DestDir: "{app}\fonts"; Flags: ignoreversion
Source: "e:\src\syncterm\syncterm.ini"; DestDir: "{userappdata}\{#MyAppName}"; Flags: ignoreversion confirmoverwrite
Source: "s:\xfer\sbbs\syncterm.lst"; DestDir: "{commonappdata}\{#MyAppName}"; Tasks: systemlist; Flags: ignoreversion confirmoverwrite
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[INI]
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Antique (8x14 and 8x16)";     Key: "Path8x14";  String: "{app}\fonts\antique.f14"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Antique (8x14 and 8x16)";     Key: "Path8x16";  String: "{app}\fonts\antique.f16"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Bold (8x8, 8x14, and 8x16)";  Key: "Path8x8";   String: "{app}\fonts\bold.f8"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Bold (8x8, 8x14, and 8x16)";  Key: "Path8x14";  String: "{app}\fonts\bold.f14"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Bold (8x8, 8x14, and 8x16)";  Key: "Path8x16";  String: "{app}\fonts\bold.f16"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Broadway (8x16)";             Key: "Path8x16";  String: "{app}\fonts\broadway.f16"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Computer (8x14 and 8x16)";    Key: "Path8x14";  String: "{app}\fonts\computer.f14"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Computer (8x14 and 8x16)";    Key: "Path8x16";  String: "{app}\fonts\computer.f16"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Courier (8x14 and 8x16)";     Key: "Path8x14";  String: "{app}\fonts\courier.f14"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Courier (8x14 and 8x16)";     Key: "Path8x16";  String: "{app}\fonts\courier.f16"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Deco (8x16)";                 Key: "Path8x16";  String: "{app}\fonts\deco.f16"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Digital (8x16)";              Key: "Path8x16";  String: "{app}\fonts\digital.f16"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Frankfurt (8x14)";            Key: "Path8x14";  String: "{app}\fonts\frankfrt.f14"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Future (8x16)";               Key: "Path8x16";  String: "{app}\fonts\future.f16"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Georgian (8x14)";             Key: "Path8x14";  String: "{app}\fonts\georgian.f14"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Gothical (8x16)";             Key: "Path8x16";  String: "{app}\fonts\gothical.f16"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Hearst (8x14 and 8x16)";      Key: "Path8x14";  String: "{app}\fonts\hearst.f14"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Hearst (8x14 and 8x16)";      Key: "Path8x16";  String: "{app}\fonts\hearst.f16"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:ICE (8x16)";                  Key: "Path8x16";  String: "{app}\fonts\ice.f16"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Jasnew (8x16)";               Key: "Path8x16";  String: "{app}\fonts\jasnew.f16"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:LCD (8x16)";                  Key: "Path8x16";  String: "{app}\fonts\lcd.f16"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:LORE (8x16)";                 Key: "Path8x16";  String: "{app}\fonts\lorefont.f16"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Magic (8x16)";                Key: "Path8x16";  String: "{app}\fonts\magic.f16"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Medieval (8x16)";             Key: "Path8x16";  String: "{app}\fonts\medieval.f16"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Modern (8x16)";               Key: "Path8x16";  String: "{app}\fonts\modern.f16"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Old English (8x14)";          Key: "Path8x14";  String: "{app}\fonts\oldeng.f14"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Roman 1 (8x16)";              Key: "Path8x16";  String: "{app}\fonts\roman-1.f16"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Roman 2 (8x16)";              Key: "Path8x16";  String: "{app}\fonts\roman.f16"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Sans-serif (8x16)";           Key: "Path8x16";  String: "{app}\fonts\sansrif-.f16"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Scribble (8x16)";             Key: "Path8x16";  String: "{app}\fonts\scribble.f16"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Script (8x14 and 8x16)";      Key: "Path8x14";  String: "{app}\fonts\script.f14"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Script (8x14 and 8x16)";      Key: "Path8x16";  String: "{app}\fonts\script.f16"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Small Caps (8x14)";           Key: "Path8x14";  String: "{app}\fonts\smalcaps.f14"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Strange (8x14)";              Key: "Path8x14";  String: "{app}\fonts\strange.f14"
Filename: "{userappdata}\{#MyAppName}\syncterm.ini"; Section: "Font:Swiss (8x16)";                Key: "Path8x16";  String: "{app}\fonts\swiss-2.f16"

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Get Synchronet BBS List"; Filename: "{sys}\curl.exe"; Parameters: "http://synchro.net/syncterm.lst -o {commonappdata}\{#MyAppName}\syncterm.lst"; IconFilename: "{app}\{#MyAppExeName}"; Comment: "Get the latest Synchronet BBS List from Vertrauen";
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{group}\Check for updates"; Filename: "http://sourceforge.net/projects/syncterm/"; Comment: "Check for new SyncTERM versions";

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Messages]
StatusCreateIniEntries=Creating Font entries...
