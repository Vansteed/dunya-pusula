@echo off
REM yayinla.bat - SensorAjani.exe'yi tek dosya, self-contained olarak yayinlar.
REM Cikti: cpp\sensor\cikti\SensorAjani.exe (CMake buradan kopyalar).
cd /d "%~dp0"
dotnet publish -c Release -r win-x64 --self-contained true ^
    -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true ^
    -o cikti
if errorlevel 1 exit /b 1
echo Yayinlandi: %~dp0cikti\SensorAjani.exe
