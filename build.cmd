@echo off
setlocal
set "SF_ROOT=%~dp0"
if /I "%VSCMD_ARG_TGT_ARCH%"=="x64" goto environmentReady
set "SF_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%SF_VSWHERE%" goto missingTools
for /f "usebackq delims=" %%I in (`"%SF_VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "SF_VSROOT=%%I"
if not defined SF_VSROOT goto missingTools
call "%SF_VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
:environmentReady
if not exist "%SF_ROOT%build\bin" mkdir "%SF_ROOT%build\bin"
pushd "%SF_ROOT%build"
cl /nologo /std:c++17 /EHsc /W4 /O1 /GL /MT /utf-8 /LD "..\src\shell.cpp" /Foshell.obj /Febin\SoftFoldersShell.dll /link /DEF:"..\src\SoftFolders.def" /LTCG /OPT:REF /OPT:ICF /DYNAMICBASE /NXCOMPAT /IMPLIB:SoftFoldersShell.lib user32.lib gdi32.lib shell32.lib shlwapi.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib comdlg32.lib advapi32.lib gdiplus.lib windowscodecs.lib uxtheme.lib dwmapi.lib psapi.lib
if errorlevel 1 goto buildFailed
cl /nologo /std:c++17 /EHsc /W4 /O1 /GL /MT /utf-8 "..\src\setup.cpp" /Fosetup.obj /Febin\SoftFoldersSetup.exe /link /SUBSYSTEM:WINDOWS /LTCG /OPT:REF /OPT:ICF /DYNAMICBASE /NXCOMPAT /MANIFEST:EMBED /MANIFESTINPUT:"..\src\app.manifest" user32.lib gdi32.lib shell32.lib shlwapi.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib comdlg32.lib advapi32.lib gdiplus.lib windowscodecs.lib psapi.lib
if errorlevel 1 goto buildFailed
popd
exit /b 0
:buildFailed
popd
exit /b 1
:missingTools
echo Install Visual Studio Build Tools with Desktop development with C++ and a Windows SDK.
exit /b 1
