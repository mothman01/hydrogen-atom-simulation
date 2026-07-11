#!/usr/bin/env bash
# Linux installer — run from project root
set -euo pipefail

PREFIX="${1:-/usr/local}"
echo "Installing Atom Wavefunction Simulator to $PREFIX..."

cd "$(dirname "$0")"

cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX"
cmake --build build --parallel
sudo cmake --install build

echo ""
echo "Done! Run: atom_sim"
echo "Or find 'Atom Wavefunction Simulator' in your app menu."
