@echo off
setlocal enabledelayedexpansion

for /f %%a in ('echo prompt $E ^| cmd') do set "ESC=%%a"
set "RED=%ESC%[31m"
set "GREEN=%ESC%[32m"
set "NC=%ESC%[0m"

set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"

call :setvar IMP_ROOT "%ROOT%"
call :setvar IMP_ENGINE_ROOT "%ROOT%\imp"
call :setvar IMP_TOOLS "%ROOT%\tools"
call :setvar IMP_BUILD_DEB "%ROOT%\build\windows-debug\engine"
call :setvar IMP_BUILD_REL "%ROOT%\build\windows-release\engine"
call :setvar IMP_EDITOR_D "%IMP_BUILD_DEB%\bin\tools\editor\imp_editor_d.exe"
call :setvar IMP_EDITOR "%IMP_BUILD_REL%\bin\tools\editor\imp_editor.exe"
call :setvar IMP_PROJ "%ROOT%\sandbox"

echo.
echo Environment variables have been set.
pause
exit /b

:setvar
if exist "%~2" (
    setx %1 "%~2" >nul
    set "COLOUR=!GREEN!"
) else (
    setx %1 "%~2" >nul
    set "COLOUR=!RED!"
)

echo !COLOUR!%1=%~2!NC!
goto :eof
