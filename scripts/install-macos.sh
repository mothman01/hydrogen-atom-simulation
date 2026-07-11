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
        brew install cmake glfw glm glew

        echo "Building..."
        SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
        cd "$SCRIPT_DIR"
        cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
        cmake --build build --parallel

        echo "Preparing .app bundle..."
        rm -rf /Applications/AtomSim.app
        cp -R build/atom_sim.app /Applications/AtomSim.app
        mkdir -p /Applications/AtomSim.app/Contents/Resources/shaders
        cp -R shaders/. /Applications/AtomSim.app/Contents/Resources/shaders/

        echo -e "${GREEN}✓ Installed to /Applications/AtomSim.app${NC}"
        echo "Launch from Finder or: open /Applications/AtomSim.app"
        exit 0
    fi
fi

# Option 2: Manual
echo "Manual installation:"
echo "  1. Install dependencies: brew install cmake glfw glm glew"
echo "  2. Build: cmake -B build && cmake --build build"
echo "  3. Run: open build/atom_sim.app"
echo ""
echo "Or download the pre-built .app from GitHub Releases."
