#!/usr/bin/env bash
# ──────────────────────────────────────────────────────────────────────────────
#  Atom Wavefunction Simulator — Graphical Installer
#  Double-click or run:  ./install-gui.sh
# ──────────────────────────────────────────────────────────────────────────────
set -euo pipefail

APP_NAME="Atom Wavefunction Simulator"
VERSION="1.0.0"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
INSTALL_DIR="${HOME}/.local/share/atom-sim"
BIN_DIR="${HOME}/.local/bin"
DESKTOP_DIR="${HOME}/.local/share/applications"

# ── Colors ───────────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
BLUE='\033[0;34m'; BOLD='\033[1m'; NC='\033[0m'

# ── Detect GUI availability ──────────────────────────────────────────────────
has_gui() { command -v zenity &>/dev/null; }
has_kdialog() { command -v kdialog &>/dev/null; }

msg() {
    if has_gui; then
        case "$1" in
            info)    zenity --info --title="$APP_NAME" --text="$2" --width=450 2>/dev/null ;;
            warn)    zenity --warning --title="$APP_NAME" --text="$2" --width=450 2>/dev/null ;;
            error)   zenity --error --title="$APP_NAME" --text="$2" --width=450 2>/dev/null ;;
            question) zenity --question --title="$APP_NAME" --text="$2" --width=450 2>/dev/null ;;
            progress) zenity --progress --title="$APP_NAME" --text="$2" --pulsate --auto-close --width=450 2>/dev/null ;;
        esac
    elif has_kdialog; then
        case "$1" in
            info)    kdialog --title="$APP_NAME" --msgbox "$2" 2>/dev/null ;;
            warn)    kdialog --title="$APP_NAME" --sorry "$2" 2>/dev/null ;;
            error)   kdialog --title="$APP_NAME" --error "$2" 2>/dev/null ;;
            question) kdialog --title="$APP_NAME" --yesno "$2" 2>/dev/null ;;
            progress) kdialog --title="$APP_NAME" --progressbar "$2" 10 2>/dev/null ;;
        esac
    else
        case "$1" in
            info)    echo -e "${GREEN}▶${NC} $2" ;;
            warn)    echo -e "${YELLOW}⚠${NC} $2" ;;
            error)   echo -e "${RED}✖${NC} $2" ;;
            question) echo -e "${BLUE}?${NC} $2 [Y/n]" ;;
        esac
    fi
}

ask() {
    if has_gui; then
        zenity --question --title="$APP_NAME" --text="$1" --width=450 2>/dev/null
    elif has_kdialog; then
        kdialog --title="$APP_NAME" --yesno "$1" 2>/dev/null
    else
        echo -e "${BLUE}?${NC} $1 [Y/n]"
        read -r answer
        [[ "$answer" != "n" && "$answer" != "N" ]]
    fi
}

# ── Check dependencies ───────────────────────────────────────────────────────
check_dep() {
    command -v "$1" &>/dev/null && return 0 || return 1
}

MISSING_DEPS=""
for dep in cmake g++ pkg-config; do
    check_dep "$dep" || MISSING_DEPS="$MISSING_DEPS $dep"
done

# Check for GLFW, GLM, OpenGL development headers
pkg-config --exists glfw3 2>/dev/null || MISSING_DEPS="$MISSING_DEPS glfw3-devel"
pkg-config --exists glm 2>/dev/null || MISSING_DEPS="$MISSING_DEPS glm-devel"

# ── Welcome ──────────────────────────────────────────────────────────────────
echo -e "${BOLD}${GREEN}╔══════════════════════════════════════════════╗${NC}"
echo -e "${BOLD}${GREEN}║   ${APP_NAME} v${VERSION} Installer       ║${NC}"
echo -e "${BOLD}${GREEN}╚══════════════════════════════════════════════╝${NC}"
echo ""

msg info "Welcome to the ${APP_NAME} v${VERSION} installer!\n\nThis will install the application on your system.\n\nFeatures:\n  • Interactive 3D quantum wavefunctions\n  • All 118 elements with periodic table\n  • GPU-accelerated volume rendering\n  • 20 hydrogen orbitals (s, p, d, f)"

# ── Handle missing dependencies ──────────────────────────────────────────────
if [ -n "$MISSING_DEPS" ]; then
    echo -e "${YELLOW}Missing dependencies:${NC}$MISSING_DEPS"
    msg warn "Missing dependencies detected:$MISSING_DEPS\n\nThese are required to build the application."

    if ask "Would you like to install the missing dependencies now?\n\n(sudo password may be required)"; then
        echo -e "${BLUE}Installing dependencies...${NC}"

        # Detect package manager
        if command -v dnf &>/dev/null; then
            PKG_MGR="sudo dnf install -y"
        elif command -v apt-get &>/dev/null; then
            sudo apt-get update -qq
            PKG_MGR="sudo apt-get install -y"
        elif command -v pacman &>/dev/null; then
            PKG_MGR="sudo pacman -S --noconfirm"
        elif command -v zypper &>/dev/null; then
            PKG_MGR="sudo zypper install -y"
        else
            msg error "Could not detect package manager.\nPlease install these manually:$MISSING_DEPS"
            echo -e "${RED}Cannot auto-install. Please install:$MISSING_DEPS${NC}"
            exit 1
        fi

        # Map generic names to distro-specific packages
        for dep in $MISSING_DEPS; do
            case "$dep" in
                glfw3-devel)
                    if command -v dnf &>/dev/null; then
                        $PKG_MGR glfw-devel 2>&1 | tail -1
                    elif command -v apt-get &>/dev/null; then
                        $PKG_MGR libglfw3-dev 2>&1 | tail -1
                    else
                        $PKG_MGR glfw 2>&1 | tail -1
                    fi
                    ;;
                glm-devel)
                    if command -v dnf &>/dev/null; then
                        $PKG_MGR glm-devel 2>&1 | tail -1
                    elif command -v apt-get &>/dev/null; then
                        $PKG_MGR libglm-dev 2>&1 | tail -1
                    else
                        $PKG_MGR glm 2>&1 | tail -1
                    fi
                    ;;
                *) $PKG_MGR "$dep" 2>&1 | tail -1 ;;
            esac
        done

        msg info "Dependencies installed successfully!"
    else
        echo -e "${YELLOW}Skipping dependency installation. Build may fail.${NC}"
    fi
fi

# ── Build ────────────────────────────────────────────────────────────────────
echo ""
echo -e "${BOLD}Building ${APP_NAME}...${NC}"
msg progress "Building ${APP_NAME}..."

cd "$SCRIPT_DIR"
mkdir -p build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" > /dev/null 2>&1

if ! cmake --build build --parallel > /dev/null 2>&1; then
    msg error "Build failed!\n\nPlease check that all dependencies are installed:\n  • CMake 3.16+\n  • C++17 compiler (g++ or clang++)\n  • GLFW 3 development files\n  • GLM development files\n  • OpenGL development files\n\nSee README.md for detailed instructions."
    exit 1
fi

echo -e "${GREEN}✓ Build successful${NC}"

# ── Install ──────────────────────────────────────────────────────────────────
echo ""
echo -e "${BOLD}Installing...${NC}"

mkdir -p "$INSTALL_DIR" "$BIN_DIR" "$DESKTOP_DIR"

# Install files
cmake --install build > /dev/null 2>&1 || {
    # Manual install fallback
    cp build/atom_sim "$INSTALL_DIR/"
    cp -r shaders "$INSTALL_DIR/"
}

# Create launcher script
cat > "$BIN_DIR/atom-sim" << EOF
#!/bin/bash
exec "$INSTALL_DIR/bin/atom_sim" "\$@"
EOF
chmod +x "$BIN_DIR/atom-sim"

# Desktop entry
cat > "$DESKTOP_DIR/atom-sim.desktop" << EOF
[Desktop Entry]
Name=Atom Wavefunction Simulator
Comment=Interactive 3D quantum wavefunction visualization
Exec=$BIN_DIR/atom-sim
Icon=atom-sim
Type=Application
Categories=Science;Physics;Education;
Terminal=false
StartupNotify=true
EOF

echo -e "${GREEN}✓ Installation complete${NC}"

# ── Done ─────────────────────────────────────────────────────────────────────
echo ""
echo -e "${BOLD}${GREEN}╔══════════════════════════════════════════════╗${NC}"
echo -e "${BOLD}${GREEN}║         Installation Complete!                ║${NC}"
echo -e "${BOLD}${GREEN}╚══════════════════════════════════════════════╝${NC}"
echo ""
echo -e "  ${BOLD}Launch from terminal:${NC}  atom-sim"
echo -e "  ${BOLD}Launch from app menu:${NC}  ${APP_NAME}"
echo ""

msg info "Installation complete!\n\nLaunch from terminal:  atom-sim\nOr find '${APP_NAME}' in your application menu."

if ask "Would you like to launch ${APP_NAME} now?"; then
    "$INSTALL_DIR/bin/atom_sim" &
fi
