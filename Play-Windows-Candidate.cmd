@echo off
setlocal
if "%~1"=="" (
  echo Usage: Play-Windows-Candidate.cmd "full-path-to-LateBrake-Win64-folder"
  echo The selected directory must contain Latebrake-candidate-manifest.json.
  echo A real Unreal build is required; no EXE is downloaded or fabricated.
  pause
  exit /b 2
)
where pwsh.exe >nul 2>&1
if errorlevel 1 (
  echo Install PowerShell 7, then launch this private Windows candidate again.
  pause
  exit /b 1
)
pwsh.exe -NoProfile -File "%~dp0scripts\Run-WindowsPlaytest.ps1" -CandidateDirectory "%~1"
set "exitCode=%ERRORLEVEL%"
if not "%exitCode%"=="0" (
  echo Game session or candidate preflight failed. See diagnostic output above.
) else (
  echo Process exited normally. Gameplay still needs manual observation.
)
pause
exit /b %exitCode%
