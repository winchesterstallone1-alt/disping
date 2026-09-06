@echo off
setlocal
chcp 65001 >nul
title DisPing - Gaming Latency & FPS Optimizer

:: Check for Administrator privileges
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo.
    echo ======================================================================
    echo  [!] Запуск от имени Администратора...
    echo ======================================================================
    powershell -Command "Start-Process '%~dp0build\disping.exe' -Verb RunAs"
    exit /b
)

:: Already elevated: run directly
"%~dp0build\disping.exe" %*
