; Inno Setup script for Folder Manager.
; Build the application first with: mingw32-make

#define MyAppName "Folder Manager"
#define MyAppNameZh "文件目录生成器"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Folder Manager Project"
#define MyBuildExeName "FolderManagerImGui.exe"
#define MyAppExeName "FolderManager.exe"
#define MyProjectRoot AddBackslash(SourcePath) + "..\"

[Setup]
AppId={{E92CE892-770A-4890-9D6F-F0A3DEBC041F}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
ChangesAssociations=yes
DisableDirPage=no
DisableProgramGroupPage=yes
AlwaysShowDirOnReadyPage=yes
LicenseFile={#MyProjectRoot}LICENSE
OutputDir={#MyProjectRoot}installer
OutputBaseFilename=FolderManagerSetup
SetupIconFile={#MyProjectRoot}resources\icon.ico
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "chinesesimplified"; MessagesFile: "{#MyProjectRoot}installer\ChineseSimplified.isl"

[CustomMessages]
english.AppShortcutName={#MyAppName}
chinesesimplified.AppShortcutName={#MyAppNameZh}
english.ContextMenuTask=Add to File Explorer context menu
chinesesimplified.ContextMenuTask=添加到资源管理器右键菜单
english.ContextMenuGroup=Integration:
chinesesimplified.ContextMenuGroup=集成选项:
english.ContextMenuText=Open with Folder Manager
chinesesimplified.ContextMenuText=使用文件目录生成器打开
english.LaunchApp=Launch Folder Manager
chinesesimplified.LaunchApp=启动文件目录生成器

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "contextmenu"; Description: "{cm:ContextMenuTask}"; GroupDescription: "{cm:ContextMenuGroup}"; Flags: unchecked

[Files]
Source: "{#MyProjectRoot}build\bin\{#MyBuildExeName}"; DestDir: "{app}"; DestName: "{#MyAppExeName}"; Flags: ignoreversion
; NOTE: The current Makefile links libgcc and libstdc++ statically. Add extra runtime DLLs here only if your build requires them.

[Registry]
Root: HKCR; Subkey: "Directory\Background\shell\FolderManager"; ValueType: string; ValueName: ""; ValueData: "{cm:ContextMenuText}"; Flags: uninsdeletekey; Tasks: contextmenu
Root: HKCR; Subkey: "Directory\Background\shell\FolderManager"; ValueType: string; ValueName: "Icon"; ValueData: """{app}\{#MyAppExeName}"",0"; Flags: uninsdeletevalue; Tasks: contextmenu
Root: HKCR; Subkey: "Directory\Background\shell\FolderManager\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%V"""; Flags: uninsdeletekey; Tasks: contextmenu

Root: HKCR; Subkey: "Directory\shell\FolderManager"; ValueType: string; ValueName: ""; ValueData: "{cm:ContextMenuText}"; Flags: uninsdeletekey; Tasks: contextmenu
Root: HKCR; Subkey: "Directory\shell\FolderManager"; ValueType: string; ValueName: "Icon"; ValueData: """{app}\{#MyAppExeName}"",0"; Flags: uninsdeletevalue; Tasks: contextmenu
Root: HKCR; Subkey: "Directory\shell\FolderManager\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: contextmenu

[Icons]
Name: "{autoprograms}\{cm:AppShortcutName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{cm:AppShortcutName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchApp}"; Flags: nowait postinstall skipifsilent
