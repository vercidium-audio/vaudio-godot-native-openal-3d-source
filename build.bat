@echo off
setlocal

if exist .env (
    for /f "usebackq tokens=1,* delims==" %%A in (".env") do set %%A=%%B
)

scons target=template_debug
if errorlevel 1 exit /b 1

scons target=template_release
if errorlevel 1 exit /b 1

set BIN_DIR=addons\vaudio-godot-native-openal-3d\bin

if not "%VAUDIO_RELEASE_DIR%"=="" (
    echo Copying build output to %VAUDIO_RELEASE_DIR%
    copy /y "%BIN_DIR%\libvaudiogodotnativeopenal3d.windows.template_debug.x86_64.dll" "%VAUDIO_RELEASE_DIR%\bin\" >nul
    copy /y "%BIN_DIR%\libvaudiogodotnativeopenal3d.windows.template_release.x86_64.dll" "%VAUDIO_RELEASE_DIR%\bin\" >nul
    if exist "%BIN_DIR%\vaudio-render-child.exe" copy /y "%BIN_DIR%\vaudio-render-child.exe" "%VAUDIO_RELEASE_DIR%\bin\" >nul
) else (
    echo VAUDIO_RELEASE_DIR not set - skipping copy to release repo
)

if not "%VAUDIO_DEMO_ADDON_DIR%"=="" (
    echo Copying build output to %VAUDIO_DEMO_ADDON_DIR%
    copy /y "%BIN_DIR%\libvaudiogodotnativeopenal3d.windows.template_debug.x86_64.dll" "%VAUDIO_DEMO_ADDON_DIR%\bin\" >nul
    copy /y "%BIN_DIR%\libvaudiogodotnativeopenal3d.windows.template_release.x86_64.dll" "%VAUDIO_DEMO_ADDON_DIR%\bin\" >nul
    if exist "%BIN_DIR%\vaudio-render-child.exe" copy /y "%BIN_DIR%\vaudio-render-child.exe" "%VAUDIO_DEMO_ADDON_DIR%\bin\" >nul
) else (
    echo VAUDIO_DEMO_ADDON_DIR not set - skipping copy to demo project addon
)

endlocal
