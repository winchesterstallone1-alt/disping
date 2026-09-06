# Build script for DisPing
$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " Building DisPing (x86_64 ASM + C++20)  " -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$msysBin = "C:\msys64\ucrt64\bin"
if (Test-Path $msysBin) {
    $env:PATH = "$msysBin;$env:PATH"
}

$buildDir = Join-Path $PSScriptRoot "..\build"
if (!(Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

Push-Location $buildDir

try {
    Write-Host "[*] Configuring with CMake & Ninja..." -ForegroundColor Yellow
    & cmake -G "Ninja" ..
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed."
    }

    Write-Host "[*] Compiling binaries with Ninja..." -ForegroundColor Yellow
    & ninja
    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed."
    }

    Write-Host "[OK] Build succeeded!" -ForegroundColor Green
    Write-Host "Executables located at:" -ForegroundColor Cyan
    Write-Host "  - $buildDir\disping.exe"
    Write-Host "  - $buildDir\disping_tests.exe"
}
finally {
    Pop-Location
}
