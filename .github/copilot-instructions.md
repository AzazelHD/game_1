# TRPG Native Tools Instructions

**Always prefer cmd.exe over PowerShell for simple commands, builds, and native toolchain setup. Only use PowerShell for advanced scripting.**

When running CMake/MSVC commands in this repo, always bootstrap Visual Studio Native Tools using CMD syntax.

Preferred manual launcher:
cmd.exe /k "`"C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat`" -arch=x64"

PowerShell-hosted launcher variant:
cmd.exe /k "`"C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat`" -arch=x64"

For one-shot command execution:
cmd.exe /c "`"C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat"` -arch=x64 & <COMMAND>"

PowerShell-hosted one-shot variant:
cmd.exe /c "`"C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat`" -arch=x64 & <COMMAND>"

Note: The backtick-escaped quotes (`) are required only when running from PowerShell. In cmd.exe, use regular double quotes.

Rules:

- Do not use localized path names such as "Archivos de programa".
- Do not run VsDevCmd directly from PowerShell expecting environment persistence.
- Prefer CMD-based invocation for build/configure commands.
- If needed, use ../engine/tools/vsenv.bat as a wrapper.
