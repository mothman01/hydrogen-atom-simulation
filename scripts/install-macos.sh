#!/usr/bin/env bash
# ──────────────────────────────────────────────────────────────────────────────
#  macOS Installer — install via Homebrew or manual .app bundle
# ──────────────────────────────────────────────────────────────────────────────
set -euo pipefail

RED='\033[0;31m'; GREEN='\033[0;32m'; BOLD='\033[1m'; NC='\033[0m'
APP="Atom Wavefunction Simulator"

echo -e "${BOLD}${GREEN}=== ${APP} — macOS Installer ===${NC}"
echo ""

# Check if running on macOS
if [[ "$(uname)" != "Darwin" ]]; then
    echo -e "${RED}This installer is for macOS only.${NC}"
    exit 1
fi

# Option 1: Homebrew
if command -v brew &>/dev/null; then
    echo "Homebrew detected."
    read -p "Install via Homebrew? (recommended) [Y/n]: " answer
    if [[ "$answer" != "n" && "$answer" != "N" ]]; then
        echo "Installing dependencies..."
        brew install cmake glfw glm

        echo "Building..."
        SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
        cd "$SCRIPT_DIR"
        cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
        cmake --build build --parallel

        echo "Creating .app bundle..."
        mkdir -p /Applications/AtomSim.app/Contents/MacOS
        mkdir -p /Applications/AtomSim.app/Contents/Resources/shaders
        cp build/atom_sim /Applications/AtomSim.app/Contents/MacOS/
        cp -r shaders/* /Applications/AtomSim.app/Contents/Resources/shaders/

        cat > /Applications/AtomSim.app/Contents/Info.plist << 'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key><string>Atom Wavefunction Simulator</string>
    <key>CFBundleExecutable</key><string>atom_sim</string>
    <key>CFBundleIdentifier</key><string>com.atomsim.app</string>
    <key>CFBundleVersion</key><string>1.0.0</string>
    <key>CFBundlePackageType</key><string>APPL</string>
    <key>LSMinimumSystemVersion</key><string>10.15</string>
</dict>
</plist>
PLIST

        echo -e "${GREEN}✓ Installed to /Applications/AtomSim.app${NC}"
        echo "Launch from Finder or: open /Applications/AtomSim.app"
        exit 0
    fi
fi

# Option 2: Manual
echo "Manual installation:"
echo "  1. Install dependencies: brew install cmake glfw glm"
echo "  2. Build: cmake -B build && cmake --build build"
echo "  3. Run: ./build/atom_sim"
echo ""
echo "Or download the pre-built .app from GitHub Releases."
