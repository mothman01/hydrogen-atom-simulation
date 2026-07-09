# Hydrogen Atom — Schrödinger Wavefunction Simulation

**Real-time 3D volume rendering of hydrogen electron orbital probability densities.**

Built with C++17, OpenGL 4.6, GLFW, and GLM. All physics computed from the exact
hydrogen wavefunction solution to the Schrödinger equation.

![1s orbital — spherical ground state electron cloud](screenshot_1s.png)

---

## Physics

The simulation solves the time-independent Schrödinger equation for the
hydrogen atom in **atomic units** (ħ = mₑ = e = 1, a₀ = 1):

```
ψ_{nlm}(r, θ, φ) = R_{nl}(r) · Y_l^m(θ, φ)
```

| Ingredient | Implementation |
|---|---|
| **Radial wavefunction** Rₙₗ(r) | Associated Laguerre polynomials Lₙ₋ₗ₋₁²ˡ⁺¹(2r/n) |
| **Angular wavefunction** Yₗᵐ(θ,φ) | Complex spherical harmonics with Condon–Shortley phase |
| **Probability density** \|ψ\|² | Norm of the full complex wavefunction |
| **Energy levels** | Eₙ = −13.6 eV / n² |

### Supported Orbitals (n ≤ 4)

- **n=1:** 1s
- **n=2:** 2s, 2p₀, 2p₁
- **n=3:** 3s, 3p₀, 3p₁, 3d₀, 3d₁, 3d₂
- **n=4:** 4s, 4p₀, 4p₁, 4d₀, 4d₁, 4d₂, 4f₀, 4f₁, 4f₂, 4f₃

## Rendering

GPU-accelerated **raymarching** through a precomputed 128³ 3D texture of
probability density values:

- Ray–AABB intersection bounding
- Trilinear texture sampling
- Central-differences gradient for Phong lighting (ambient + diffuse + specular)
- Blue → cyan → white transfer function with power-law opacity
- Front-to-back compositing with early ray termination

## Controls

| Key | Action |
|---|---|
| `←` `→` / `A` `D` | Previous / next orbital |
| `W` `S` | Jump to next / previous principal quantum number n |
| `1`–`9`, `0` | Direct orbital selection |
| **Mouse drag** | Rotate camera |
| **Scroll** | Zoom in / out |
| `+`/`-` | Opacity scale |
| `Q`/`E` | Exposure (brightness) |
| `R` | Toggle auto-rotate |
| `Space` | Reset camera |
| `Esc` | Quit |

## Building

**Dependencies:** CMake ≥ 3.16, a C++17 compiler, GLFW 3, GLM, OpenGL

### Fedora
```bash
sudo dnf install cmake gcc-c++ glfw-devel glm-devel mesa-libGL-devel
cmake -B build -S .
cmake --build build
./build/hydrogen_sim
```

### Ubuntu / Debian
```bash
sudo apt install cmake g++ libglfw3-dev libglm-dev
cmake -B build -S .
cmake --build build
./build/hydrogen_sim
```

## Project Structure

```
quantum_particle_sim/
├── CMakeLists.txt
├── shaders/
│   ├── screen_quad.vert      # Full-screen triangle vertex shader
│   └── raymarch.frag         # GPU volume raymarching + lighting
└── src/
    ├── main.cpp              # GLFW window, orbit camera, controls
    ├── wavefunction.h/.cpp   # Full hydrogen wavefunction solver
    ├── shader.h/.cpp         # GLSL shader compiler / linker
    └── volume_renderer.h/.cpp # 3D texture manager + raymarch renderer
```

---

*This project was built with assistance from the Zed AI coding agent.*
