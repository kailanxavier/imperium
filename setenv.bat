@echo off
setlocal EnableDelayedExpansion

REM Console colours
for /f %%a in ('echo prompt $E ^| cmd') do set "ESC=%%a"
set "RED=%ESC%[31m"
set "GREEN=%ESC%[32m"
set "NC=%ESC%[0m"

REM Root directories
set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"

set "IMP_ROOT=%ROOT%"
set "IMP_ENGINE_ROOT=%ROOT%\imp"
set "IMP_TOOLS=%ROOT%\tools"
set "IMP_PROJ=%ROOT%\game"

echo.
echo Searching for build configurations...
echo.

REM Release builds
set "IMP_BUILD_REL="
set "IMP_EDITOR="

if exist "%ROOT%\build\windows-release-clang\engine\bin\tools\editor\imp_editor.exe" (
    set "IMP_BUILD_REL=%ROOT%\build\windows-release-clang\engine"
    set "IMP_EDITOR=!IMP_BUILD_REL!\bin\tools\editor\imp_editor.exe"
    echo !GREEN![SUCCESS] Found Windows Release (Clang^)!NC!
)

if not defined IMP_EDITOR (
    if exist "%ROOT%\build\windows-release\engine\bin\tools\editor\imp_editor.exe" (
        set "IMP_BUILD_REL=%ROOT%\build\windows-release\engine"
        set "IMP_EDITOR=!IMP_BUILD_REL!\bin\tools\editor\imp_editor.exe"
        echo !GREEN![SUCCESS] Found Windows Release (MSVC x64^)!NC!
    )
)

if not defined IMP_EDITOR (
    echo !RED![FAILED] No release editor build found.!NC!
)

REM Debug builds
set "IMP_BUILD_DEB="
set "IMP_EDITOR_D="

if exist "%ROOT%\build\windows-debug-clang\engine\bin\tools\editor\imp_editor_d.exe" (
    set "IMP_BUILD_DEB=%ROOT%\build\windows-debug-clang\engine"
    set "IMP_EDITOR_D=!IMP_BUILD_DEB!\bin\tools\editor\imp_editor_d.exe"
    echo !GREEN![SUCCESS] Found Windows Debug (Clang^)!NC!
)

if not defined IMP_EDITOR_D (
    if exist "%ROOT%\build\windows-debug\engine\bin\tools\editor\imp_editor_d.exe" (
        set "IMP_BUILD_DEB=%ROOT%\build\windows-debug\engine"
        set "IMP_EDITOR_D=!IMP_BUILD_DEB!\bin\tools\editor\imp_editor_d.exe"
        echo !GREEN![SUCCESS] Found Windows Debug (MSVC x64^)!NC!
    )
)

if not defined IMP_EDITOR_D (
    echo !RED![FAILED] No debug editor build found.!NC!
)

echo.
echo Environment variables:
call :setvar IMP_ROOT "%IMP_ROOT%"
call :setvar IMP_ENGINE_ROOT "%IMP_ENGINE_ROOT%"
call :setvar IMP_TOOLS "%IMP_TOOLS%"
call :setvar IMP_PROJ "%IMP_PROJ%"
call :setvar IMP_BUILD_DEB "%IMP_BUILD_DEB%"
call :setvar IMP_BUILD_REL "%IMP_BUILD_REL%"
call :setvar IMP_EDITOR_D "%IMP_EDITOR_D%"
call :setvar IMP_EDITOR "%IMP_EDITOR%"

pause
exit /b

:setvar
set "%~1=%~2"

if not defined %~1 (
    set "COLOUR=!RED!"
) else (
    setx "%~1" "%~2" >nul

    if errorlevel 1 (
        set "COLOUR=!RED!"
    ) else (
        set "COLOUR=!GREEN!"
    )
)
echo !COLOUR!%1=%~2!NC!
goto :eof
