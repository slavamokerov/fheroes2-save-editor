# Windows build (PowerShell). Requires: CMake and Qt6 (msvc2022_64) — for example via
# the Qt online installer or aqtinstall:  pip install aqtinstall; aqt install-qt windows desktop 6.8.2 win64_msvc2022_64
param(
    [string]$QtDir = "C:\Qt\6.8.2\msvc2022_64"
)

$ErrorActionPreference = "Stop"

cmake -S . -B build-win -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="$QtDir"
cmake --build build-win --config Release

Write-Host ""
Write-Host "Done: build-win\Release\fheroes2-save-editor.exe"
Write-Host "To redistribute on other machines, use windeployqt:"
Write-Host "  $QtDir\bin\windeployqt.exe build-win\Release\fheroes2-save-editor.exe"
