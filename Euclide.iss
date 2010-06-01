; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=Euclide
AppVerName=Euclide 0.98
AppPublisher=�tienne Dupuis
AppPublisherURL=http://lestourtereaux.free.fr/euclide/
AppSupportURL=http://lestourtereaux.free.fr/euclide/
AppUpdatesURL=http://lestourtereaux.free.fr/euclide/
DefaultDirName={pf}\Euclide
DefaultGroupName=Euclide

AppVersion=0.98
; uncomment the following line if you want your installation to run on NT 3.51 too.
; MinVersion=4,3.51

[Tasks]

[Files]
Source: Euclide.exe; DestDir: {app}; Flags: ignoreversion
Source: Output.txt; DestDir: {app}; Flags: ignoreversion; Permissions: users-modify
Source: Input.txt; DestDir: {app}; Flags: confirmoverwrite ignoreversion; Permissions: users-modify
Source: Help File\Euclide.html; DestDir: {app}; Flags: ignoreversion
Source: Help File\Uputstvo-Euclide.doc; DestDir: {app}; Flags: ignoreversion
Source: Language Files\Slovencina.txt; DestDir: {app}; Flags: ignoreversion
Source: Language Files\Nederlands.txt; DestDir: {app}; Flags: ignoreversion
Source: Language Files\Fran�ais.txt; DestDir: {app}; Flags: ignoreversion
Source: Language Files\Espa�ol.txt; DestDir: {app}; Flags: ignoreversion
Source: Language Files\English.txt; DestDir: {app}; Flags: ignoreversion
Source: Language Files\Deutsch.txt; DestDir: {app}; Flags: ignoreversion
Source: Language Files\Romana.txt; DestDir: {app}; Flags: ignoreversion
Source: Language Files\Srpski.txt; DestDir: {app}; Flags: ignoreversion
Source: Set Language.bat; DestDir: {app}; Flags: ignoreversion
Source: Euclide.txt; DestDir: {app}; Flags: ignoreversion; Permissions: users-modify

[Icons]
Name: {group}\Euclide; Filename: {app}\Euclide.exe; Parameters: Input.txt Output.txt; WorkingDir: {app}; IconIndex: 0
Name: {group}\Euclide (Batch Mode); Filename: {app}\Euclide.exe; Parameters: Input.txt Output.txt -batch; WorkingDir: {app}; IconIndex: 0
Name: {group}\Input File; Filename: {app}\Input.txt; IconIndex: 0
Name: {group}\Output File; Filename: {app}\Output.txt; IconIndex: 0
Name: {group}\Help Files\Help; Filename: {app}\Euclide.html; IconIndex: 0
Name: {group}\Help Files\Uputstvo; Filename: {app}\Uputstvo-Euclide.doc; IconIndex: 0
Name: {group}\Language Files\Deutsch; Filename: {app}\Set Language.bat; Parameters: Deutsch.txt; WorkingDir: {app}; IconIndex: 0; Flags: runminimized closeonexit
Name: {group}\Language Files\English; Filename: {app}\Set Language.bat; Parameters: English.txt; WorkingDir: {app}; IconIndex: 0; Flags: runminimized closeonexit
Name: {group}\Language Files\Espa�ol; Filename: {app}\Set Language.bat; Parameters: Espa�ol.txt; WorkingDir: {app}; IconIndex: 0; Flags: runminimized closeonexit
Name: {group}\Language Files\Fran�ais; Filename: {app}\Set Language.bat; Parameters: Fran�ais.txt; WorkingDir: {app}; IconIndex: 0; Flags: runminimized closeonexit
Name: {group}\Language Files\Nederlands; Filename: {app}\Set Language.bat; Parameters: Nederlands.txt; WorkingDir: {app}; IconIndex: 0; Flags: runminimized closeonexit
Name: {group}\Language Files\Romana; Filename: {app}\Set Language.bat; Parameters: Romana.txt; WorkingDir: {app}; IconIndex: 0; Flags: runminimized closeonexit
Name: {group}\Language Files\Slovencina; Filename: {app}\Set Language.bat; Parameters: Slovencina.txt; WorkingDir: {app}; IconIndex: 0; Flags: runminimized closeonexit
Name: {group}\Language Files\Srpski; Filename: {app}\Set Language.bat; Parameters: Srpski.txt; WorkingDir: {app}; IconIndex: 0; Flags: runminimized closeonexit

[Run]
Filename: {app}\Euclide.html; Description: Read Help File; Flags: nowait postinstall skipifsilent shellexec; WorkingDir: {app}

[_ISTool]
EnableISX=false

[UninstallDelete]
Name: Debug.txt; Type: files
