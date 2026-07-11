# ──────────────────────────────────────────────────────────────────────────────
#  Windows Installer — PowerShell script
#  Right-click → Run with PowerShell, or:  powershell -File install-windows.ps1
# ──────────────────────────────────────────────────────────────────────────────

$ErrorActionPreference = "Stop"
$APP_NAME = "Atom Wavefunction Simulator"
$VERSION = "1.0.0"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  $APP_NAME — Windows Installer" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# ── Check for vcpkg ──────────────────────────────────────────────────────────
$VCPKG_ROOT = $env:VCPKG_ROOT
if (-not $VCPKG_ROOT) {
    $VCPKG_ROOT = "$env:USERPROFILE\vcpkg"
}

if (-not (Test-Path "$VCPKG_ROOT\vcpkg.exe")) {
    Write-Host "vcpkg not found. It's needed to install dependencies." -ForegroundColor Yellow
    Write-Host ""
    $install = Read-Host "Install vcpkg automatically? (y/n)"
    if ($install -eq "y") {
        Write-Host "Installing vcpkg..."
        git clone https://github.com/Microsoft/vcpkg.git $VCPKG_ROOT
        Push-Location $VCPKG_ROOT
        .\bootstrap-vcpkg.bat
        Pop-Location
        Write-Host "vcpkg installed!" -ForegroundColor Green
    } else {
        Write-Host "Please install vcpkg manually: https://github.com/Microsoft/vcpkg" -ForegroundColor Red
        exit 1
    }
}

# ── Install dependencies ─────────────────────────────────────────────────────
Write-Host "Installing dependencies (GLFW, GLM)..." -ForegroundColor Yellow
Push-Location $VCPKG_ROOT
.\vcpkg install glfw3:x64-windows glm:x64-windows
Pop-Location
Write-Host "Dependencies installed!" -ForegroundColor Green

# ── Build ────────────────────────────────────────────────────────────────────
$SCRIPT_DIR = Split-Path -Parent $MyInvocation.MyCommand.Path
$PROJECT_DIR = Split-Path -Parent $SCRIPT_DIR

Write-Host "Building $APP_NAME..." -ForegroundColor Yellow
Push-Location $PROJECT_DIR

cmake -B build -S . -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

cmake --build build --config Release --parallel

Pop-Location
Write-Host "Build successful!" -ForegroundColor Green

# ── Install ──────────────────────────────────────────────────────────────────
$INSTALL_DIR = "$env:LOCALAPPDATA\AtomSim"
$DESKTOP_DIR = [Environment]::GetFolderPath("Desktop")

Write-Host "Installing to $INSTALL_DIR..." -ForegroundColor Yellow
New-Item -ItemType Directory -Force -Path $INSTALL_DIR | Out-Null
Copy-Item "$PROJECT_DIR\build\Release\atom_sim.exe" "$INSTALL_DIR\"
Copy-Item -Recurse "$PROJECT_DIR\shaders" "$INSTALL_DIR\"

# Create shortcut on Desktop
$WScriptShell = New-Object -ComObject WScript.Shell
$Shortcut = $WScriptShell.CreateShortcut("$DESKTOP_DIR\Atom Wavefunction Simulator.lnk")
$Shortcut.TargetPath = "$INSTALL_DIR\atom_sim.exe"
$Shortcut.WorkingDirectory = "$INSTALL_DIR"
$Shortcut.Save()

# Start menu entry
$START_MENU = [Environment]::GetFolderPath("StartMenu") + "\Programs\AtomSim"
New-Item -ItemType Directory -Force -Path $START_MENU | Out-Null
$StartShortcut = $WScriptShell.CreateShortcut("$START_MENU\Atom Wavefunction Simulator.lnk")
$StartShortcut.TargetPath = "$INSTALL_DIR\atom_sim.exe"
$StartShortcut.WorkingDirectory = "$INSTALL_DIR"
$StartShortcut.Save()

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  Installation Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "  Desktop shortcut created" -ForegroundColor White
Write-Host "  Start Menu entry created" -ForegroundColor White
Write-Host "  Launch from: $INSTALL_DIR\atom_sim.exe" -ForegroundColor White
Write-Host ""

$launch = Read-Host "Launch now? (y/n)"
if ($launch -eq "y") {
    Start-Process "$INSTALL_DIR\atom_sim.exe"
}
