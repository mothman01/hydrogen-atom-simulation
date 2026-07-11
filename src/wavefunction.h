#pragma once

#include <complex>
#include <vector>
#include <cstdint>

/**
 * Hydrogen atom wavefunction solver.
 *
 * All computations use atomic units:
 *   ħ = m_e = e = 1, a₀ = 1 (Bohr radius)
 *
 * The wavefunction for quantum numbers (n, l, m) in spherical coordinates:
 *   ψ_{nlm}(r, θ, φ) = R_{nl}(r) × Y_l^m(θ, φ)
 *
 * where:
 *   R_{nl}(r)  — radial wavefunction (real-valued)
 *   Y_l^m(θ,φ) — spherical harmonic (complex-valued)
 *
 * Probability density = |ψ|²
 */

// ---------------------------------------------------------------------------
// Associated Laguerre polynomials  L_n^k(x)
// ---------------------------------------------------------------------------
double associatedLaguerre(int n, int k, double x);

// ---------------------------------------------------------------------------
// Associated Legendre polynomials  P_l^m(x)  (x = cos θ)
// Uses the Condon–Shortley phase:  (-1)^m
// ---------------------------------------------------------------------------
double associatedLegendre(int l, int m, double x);

// ---------------------------------------------------------------------------
// Normalised spherical harmonic  Y_l^m(θ, φ)
// ---------------------------------------------------------------------------
std::complex<double> sphericalHarmonic(int l, int m, double theta, double phi);

// ---------------------------------------------------------------------------
// Radial wavefunction  R_{nl}(r)  for the hydrogen atom.
// Always real-valued.
// ---------------------------------------------------------------------------
double radialWavefunction(int n, int l, double r);

// ---------------------------------------------------------------------------
// Hydrogenic radial wavefunction with nuclear charge Z.
// Uses scaled coordinate ρ = 2Zr/n.
// ---------------------------------------------------------------------------
double radialWavefunctionZ(int n, int l, double r, int Z);

// ---------------------------------------------------------------------------
// Full 3-D hydrogen wavefunction  ψ_{nlm}(x, y, z)
// Handles the r = 0 corner case correctly.
// Returns 0 if quantum numbers are invalid.
// ---------------------------------------------------------------------------
std::complex<double> wavefunction(int n, int l, int m,
                                  double x, double y, double z);

// ---------------------------------------------------------------------------
// Hydrogenic wavefunction with nuclear charge Z.
// ---------------------------------------------------------------------------
std::complex<double> wavefunctionZ(int n, int l, int m,
                                   double x, double y, double z, int Z);

// ---------------------------------------------------------------------------
// Probability density  |ψ_{nlm}(x, y, z)|²
// ---------------------------------------------------------------------------
double probabilityDensity(int n, int l, int m,
                          double x, double y, double z);

// ---------------------------------------------------------------------------
// Hydrogenic probability density with nuclear charge Z.
// ---------------------------------------------------------------------------
double probabilityDensityZ(int n, int l, int m,
                           double x, double y, double z, int Z);

// ---------------------------------------------------------------------------
// Pre-compute a 3-D grid of probability density values.
// The grid spans [-halfSize, +halfSize] in each dimension with `resolution`
// samples per axis (total = resolution³ floats).
//
// The resulting data vector is indexed as:
//   data[iz * resolution² + iy * resolution + ix]
//
// After calling, the data is auto-normalised so max(density) = 1.0
// for optimal rendering.
// ---------------------------------------------------------------------------
void computeVolumeData(int n, int l, int m,
                       int resolution, double halfSize,
                       std::vector<float> &data);

// ---------------------------------------------------------------------------
// Suggested bounding box half-size for a given principal quantum number n.
// Covers > 99.9 % of the integrated probability.
// ---------------------------------------------------------------------------
double suggestedHalfSize(int n);

// ---------------------------------------------------------------------------
// Orbital labels and descriptions for UI
// ---------------------------------------------------------------------------
struct OrbitalInfo {
    int n, l, m;
    const char *label;       // e.g. "1s"
    const char *description; // e.g. "Ground state — spherical"
};

// All supported orbitals (n ≤ 4, l < n, |m| ≤ l)
extern const OrbitalInfo ORBITALS[];
extern const int NUM_ORBITALS;
