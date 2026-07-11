#!/usr/bin/env bash
# ──────────────────────────────────────────────────────────────────────────────
#  Build AppImage — single-file portable application for any Linux distro
#  Usage: ./make-appimage.sh
# ──────────────────────────────────────────────────────────────────────────────
set -euo pipefail

APP="AtomSim"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
APPDIR="$SCRIPT_DIR/build/AppDir"

echo "==> Building portable AppImage..."

# Clean
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/share/shaders" "$APPDIR/usr/share/applications"

# Build the project
cmake -B "$SCRIPT_DIR/build" -S "$SCRIPT_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    > /dev/null 2>&1
cmake --build "$SCRIPT_DIR/build" --parallel > /dev/null 2>&1

# Copy binary and data
cp "$SCRIPT_DIR/build/atom_sim" "$APPDIR/usr/bin/"
cp -r "$SCRIPT_DIR/shaders/"* "$APPDIR/usr/share/shaders/"

# Create AppRun entry point
cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/bash
HERE="$(dirname "$(readlink -f "$0")")"
export PATH="$HERE/usr/bin:$PATH"
exec "$HERE/usr/bin/atom_sim" "$@"
EOF
chmod +x "$APPDIR/AppRun"

# Desktop entry
cat > "$APPDIR/atom-sim.desktop" << EOF
[Desktop Entry]
Name=Atom Wavefunction Simulator
Comment=Interactive 3D quantum wavefunction visualization
Exec=atom_sim
Icon=atom-sim
Type=Application
Categories=Science;Physics;Education;
Terminal=false
EOF

# Copy desktop file to where AppImage tools expect it
cp "$APPDIR/atom-sim.desktop" "$APPDIR/usr/share/applications/"

# Check for linuxdeploy
if command -v linuxdeploy &>/dev/null; then
    echo "==> Using linuxdeploy for dependency bundling..."
    linuxdeploy --appdir "$APPDIR" --plugin gtk --output appimage 2>&1 | tail -5
    echo "==> AppImage created!"
elif command -v appimagetool &>/dev/null; then
    echo "==> Using appimagetool..."
    appimagetool "$APPDIR" "AtomSim-1.0.0-x86_64.AppImage" 2>&1
    echo "==> AppImage created!"
else
    echo ""
    echo "==> AppDir prepared at: $APPDIR"
    echo ""
    echo "To create the AppImage, install linuxdeploy or appimagetool:"
    echo "  wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    echo "  chmod +x linuxdeploy-x86_64.AppImage"
    echo "  ./linuxdeploy-x86_64.AppImage --appdir $APPDIR --output appimage"
    echo ""
    echo "Or manually:"
    echo "  tar -czf AtomSim-1.0.0-x86_64.tar.gz -C build AppDir"
    echo ""
    echo "The build/AppDir directory contains a portable copy."
    echo "Users can run it directly with:  ./build/AppDir/AppRun"
fi
