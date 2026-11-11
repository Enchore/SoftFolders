@echo off
setlocal
set "SF_BIN=%~dp0..\build\bin"
if not exist "%SF_BIN%\SoftFoldersSetup.exe" (
    echo Build the project with build.cmd first.
    exit /b 1
)
start "" /wait "%SF_BIN%\SoftFoldersSetup.exe" --verify-icons
if errorlevel 1 (
    echo Icon validation failed. See build\bin\icon-verification.json.
    exit /b 1
)
type "%SF_BIN%\icon-verification.json"
exit /b 0
