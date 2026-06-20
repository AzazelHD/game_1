@echo off
cd /d "%~dp0.."
call "..\engine\tools\vsenv.bat" cmake --preset game-release
call "..\engine\tools\vsenv.bat" cmake --build build/release --config Release