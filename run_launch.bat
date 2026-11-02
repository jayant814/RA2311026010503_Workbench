@echo off
rem Simple launcher for ThermoBridge Workbench
rem This batch file ensures the executable is started with the correct working directory

set "EXE_DIR=%~dp0"
pushd "%EXE_DIR%"
start "ThermoBridge" "Launch.exe"
popd
