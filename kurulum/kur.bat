@echo off
REM Kurulum dosyasini uretir. Once cpp\derle.bat ile build guncel olmali.
set ISCC="C:\Program Files\Inno Setup 7\ISCC.exe"
if not exist %ISCC% (
    echo Inno Setup bulunamadi: %ISCC%
    exit /b 1
)
if not exist "%~dp0..\build\DunyaPusula.exe" (
    echo build\DunyaPusula.exe yok - once derle.bat calistir.
    exit /b 1
)
%ISCC% "%~dp0DunyaPusula.iss" || exit /b 1
echo.
echo Kurulum dosyasi: %~dp0cikti\DunyaPusulaKurulum.exe
