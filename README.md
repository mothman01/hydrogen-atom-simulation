# Atom Wavefunction Simulator

**Interactive 3D quantum wavefunction visualization with a clickable periodic table.**

Supports all 118 elements using the hydrogenic approximation. Select any element
from the periodic table, choose an orbital, and explore the electron probability
density cloud in real-time with GPU-accelerated volume raymarching.

[![Build](https://github.com/mothman01/hydrogen-atom-simulation/actions/workflows/build.yml/badge.svg)](https://github.com/mothman01/hydrogen-atom-simulation/actions)

---

## Download

**[Download the latest release →](https://github.com/mothman01/hydrogen-atom-simulation/releases)**

Pre-built binaries for Linux, macOS, and Windows are available on the Releases page.
Choose the zip for your platform, extract, and run.

| Platform | File |
|---|---|
| 🐧 **Linux** | `atom-sim-linux-x86_64.zip` |
| 🍎 **macOS** | `atom-sim-macos-arm64.zip` |
| 🪟 **Windows** | `atom-sim-windows-x86_64.zip` |

### Or build from source

```bash
git clone https://github.com/mothman01/hydrogen-atom-simulation.git
cd hydrogen-atom-simulation

# Linux — graphical installer (double-click)
./install-gui.sh

# Or one-liner build
cmake -B build && cmake --build build && ./build/atom_sim
```
| `1`–`9` | Direct orbital selection |
| **Mouse drag** | Rotate camera |
| **Scroll** | Zoom in / out |
| `+`/`-` | Opacity scale |
| `R` | Toggle auto-rotate |
| **TAB** | Toggle periodic table |
| **ESC** | Quit |

## Physics

The wavefunction for quantum numbers (n, l, m) with nuclear charge Z:

```
ψ_{nlm}(r, θ, φ) = R_{nl}(r, Z) × Y_l^m(θ, φ)
```

- Radial part: hydrogenic with effective nuclear charge Z<sub>eff</sub>
- Angular part: complex spherical harmonics with Condon–Shortley phase
- Probability density: |ψ|² rendered as translucent electron cloud
- Energy levels: E<sub>n</sub> = −13.6 × Z²/n² eV (approximate)

## System Requirements

| | Minimum | Recommended |
|---|---|---|
| **OS** | Linux, macOS 10.15+, Windows 10+ | Linux (any distro) |
| **CPU** | Any x86-64 with SSE2 | 2+ cores |
| **GPU** | OpenGL 3.3+ (any GPU from 2010+) | OpenGL 4.0+ |
| **RAM** | 256 MB | 512 MB |
| **Disk** | 10 MB | 20 MB |
| **Dependencies** | CMake 3.16+, C++17 compiler, GLFW 3, GLM, OpenGL | — |

No internet connection required after build. No external data files needed.

## Building

### Dependencies

- **CMake** ≥ 3.16
- **C++17** compiler
- **GLFW 3**
- **GLM**
- **OpenGL** 3.3+

### Linux

```bash
# Fedora
sudo dnf install cmake gcc-c++ glfw-devel glm-devel mesa-libGL-devel
cmake -B build -S . && cmake --build build --parallel
./build/atom_sim

# Ubuntu / Debian
sudo apt install cmake g++ libglfw3-dev libglm-dev
cmake -B build -S . && cmake --build build --parallel
./build/atom_sim

# Install system-wide
./install.sh
```

### macOS

```bash
brew install cmake glfw glm
cmake -B build -S . && cmake --build build --parallel
open build/atom_sim.app
```

### Windows

```powershell
# Using vcpkg
vcpkg install glfw3 glm
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
.\build\Release\atom_sim.exe
```

## Project Structure

```
quantum_particle_sim/
├── CMakeLists.txt
├── LICENSE
├── README.md
├── install.sh
├── assets/
│   └── atom-sim.desktop
├── shaders/
│   ├── screen_quad.vert      # Full-screen triangle vertex shader
│   └── raymarch.frag         # GPU volume raymarching + lighting
└── src/
    ├── main.cpp              # GLFW window, camera, periodic table, controls
    ├── element_data.h/.cpp   # All 118 elements with properties
    ├── wavefunction.h/.cpp   # Hydrogenic wavefunction solver + Z scaling
    ├── atom_scene.h/.cpp     # Multi-atom scene management
    ├── shader.h/.cpp         # GLSL shader compiler / linker
    └── volume_renderer.h/.cpp # 3D texture manager + raymarch renderer
```
