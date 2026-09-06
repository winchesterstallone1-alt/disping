# Test runner for DisPing
$ErrorActionPreference = "Stop"

$testExe = Join-Path $PSScriptRoot "..\build\disping_tests.exe"
if (!(Test-Path $testExe)) {
    Write-Host "[!] Test executable not found. Running build script first..." -ForegroundColor Yellow
    & "$PSScriptRoot\build.ps1"
}

Write-Host "`n[*] Executing DisPing Test Suite..." -ForegroundColor Cyan
& $testExe
if ($LASTEXITCODE -eq 0) {
    Write-Host "`n[OK] All verification tests passed successfully!" -ForegroundColor Green
} else {
    Write-Host "`n[FAIL] Test suite reported failures." -ForegroundColor Red
    exit 1
}
