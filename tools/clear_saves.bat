@echo off
rem Deletes every stale user.dat save queue under the repo (workspace root and
rem all build output dirs). The game writes user.dat relative to the process
rem working directory, so stale copies can appear in several places after
rem switching how the exe is launched.
rem
rem Run this after any "important" change that makes old campaigns invalid
rem (save-format/schema changes, roster seed changes, progression semantics)
rem so testing always starts from a fresh campaign. It does NOT touch user
rem settings (those live in a separate settings file, intentionally kept).
setlocal
set "ROOT=%~dp0\.."

echo Deleting user.dat copies under %ROOT%
for /r "%ROOT%" %%f in (user.dat) do (
    if exist "%%f" (
        echo   deleting %%f
        del /f /q "%%f"
    )
)
echo Done. Next "New Game" seeds the current roster/progression.