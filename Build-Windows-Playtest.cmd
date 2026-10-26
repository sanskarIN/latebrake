@echo off
setlocal
where pwsh.exe >nul 2>&1
if errorlevel 1 (
  echo PowerShell 7 is required. Install pwsh, then run this launcher again.
  echo No game executable was created.
  pause
  exit /b 1
)
pwsh.exe -NoProfile -File "%~dp0scripts\Build-WindowsPlaytest.ps1" %*
set "exitCode=%ERRORLEVEL%"
if not "%exitCode%"=="0" (
  echo.
  echo Build failed. Review the actual Unreal Engine errors above.
  echo No verified playable Windows package is available from this run.
) else (
  echo.
  echo Build output and game EXE path were printed above.
  echo Keep the entire cooked directory when testing on Windows.
)
pause
exit /b %exitCode%
