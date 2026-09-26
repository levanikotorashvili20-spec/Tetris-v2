; Inno Setup script for Tetris v2 (C version)
; Compile with Inno Setup 6: open this file and press Build -> Compile (Ctrl+F9)
; The installer appears in the "installer" folder.
; Build the Release | x86 configuration in Visual Studio first.

#define AppName    "Tetris v2"
#define AppVersion "1.0"
#define AppExe     "Tetris v2.exe"

[Setup]
AppId={{142BA6F2-E2FB-460D-81C7-93214828D874}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=YOUR NAME
; installs into the user's own folder: no admin rights needed,
; and the game can write its save files next to the exe
DefaultDirName={localappdata}\Programs\{#AppName}
PrivilegesRequired=lowest
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
LicenseFile=LICENSE
SetupIconFile=src\tetris.ico
UninstallDisplayIcon={app}\{#AppExe}
OutputDir=installer
OutputBaseFilename=Tetris-v2-Setup-{#AppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "Release\{#AppExe}"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExe}"; WorkingDir: "{app}"
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchProgram,{#AppName}}"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; save files created by the game
Type: files; Name: "{app}\*.sav"
