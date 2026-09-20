@echo off
setlocal enabledelayedexpansion

:: Set the output log file in the same directory as this batch file
set "LOGFILE=%~dp0combined_output.log"

:: Initialize/Create the log file immediately
> "%LOGFILE%" echo Dump Log - %date% %time%
echo. >> "%LOGFILE%"

set "count=0"

:: Loop through all .txt files recursively starting from the script's folder
for /r "%~dp0" %%f in (*.txt, *.c, *.cpp, *.h, *.hpp) do (
    set /a count+=1
    echo ======================================== >> "%LOGFILE%"
    echo File: %%f >> "%LOGFILE%"
    echo ======================================== >> "%LOGFILE%"
    type "%%f" >> "%LOGFILE%"
    echo. >> "%LOGFILE%"
    echo. >> "%LOGFILE%"
)

echo Done! Processed !count! text files.
echo Log saved to: %LOGFILE%
pause