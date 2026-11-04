#define ProductVersion "0.1.0"
[Setup]
AppId={{6ADDA1E6-7537-4288-8DC9-435BE8A0832A}
AppName=NimBox 1 USB audio device
AppVersion={#ProductVersion}
AppPublisher=NimBox project
DefaultDirName={autopf}\NimBox 1
DefaultGroupName=NimBox 1
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
MinVersion=10.0.22000
PrivilegesRequired=admin
LicenseFile=..\COPYING
InfoBeforeFile=INSTALL-NOTES.txt
OutputDir=..\..\dist
OutputBaseFilename=NimBox-1-ASIO-{#ProductVersion}-x64-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=NimBox 1 USB audio device (ASIO)
CloseApplications=yes
RestartApplications=no
SetupLogging=yes

[Files]
Source: "..\build\asio\NimBoxAsio.dll"; DestDir: "{app}"; Flags: ignoreversion regserver 64bit
Source: "..\build\asio\UsbDkHelper.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\asio\NimBoxAsioProbe.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\asio\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\asio\THIRD-PARTY-NOTICES.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\COPYING"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\third_party\usbdk\LICENSE"; DestDir: "{app}"; DestName: "UsbDk-LICENSE.txt"; Flags: ignoreversion
Source: "..\..\dist\NimBox-1-ASIO-{#ProductVersion}-source.zip"; DestDir: "{app}\Source"; Flags: ignoreversion
Source: "..\..\downloads\UsbDk_1.0.22_x64.msi"; Flags: dontcopy

[Icons]
Name: "{group}\Read me"; Filename: "{app}\README.md"
Name: "{group}\Uninstall NimBox"; Filename: "{uninstallexe}"

[Code]
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  Code: Integer;
  Msi, Hash, Version: String;
begin
  Result := '';
  if RegKeyExists(HKLM, 'SYSTEM\CurrentControlSet\Services\UsbDk') then begin
    if (not GetVersionNumbersString(ExpandConstant('{sys}\drivers\UsbDk.sys'), Version)) or (Version <> '1.0.22.0') then
      Result := 'NimBox requires UsbDk 1.0.22. Repair or update the existing shared UsbDk runtime before installing NimBox.';
    exit;
  end;
  ExtractTemporaryFile('UsbDk_1.0.22_x64.msi');
  Msi := ExpandConstant('{tmp}\UsbDk_1.0.22_x64.msi');
  Hash := GetSHA256OfFile(Msi);
  if CompareText(Hash, '91f6f695e1e13c656024e6d3b55620bf08d8835ef05ee0496935ba6bb62466a5') <> 0 then begin
    Result := 'The bundled UsbDk package failed its integrity check.';
    exit;
  end;
  if not Exec(ExpandConstant('{sys}\msiexec.exe'), '/i "' + Msi + '" /qn /norestart /L*v "' + ExpandConstant('{tmp}\NimBox-UsbDk-install.log') + '"', '', SW_HIDE, ewWaitUntilTerminated, Code) then begin
    Result := 'Could not start the UsbDk runtime installer.';
    exit;
  end;
  if Code = 3010 then NeedsRestart := True
  else if Code <> 0 then Result := 'UsbDk runtime installation failed (code ' + IntToStr(Code) + '). See NimBox-UsbDk-install.log in the temporary folder.';
end;
