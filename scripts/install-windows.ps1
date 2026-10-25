# ──────────────────────────────────────────────────────────────────────────────
#  Windows Installer — builds from source and adds Desktop / Start Menu shortcuts
#  Right-click → Run with PowerShell, or:  powershell -File install-windows.ps1
#
#  Most users should simply download AtomSim-Windows-x64.exe from the GitHub
#  Releases page instead — it is a single self-contained executable.
#
#  Requirements: Visual Studio 2019+ (or Build Tools) with "Desktop development
#  with C++".  GLFW, GLEW and GLM are downloaded and built automatically.
# ──────────────────────────────────────────────────────────────────────────────

$ErrorActionPreference = "Stop"
$APP_NAME = "Atom Wavefunction Simulator"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  $APP_NAME — Windows Installer" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# ── Locate CMake (PATH first, then the copy bundled with Visual Studio) ──────
$cmake = (Get-Command cmake -ErrorAction SilentlyContinue).Source
if (-not $cmake) {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $vs = & $vswhere -latest -products * -property installationPath
        $candidate = Join-Path $vs "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
        if (Test-Path $candidate) { $cmake = $candidate }
    }
}
if (-not $cmake) {
    Write-Host "CMake / Visual Studio C++ tools not found." -ForegroundColor Red
    Write-Host "Install Visual Studio with 'Desktop development with C++', or download"
    Write-Host "the ready-made AtomSim-Windows-x64.exe from the GitHub Releases page."
    exit 1
}

# ── Build ────────────────────────────────────────────────────────────────────
$SCRIPT_DIR = Split-Path -Parent $MyInvocation.MyCommand.Path
$PROJECT_DIR = Split-Path -Parent $SCRIPT_DIR

Write-Host "Building $APP_NAME..." -ForegroundColor Yellow
Push-Location $PROJECT_DIR
& $cmake -B build -S . -DATOM_SIM_PORTABLE=ON
if ($LASTEXITCODE -ne 0) { Pop-Location; throw "CMake configure failed" }
& $cmake --build build --config Release --parallel
if ($LASTEXITCODE -ne 0) { Pop-Location; throw "Build failed" }
Pop-Location
Write-Host "Build successful!" -ForegroundColor Green

# ── Install ──────────────────────────────────────────────────────────────────
$INSTALL_DIR = "$env:LOCALAPPDATA\AtomSim"
$EXE = "$INSTALL_DIR\AtomSim.exe"
$DESKTOP_DIR = [Environment]::GetFolderPath("Desktop")

Write-Host "Installing to $INSTALL_DIR..." -ForegroundColor Yellow
New-Item -ItemType Directory -Force -Path $INSTALL_DIR | Out-Null
Copy-Item "$PROJECT_DIR\build\Release\atom_sim.exe" $EXE -Force

$WScriptShell = New-Object -ComObject WScript.Shell
$Shortcut = $WScriptShell.CreateShortcut("$DESKTOP_DIR\$APP_NAME.lnk")
$Shortcut.TargetPath = $EXE
$Shortcut.WorkingDirectory = $INSTALL_DIR
$Shortcut.Save()

$START_MENU = [Environment]::GetFolderPath("StartMenu") + "\Programs\AtomSim"
New-Item -ItemType Directory -Force -Path $START_MENU | Out-Null
$StartShortcut = $WScriptShell.CreateShortcut("$START_MENU\$APP_NAME.lnk")
$StartShortcut.TargetPath = $EXE
$StartShortcut.WorkingDirectory = $INSTALL_DIR
$StartShortcut.Save()

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  Installation Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "  Desktop shortcut created" -ForegroundColor White
Write-Host "  Start Menu entry created" -ForegroundColor White
Write-Host "  Launch from: $EXE" -ForegroundColor White
Write-Host ""

$launch = Read-Host "Launch now? (y/n)"
if ($launch -eq "y") {
    Start-Process $EXE
}
