#!/usr/bin/env bash
# ──────────────────────────────────────────────────────────────────────────────
#  macOS Installer — install via Homebrew or manual .app bundle
# ──────────────────────────────────────────────────────────────────────────────
set -euo pipefail

RED='\033[0;31m'; GREEN='\033[0;32m'; BOLD='\033[1m'; NC='\033[0m'
APP="Atom Wavefunction Simulator"
BUILD_BUNDLE="build/atom_sim.app"
INSTALL_BUNDLE="/Applications/AtomSim.app"

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
        if [[ ! -d "$BUILD_BUNDLE" ]]; then
            echo -e "${RED}Error: $BUILD_BUNDLE not found. Build may have failed.${NC}"
            exit 1
        fi

        if [[ -e "$INSTALL_BUNDLE" ]]; then
            read -p "Replace existing /Applications/AtomSim.app? [Y/n]: " replace_app
            if [[ "$replace_app" == "n" || "$replace_app" == "N" ]]; then
                echo "Installation cancelled."
                exit 0
            fi
            if [[ "$INSTALL_BUNDLE" != "/Applications/AtomSim.app" ]]; then
                echo -e "${RED}Error: unexpected install path '$INSTALL_BUNDLE'.${NC}"
                exit 1
            fi
            rm -rf -- "$INSTALL_BUNDLE"
        fi
        cp -R "$BUILD_BUNDLE" "$INSTALL_BUNDLE"
        mkdir -p "$INSTALL_BUNDLE/Contents/Resources/shaders"
        cp -R shaders/. "$INSTALL_BUNDLE/Contents/Resources/shaders/"

        echo -e "${GREEN}✓ Installed to $INSTALL_BUNDLE${NC}"
        echo "Launch from Finder or: open $INSTALL_BUNDLE"
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
