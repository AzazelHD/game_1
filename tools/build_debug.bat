@echo off
cd /d "%~dp0.."
call "..\engine\tools\vsenv.bat" cmake --preset game-debug
call "..\engine\tools\vsenv.bat" cmake --build build/debug --config Debug