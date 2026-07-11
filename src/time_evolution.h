#pragma once

#include "wavefunction.h"
#include <glm/glm.hpp>
#include <vector>
#include <complex>

/**
 * Time-dependent Schrödinger equation animation.
 *
 * Constructs a superposition of two hydrogenic eigenstates:
 *
 *   ψ(r, t) = c₁ ψ₁(r) e^{-iE₁t/ħ}  +  c₂ ψ₂(r) e^{-iE₂t/ħ}
 *
 * The probability density |ψ(r,t)|² oscillates at the beat frequency
 *  ω = (E₂ - E₁) / ħ, producing a visible breathing / wobbling effect.
 *
 * The result is uploaded to a 3-D texture each frame, so the GPU
 * raymarcher renders the time-evolving cloud directly.
 */
namespace TimeEvolution {

// Configuration
struct State {
    int n1, l1, m1;  // first eigenstate
    int n2, l2, m2;  // second eigenstate
    double c1;         // amplitude of ψ₁  (c₂ = sqrt(1 - c₁²))
    double phaseOffset; // initial phase difference (radians)
};

// Build / rebuild volume data for the current time `t` (seconds).
// Populates `data` (resolution³ floats) with |ψ(r,t)|².
// Z is the nuclear charge.
void computeFrame(const State& st, int Z,
                  double t, int resolution, double halfSize,
                  std::vector<float>& data);

// Whether time evolution is currently active
extern bool enabled;
extern State currentState;

// Reset to default superposition (2s + 2p beating)
void reset();

// Get the beat period (seconds) for the current superposition
double beatPeriod(int Z);

} // namespace TimeEvolution
