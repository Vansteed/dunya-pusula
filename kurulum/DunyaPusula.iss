; Dunya Pusula kurulum betigi (Inno Setup 7).
; Derleme: kurulum\kur.bat  ->  kurulum\cikti\DunyaPusulaKurulum.exe
;
; Uygulama donanim sensorleri icin cekirdek surucusu yukleyen SensorAjani'ni
; calistiriyor; bu yuzden yonetici hakki sart (PrivilegesRequired=admin).

#define UygulamaAdi "Dünya Pusula"
#define Surum "1.0.2"
#define Yayinci "Safer"
#define ExeAdi "DunyaPusula.exe"

[Setup]
AppId={{7F3A1C64-9E52-4B18-A0D7-2C6B5E8F4A31}
AppName={#UygulamaAdi}
AppVersion={#Surum}
AppVerName={#UygulamaAdi} {#Surum}
AppPublisher={#Yayinci}
DefaultDirName={autopf}\DunyaPusula
DefaultGroupName={#UygulamaAdi}
DisableProgramGroupPage=yes
OutputDir=cikti
OutputBaseFilename=DunyaPusulaKurulum
SetupIconFile=..\res\DunyaPusula.ico
UninstallDisplayIcon={app}\{#ExeAdi}
UninstallDisplayName={#UygulamaAdi}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
; 64 bit uygulama - 32 bit Windows'a kurulmasin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; Sensor ajani icin yonetici gerekiyor
PrivilegesRequired=admin

[Languages]
Name: "turkce"; MessagesFile: "compiler:Languages\Turkish.isl"

[Tasks]
Name: "masaustu"; Description: "Masaüstü kısayolu oluştur"; GroupDescription: "Ek kısayollar:"

[Files]
; Ana uygulama ve sensor ajani
Source: "..\build\{#ExeAdi}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\SensorAjani.exe"; DestDir: "{app}"; Flags: ignoreversion
; Qt calisma zamani DLL'leri (build kokundeki tum dll'ler)
Source: "..\build\*.dll"; DestDir: "{app}"; Flags: ignoreversion
; Qt eklentileri ve QML modulleri - alt klasor yapisi korunur
; *.pdb hata ayiklama sembolleri - kurulumda isi yok, onlarca MB tutuyor
Source: "..\build\platforms\*"; DestDir: "{app}\platforms"; Excludes: "*.pdb"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\qml\*"; DestDir: "{app}\qml"; Excludes: "*.pdb"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#UygulamaAdi}"; Filename: "{app}\{#ExeAdi}"
Name: "{group}\{#UygulamaAdi} - Kaldır"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#UygulamaAdi}"; Filename: "{app}\{#ExeAdi}"; Tasks: masaustu

[Run]
; shellexec sart: uygulama manifesti yonetici istiyor, Inno postinstall adimini
; yetkisiz kullanici baglaminda calistiriyor ve CreateProcess kod 740 veriyor.
; ShellExecute ise UAC istemini duzgun aciyor.
Filename: "{app}\{#ExeAdi}"; Description: "{#UygulamaAdi} uygulamasını başlat"; Flags: nowait postinstall skipifsilent shellexec

[UninstallDelete]
; SensorAjani'nin bosluksuz yola aldigi calisma kopyasi - kaldirmada temizlensin
Type: filesandordirs; Name: "C:\ProgramData\DunyaPusula"

[Code]
// Kurulumdan once calisan surumu kapat, yoksa dosyalar kilitli kalir.
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  SonucKodu: Integer;
begin
  Exec(ExpandConstant('{sys}\taskkill.exe'), '/F /IM DunyaPusula.exe /IM SensorAjani.exe',
       '', SW_HIDE, ewWaitUntilTerminated, SonucKodu);
  Result := '';
end;
