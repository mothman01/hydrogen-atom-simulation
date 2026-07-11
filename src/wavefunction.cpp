#include "wavefunction.h"

#include <cmath>
#include <algorithm>
#include <complex>
#include <stdexcept>
#include <cassert>

using namespace std::complex_literals;

// ============================================================================
//  Associated Laguerre polynomials  L_n^k(x)
//
//  Recurrence (Abramowitz & Stegun 22.7.12):
//    L_0^k(x) = 1
//    L_1^k(x) = 1 + k – x
//    (j+1) L_{j+1}^k(x) = (2j + k + 1 – x) L_j^k(x) – (j + k) L_{j-1}^k(x)
// ============================================================================
double associatedLaguerre(int n, int k, double x)
{
    if (n < 0 || k < 0) return 0.0;
    if (n == 0) return 1.0;

    double L_prev = 1.0;                     // L_0^k
    double L_curr = 1.0 + static_cast<double>(k) - x; // L_1^k

    for (int j = 1; j < n; ++j) {
        double L_next = ((2.0 * j + k + 1.0 - x) * L_curr
                         - (j + k) * L_prev) / (j + 1.0);
        L_prev = L_curr;
        L_curr = L_next;
    }
    return L_curr;
}

// ============================================================================
//  Associated Legendre polynomials  P_l^m(x)   with  x = cos θ
//
//  Uses the Condon–Shortley phase  (-1)^m  consistent with most QM texts.
//
//  Recurrence (stable for |x| ≤ 1):
//    P_0^0  = 1
//    P_l^l  = (-1)^l · (2l–1)!! · (1–x²)^{l/2}
//    P_l^{l-1} = x · (2l–1) · P_{l-1}^{l-1}
//    P_l^m = [ (2l–1) x P_{l-1}^m  –  (l+m–1) P_{l-2}^m ] / (l–m)
// ============================================================================
double associatedLegendre(int l, int m, double x)
{
    // Only defined for  0 ≤ m ≤ l
    if (m < 0 || m > l || l < 0) return 0.0;

    // Small-number guard for the sin^m factor
    double sinTheta = std::sqrt(std::max(0.0, 1.0 - x * x));

    // --- build diagonal: P_m^m = (-1)^m (2m-1)!! sin^m θ ----------------
    double p_mm = 1.0;
    if (m > 0) {
        double sign = (m % 2 == 0) ? 1.0 : -1.0;  // Condon–Shortley
        double df   = 1.0;                         // double factorial (2m-1)!!
        for (int i = 1; i <= m; ++i) {
            // (2i-1)!!
            if (i > 1) df *= 2.0 * i - 1.0;
            p_mm *= sinTheta;
        }
        p_mm *= sign * df;
    }

    if (l == m) return p_mm;

    // --- P_{m+1}^m = x · (2m+1) · P_m^m --------------------------------
    double p_prev = p_mm;
    double p_curr = x * (2.0 * m + 1.0) * p_mm;

    for (int ll = m + 2; ll <= l; ++ll) {
        double p_next = ((2.0 * ll - 1.0) * x * p_curr
                         - (ll + m - 1.0) * p_prev) / (ll - m);
        p_prev = p_curr;
        p_curr = p_next;
    }
    return p_curr;
}

// ============================================================================
//  Spherical harmonic  Y_l^m(θ, φ)
//
//  Y_l^m(θ,φ) = (-1)^m  √[ (2l+1)/(4π) · (l–m)!/(l+m)! ]  P_l^m(cos θ)  e^{i m φ}
//
//  The (-1)^m in the normalisation is the Condon–Shortley phase and is
//  *already* included in our associatedLegendre above, so we do NOT
//  multiply again here (i.e. our P_l^m already carries (-1)^m).
//
//  Phase convention for m < 0:
//    Y_l^{-|m|} = (-1)^|m|  [Y_l^{|m|}]*
// ============================================================================
std::complex<double> sphericalHarmonic(int l, int m, double theta, double phi)
{
    if (l < 0 || std::abs(m) > l) return 0.0;

    int absM = std::abs(m);

    // Normalisation factor
    double norm = std::sqrt((2.0 * l + 1.0) / (4.0 * M_PI));
    // Ratio of factorials: (l-|m|)!/(l+|m|)!
    double factRatio = 1.0;
    for (int i = l - absM + 1; i <= l + absM; ++i) factRatio /= i;
    norm *= std::sqrt(factRatio);

    double Plm = associatedLegendre(l, absM, std::cos(theta));

    if (m >= 0) {
        return std::complex<double>(norm * Plm * std::cos(m * phi),
                                    norm * Plm * std::sin(m * phi));
    } else {
        // Y_l^{-|m|} = (-1)^|m|  (Y_l^{|m|})*
        double sign = (absM % 2 == 0) ? 1.0 : -1.0;
        return std::complex<double>( sign * norm * Plm * std::cos(absM * phi),
                                    -sign * norm * Plm * std::sin(absM * phi));
    }
}

// ============================================================================
//  Radial wavefunction  R_{nl}(r)
//
//  R_{nl}(r) = √[ (2/n)³ · (n–l–1)! / (2n · (n+l)!) ]
//              · (2r/n)^l
//              · L_{n–l–1}^{2l+1}(2r/n)
//              · exp(–r/n)
//
//  All in atomic units (a₀ = 1).
// ============================================================================
double radialWavefunction(int n, int l, double r)
{
    if (n < 1 || l < 0 || l >= n) return 0.0;
    if (r < 0.0) return 0.0;

    double rho = 2.0 * r / n;

    // Normalisation constant
    double norm = 2.0 / n;
    norm = norm * norm * norm;                     // (2/n)³
    norm /= (2.0 * n);                              // ÷ 2n

    // (n-l-1)! / (n+l)!
    double factRatio = 1.0;
    for (int i = n - l; i <= n + l; ++i) factRatio /= i;
    norm *= factRatio;
    norm = std::sqrt(norm);

    double result = norm;
    if (l > 0) result *= std::pow(rho, l);          // (2r/n)^l
    result *= associatedLaguerre(n - l - 1, 2 * l + 1, rho);
    result *= std::exp(-r / n);

    return result;
}

// ============================================================================
//  Full wavefunction  ψ_{nlm}(x, y, z)
// ============================================================================
std::complex<double> wavefunction(int n, int l, int m,
                                   double x, double y, double z)
{
    if (n < 1 || l < 0 || l >= n || std::abs(m) > l) return 0.0;

    double r = std::sqrt(x*x + y*y + z*z);

    if (r < 1e-12) {
        // At the origin only s-orbitals (l=0) are non-zero.
        // Y_0^0 = 1/√(4π)
        if (l == 0) {
            return radialWavefunction(n, 0, 0.0) / std::sqrt(4.0 * M_PI);
        }
        return 0.0;
    }

    double theta = std::acos(z / r);
    double phi   = std::atan2(y, x);

    double R = radialWavefunction(n, l, r);
    auto   Y = sphericalHarmonic(l, m, theta, phi);

    return R * Y;
}

// ============================================================================
//  Probability density  |ψ|²
// ============================================================================
double probabilityDensity(int n, int l, int m,
                           double x, double y, double z)
{
    auto psi = wavefunction(n, l, m, x, y, z);
    return std::norm(psi);
}

// ============================================================================
//  Hydrogenic radial wavefunction with nuclear charge Z
// ============================================================================
double radialWavefunctionZ(int n, int l, double r, int Z)
{
    if (n < 1 || l < 0 || l >= n || Z < 1) return 0.0;
    if (r < 0.0) return 0.0;
    double z = static_cast<double>(Z);
    double rho = 2.0 * z * r / n;
    double norm = 2.0 * z / n;
    norm = norm * norm * norm;
    norm /= (2.0 * n);
    double factRatio = 1.0;
    for (int i = n - l; i <= n + l; ++i) factRatio /= i;
    norm *= factRatio;
    norm = std::sqrt(norm);
    double result = norm;
    if (l > 0) result *= std::pow(rho, l);
    result *= associatedLaguerre(n - l - 1, 2 * l + 1, rho);
    result *= std::exp(-z * r / n);
    return result;
}

std::complex<double> wavefunctionZ(int n, int l, int m,
                                   double x, double y, double zc, int Z)
{
    if (n < 1 || l < 0 || l >= n || std::abs(m) > l || Z < 1) return 0.0;
    double r = std::sqrt(x*x + y*y + zc*zc);
    if (r < 1e-12) {
        if (l == 0) return radialWavefunctionZ(n, 0, 0.0, Z) / std::sqrt(4.0 * M_PI);
        return 0.0;
    }
    double theta = std::acos(zc / r);
    double phi   = std::atan2(y, x);
    return radialWavefunctionZ(n, l, r, Z) * sphericalHarmonic(l, m, theta, phi);
}

double probabilityDensityZ(int n, int l, int m,
                           double x, double y, double zc, int Z)
{
    auto psi = wavefunctionZ(n, l, m, x, y, zc, Z);
    return std::norm(psi);
}

// ============================================================================
//  Pre-compute 3-D volume data
// ============================================================================
void computeVolumeData(int n, int l, int m,
                        int resolution, double halfSize,
                        std::vector<float> &data)
{
    data.resize(resolution * resolution * resolution);

    double dx = 2.0 * halfSize / (resolution - 1);

#ifdef USE_OPENMP
    #pragma omp parallel for collapse(3) schedule(dynamic)
#endif
    for (int iz = 0; iz < resolution; ++iz) {
        for (int iy = 0; iy < resolution; ++iy) {
            for (int ix = 0; ix < resolution; ++ix) {
                double x = -halfSize + ix * dx;
                double y = -halfSize + iy * dx;
                double z = -halfSize + iz * dx;

                double dens = probabilityDensity(n, l, m, x, y, z);
                data[iz * resolution * resolution + iy * resolution + ix] =
                    static_cast<float>(dens);
            }
        }
    }

    // Normalise so max = 1.0 for optimal dynamic range in the texture
    float maxVal = 0.0f;
#ifdef USE_OPENMP
    #pragma omp parallel for reduction(max:maxVal)
#endif
    for (size_t i = 0; i < data.size(); ++i)
        if (data[i] > maxVal) maxVal = data[i];
    if (maxVal > 0.0f) {
        float invMax = 1.0f / maxVal;
#ifdef USE_OPENMP
        #pragma omp parallel for
#endif
        for (int i = 0; i < static_cast<int>(data.size()); ++i)
            data[i] *= invMax;
    }
}

// ============================================================================
//  Z-aware volume computation (normalised so max = 1.0)
// ============================================================================
void computeOrbital(int n, int l, int m, int resolution, double halfSize,
                    int Z, std::vector<float> &data)
{
    const auto res = static_cast<std::vector<float>::size_type>(resolution);
    data.resize(res * res * res);

    double dx = 2.0 * halfSize / (resolution - 1);

#ifdef USE_OPENMP
    #pragma omp parallel for collapse(3) schedule(dynamic)
#endif
    for (int iz = 0; iz < resolution; ++iz) {
        for (int iy = 0; iy < resolution; ++iy) {
            for (int ix = 0; ix < resolution; ++ix) {
                double x = -halfSize + ix * dx;
                double y = -halfSize + iy * dx;
                double z = -halfSize + iz * dx;
                double dens = probabilityDensityZ(n, l, m, x, y, z, Z);
                data[iz * resolution * resolution + iy * resolution + ix] =
                    static_cast<float>(dens);
            }
        }
    }

    float maxVal = 0.0f;
#ifdef USE_OPENMP
    #pragma omp parallel for reduction(max:maxVal)
#endif
    for (size_t i = 0; i < data.size(); ++i)
        if (data[i] > maxVal) maxVal = data[i];
    if (maxVal > 0.0f) {
        float invMax = 1.0f / maxVal;
#ifdef USE_OPENMP
        #pragma omp parallel for
#endif
        for (int i = 0; i < static_cast<int>(data.size()); ++i)
            data[i] *= invMax;
    }
}

// ============================================================================
//  Suggested half-size
//
//  Classical turning point r_max ≈ 2 n² ;  we go to 3 n² to capture the
//  tail (integrates > 99.9 % of the probability).
// ============================================================================
double suggestedHalfSize(int n)
{
    return 3.0 * n * n;
}

// ============================================================================
//  Orbital catalogue
// ============================================================================
const OrbitalInfo ORBITALS[] = {
    // n=1
    {1, 0,  0, "1s",  "Ground state — spherically symmetric"},
    // n=2
    {2, 0,  0, "2s",  "Spherical with one radial node"},
    {2, 1,  0, "2p₀", "Dumbbell along z-axis"},
    {2, 1,  1, "2p₁", "Torus in xy-plane"},
    // n=3
    {3, 0,  0, "3s",  "Spherical with two radial nodes"},
    {3, 1,  0, "3p₀", "Dumbbell along z (n=3)"},
    {3, 1,  1, "3p₁", "Torus in xy-plane (n=3)"},
    {3, 2,  0, "3d₀", "d_{z²} — double dumbbell + torus"},
    {3, 2,  1, "3d₁", "d_{xz} — four-lobed"},
    {3, 2,  2, "3d₂", "d_{x²–y²} — four-lobed in xy-plane"},
    // n=4
    {4, 0,  0, "4s",  "Spherical with three radial nodes"},
    {4, 1,  0, "4p₀", "Dumbbell along z (n=4)"},
    {4, 1,  1, "4p₁", "Torus in xy-plane (n=4)"},
    {4, 2,  0, "4d₀", "d_{z²} (n=4)"},
    {4, 2,  1, "4d₁", "d_{xz} (n=4)"},
    {4, 2,  2, "4d₂", "d_{x²–y²} (n=4)"},
    {4, 3,  0, "4f₀", "f_{z³} — complex lobed structure"},
    {4, 3,  1, "4f₁", "f orbital (m=1)"},
    {4, 3,  2, "4f₂", "f orbital (m=2)"},
    {4, 3,  3, "4f₃", "f orbital (m=3)"},
    // n=5
    {5, 0,  0, "5s",  "Spherical with four radial nodes"},
    {5, 1,  0, "5p₀", "Dumbbell along z (n=5)"},
    {5, 1,  1, "5p₁", "Torus in xy-plane (n=5)"},
    {5, 2,  0, "5d₀", "d_{z²} (n=5)"},
    {5, 2,  1, "5d₁", "d_{xz} (n=5)"},
    {5, 2,  2, "5d₂", "d_{x²–y²} (n=5)"},
    {5, 3,  0, "5f₀", "f_{z³} (n=5)"},
    {5, 3,  1, "5f₁", "f orbital (n=5, m=1)"},
    {5, 3,  2, "5f₂", "f orbital (n=5, m=2)"},
    {5, 3,  3, "5f₃", "f orbital (n=5, m=3)"},
    {5, 4,  0, "5g₀", "g orbital — exotic 8-lobed"},
    {5, 4,  1, "5g₁", "g orbital (m=1)"},
    {5, 4,  2, "5g₂", "g orbital (m=2)"},
    // n=6
    {6, 0,  0, "6s",  "Spherical with five radial nodes"},
    {6, 1,  0, "6p₀", "Dumbbell along z (n=6)"},
    {6, 1,  1, "6p₁", "Torus in xy-plane (n=6)"},
    {6, 2,  0, "6d₀", "d_{z²} (n=6)"},
    {6, 2,  1, "6d₁", "d_{xz} (n=6)"},
    {6, 2,  2, "6d₂", "d_{x²–y²} (n=6)"},
    // n=7
    {7, 0,  0, "7s",  "Spherical with six radial nodes"},
    {7, 1,  0, "7p₀", "Dumbbell along z (n=7)"},
    {7, 1,  1, "7p₁", "Torus in xy-plane (n=7)"},
};
const int NUM_ORBITALS = sizeof(ORBITALS) / sizeof(ORBITALS[0]);
