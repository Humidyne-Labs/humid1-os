@echo off
setlocal
cd /d "%~dp0"

:: Set default script name if not passed via command line argument
set "TARGET_SCRIPT=%~1"
if "%TARGET_SCRIPT%"=="" set "TARGET_SCRIPT=term_usb_toolbox.ps1"
set "FULL_PATH=%~dp0%TARGET_SCRIPT%"

:: Verify target script exists before requesting elevation
if not exist "%FULL_PATH%" (
    echo [ERROR] Target script not found: "%FULL_PATH%"
    pause
    exit /b 1
)

:: Test for Administrator privileges
net session >nul 2>&1
if %errorlevel% neq 0 (
    :: Not elevated: invoke UAC prompt and launch PowerShell directly into hidden mode
    powershell -NoProfile -ExecutionPolicy Bypass -Command ^
        "Start-Process powershell.exe -ArgumentList '-NoProfile -ExecutionPolicy Bypass -NonInteractive -WindowStyle Hidden -File \"\"%FULL_PATH%\"\"' -Verb RunAs"
    exit /b
)

:: Already running elevated: launch PowerShell process hidden
powershell.exe -NoProfile -ExecutionPolicy Bypass -NonInteractive -WindowStyle Hidden -File "%FULL_PATH%"