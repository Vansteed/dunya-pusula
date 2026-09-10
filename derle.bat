@echo off
set VSBT=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools
call "%VSBT%\VC\Auxiliary\Build\vcvars64.bat" >nul
set CMAKEBIN=%VSBT%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe
set NINJABIN=%VSBT%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe
"%CMAKEBIN%" -B build -S . -G Ninja -DCMAKE_MAKE_PROGRAM="%NINJABIN%" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/qtvcpkg/installed/x64-windows || exit /b 1
"%CMAKEBIN%" --build build || exit /b 1
