# Atom Wavefunction Simulator

**Interactive 3D quantum wavefunction visualization with a clickable periodic table.**

Supports all 118 elements using the hydrogenic approximation. Select any element
from the periodic table, choose an orbital, and explore the electron probability
density cloud in real-time with GPU-accelerated volume raymarching.

[![Build](https://github.com/mothman01/hydrogen-atom-simulation/actions/workflows/build.yml/badge.svg)](https://github.com/mothman01/hydrogen-atom-simulation/actions)

---

## Download

One click, no installer, nothing else to install:

| Platform | Download | Then |
|---|---|---|
| 🪟 **Windows 10/11** (x64) | [**AtomSim-Windows-x64.exe**](https://github.com/mothman01/hydrogen-atom-simulation/releases/latest/download/AtomSim-Windows-x64.exe) | Double-click it |
| 🍎 **macOS 10.15+** (Apple Silicon & Intel) | [**AtomSim-macOS-universal.zip**](https://github.com/mothman01/hydrogen-atom-simulation/releases/latest/download/AtomSim-macOS-universal.zip) | Unzip, then open **AtomSim.app** |
| 🐧 **Linux** (x86-64, any distro) | [**AtomSim-Linux-x86_64.AppImage**](https://github.com/mothman01/hydrogen-atom-simulation/releases/latest/download/AtomSim-Linux-x86_64.AppImage) | `chmod +x` it (or tick *Allow executing* in Properties), then double-click |

Alternative Linux download without AppImage/FUSE:
[AtomSim-Linux-x86_64.tar.gz](https://github.com/mothman01/hydrogen-atom-simulation/releases/latest/download/AtomSim-Linux-x86_64.tar.gz).
All versions: [Releases page](https://github.com/mothman01/hydrogen-atom-simulation/releases).

> **First launch warnings.** The downloads are not code-signed, so the OS asks once:
> - **Windows SmartScreen:** click **More info → Run anyway**.
> - **macOS Gatekeeper:** right-click **AtomSim.app → Open → Open** (on macOS 15+,
>   open it once, then go to **System Settings → Privacy & Security → Open Anyway**).

### Or build from source

Only CMake ≥ 3.16 and a C++17 compiler are required — GLFW, GLEW and GLM are
used from your system if installed, otherwise downloaded and built automatically.

```bash
git clone https://github.com/mothman01/hydrogen-atom-simulation.git
cd hydrogen-atom-simulation
cmake -B build && cmake --build build --config Release --parallel
```

Run `./build/atom_sim` (Linux), `open build/AtomSim.app` (macOS) or
`build\Release\atom_sim.exe` (Windows).

Linux builds of GLFW from source need the X11/Wayland headers, e.g. on Ubuntu:
`sudo apt install xorg-dev libwayland-dev libxkbcommon-dev wayland-protocols`
(or just `sudo apt install libglfw3-dev libglew-dev libglm-dev` to use system packages).

Add `-DATOM_SIM_PORTABLE=ON` to produce the same fully self-contained binary as
the release downloads. Installers: `./install-gui.sh` (Linux),
`scripts/install-macos.sh`, `scripts/install-windows.ps1`.

## Controls

| Key | Action |
|---|---|
| **Click element** | Select it and show its valence orbital |
| `SPACE` | Show selected element with the current orbital |
| `←` / `→` (`A`/`D`) | Previous / next orbital |
| `↑` / `↓` (`W`/`S`) | Jump to next / previous shell *n* |
| `1`–`9` | Direct orbital selection |
| `ENTER` | Add another atom (in molecule mode: bond a new atom) |
| `DEL` / `BACKSPACE` | Remove last atom |
| `C` | Clear all atoms |
| `E` | Toggle energy-level diagram (click a level to select it) |
| `L` / `K` | Toggle crystal-lattice mode / cycle lattice type |
| `M` | Toggle molecule mode |
| `T` | Toggle time evolution (1s + 2p superposition) |
| **Mouse drag** | Rotate camera |
| **Scroll** | Zoom in / out |
| `+` / `-` | Opacity scale |
| `R` | Toggle auto-rotate |
| `TAB` | Toggle periodic table |
| `H` | Toggle on-screen help |
| `F12` | Screenshot (PNG, saved to Pictures) |
| `ESC` | Quit |

## Physics

The wavefunction for quantum numbers (n, l, m) with nuclear charge Z:

```
ψ_{nlm}(r, θ, φ) = R_{nl}(r, Z) × Y_l^m(θ, φ)
```

- Radial part: hydrogenic with (fractional) effective nuclear charge Z<sub>eff</sub>
- Angular part: complex spherical harmonics with Condon–Shortley phase
- Probability density: |ψ|² rendered as translucent electron cloud
- Energy levels: E<sub>n</sub> = −13.6 × Z²/n² eV (approximate)
- Time evolution: |c₁ψ₁e<sup>−iE₁t</sup> + c₂ψ₂e<sup>−iE₂t</sup>|², slowed so one beat lasts 3 s

## System Requirements

| | Minimum |
|---|---|
| **OS** | Windows 10, macOS 10.15, Linux with glibc 2.35+ (Ubuntu 22.04, Fedora 36, …) |
| **GPU** | OpenGL 3.3 core profile (virtually any GPU from 2010+) |
| **RAM** | 256 MB |
| **Disk** | ~2 MB |

No internet connection required. No external data files needed.

## Testing

```bash
ctest --test-dir build -C Release      # headless physics tests (all orbitals, all 118 elements)
./build/atom_sim --selftest            # renders every element and mode, exits non-zero on GL errors
```

## Releasing

Push a version tag — CI builds, tests and publishes all downloads:

```bash
git tag v2.0.0 && git push origin v2.0.0
```

## Project Structure

```
hydrogen-atom-simulation/
├── CMakeLists.txt
├── .github/workflows/build.yml   # CI + release packaging for all 3 OSes
├── shaders/                      # GLSL sources (embedded into the binary at build time)
│   ├── screen_quad.vert
│   └── raymarch.frag
├── tests/test_physics.cpp        # headless orbital / element tests
└── src/
    ├── main.cpp                  # GLFW window, camera, periodic table, controls
    ├── element_data.h/.cpp       # All 118 elements with properties
    ├── wavefunction.h/.cpp       # Hydrogenic wavefunction solver + Z scaling
    ├── parallel.h                # std::thread parallel-for
    ├── atom_scene.h/.cpp         # Multi-atom scene management
    ├── crystal_lattice.h/.cpp    # Lattice generators (SC, BCC, FCC, diamond, …)
    ├── molecular.h/.cpp          # LCAO molecule mode
    ├── time_evolution.h/.cpp     # Two-state superposition animation
    ├── energy_diagram.h/.cpp     # Energy-level overlay
    ├── text_renderer.h/.cpp      # stb_truetype text using a system font
    ├── screenshot.h/.cpp         # Built-in PNG writer
    ├── shader.h/.cpp             # GLSL shader compiler / linker
    └── volume_renderer.h/.cpp    # 3D texture manager + raymarch renderer
```
