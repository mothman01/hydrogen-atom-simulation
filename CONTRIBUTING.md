# Contributing to Atom Wavefunction Simulator

Thank you for your interest in contributing! This document explains how to get
started, the development workflow, and the conventions used in this project.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Reporting Bugs](#reporting-bugs)
- [Requesting Features](#requesting-features)
- [Development Workflow](#development-workflow)
- [Coding Conventions](#coding-conventions)
- [Submitting a Pull Request](#submitting-a-pull-request)

---

## Code of Conduct

This project follows the [Contributor Covenant Code of Conduct](CODE_OF_CONDUCT.md).
By participating you are expected to uphold this code.

---

## Getting Started

### Prerequisites

| Tool | Minimum version |
|------|----------------|
| CMake | 3.16 |
| C++ compiler | C++17 (GCC 9+, Clang 10+, MSVC 2019+) |
| GLFW | 3.x |
| GLM | any recent |
| GLEW | any recent |
| OpenGL | 3.3+ |

### Linux (Ubuntu / Debian)

```bash
sudo apt install cmake g++ libglfw3-dev libglm-dev libgl-dev libglew-dev
git clone https://github.com/mothman01/hydrogen-atom-simulation.git
cd hydrogen-atom-simulation
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/atom_sim
```

### macOS

```bash
brew install cmake glfw glm glew
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
open build/atom_sim.app
```

### Windows

```powershell
vcpkg install glfw3 glm glew
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
.\build\Release\atom_sim.exe
```

---

## Reporting Bugs

Use the **Bug Report** issue template. Please include:

- OS, GPU, and OpenGL version
- Steps to reproduce the issue
- What you expected vs. what actually happened
- Any relevant console output or screenshots

---

## Requesting Features

Use the **Feature Request** issue template. Describe the problem your feature
would solve and any implementation ideas you have in mind.

---

## Development Workflow

1. Fork the repository and create a branch from `main`:
   ```bash
   git checkout -b feat/your-feature-name
   ```
2. Make your changes in small, focused commits.
3. Build and test locally before opening a pull request:
   ```bash
   cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
   cmake --build build --parallel
   ./build/atom_sim   # smoke-test the simulator
   ```
4. Push your branch and open a pull request against `main`.

---

## Coding Conventions

- **C++17** throughout; no compiler extensions.
- 4-space indentation, no tabs.
- Header guards use `#pragma once`.
- Keep GLSL shader files in `shaders/` and C++ source in `src/`.
- New physics or rendering features should be self-contained in their own
  `.h`/`.cpp` pair following the existing style (see `wavefunction.h/.cpp`).
- Avoid adding new third-party dependencies without discussion in an issue first.

---

## Submitting a Pull Request

- Fill in the pull request template completely.
- Ensure the CI build passes on all three platforms (Linux, macOS, Windows).
- Keep diffs focused — one logical change per PR.
- Be responsive to review feedback; unreviewed PRs idle for 30 days may be closed.

Thank you for helping make this project better!
