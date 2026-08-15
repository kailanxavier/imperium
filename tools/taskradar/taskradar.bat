@echo off
for /f %%a in ('echo prompt $E ^| cmd') do set "ESC=%%a"

set "RED=%ESC%[31m"
set "GREEN=%ESC%[32m"
set "NC=%ESC%[0m"

title TaskRadar
echo %GREEN%Initialising TaskRadar%NC%
echo %GREEN%Version: 0.1.0%GREEN%
py src/main.py
