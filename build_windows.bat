@echo off
setlocal
cd /d "%~dp0"

where cmake >nul 2>nul
if errorlevel 1 (
    echo CMake wurde nicht gefunden. Bitte CMake installieren und zum PATH hinzufuegen.
    exit /b 1
)

echo ==^> Baue 64-Bit
cmake -S . -B build\x64 -A x64
if errorlevel 1 exit /b 1
cmake --build build\x64 --config Release
if errorlevel 1 exit /b 1

echo ==^> Baue 32-Bit
cmake -S . -B build\x86 -A Win32
if errorlevel 1 exit /b 1
cmake --build build\x86 --config Release
if errorlevel 1 exit /b 1

echo.
echo Fertig:
dir /s /b build\x64\Release\ColumnReplace.dll
dir /s /b build\x86\Release\ColumnReplace.dll
