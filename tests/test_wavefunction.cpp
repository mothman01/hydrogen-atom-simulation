#include <doctest/doctest.h>
#include "wavefunction.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <vector>

// Use our own pi constant to avoid platform-specific macro issues.
static constexpr double kPi = 3.14159265358979323846;

// ===========================================================================
//  associatedLaguerre
// ===========================================================================

TEST_CASE("associatedLaguerre: boundary guards return 0") {
    CHECK(associatedLaguerre(-1, 0, 1.0) == doctest::Approx(0.0));
    CHECK(associatedLaguerre(0, -1, 1.0) == doctest::Approx(0.0));
    CHECK(associatedLaguerre(-2, -3, 5.0) == doctest::Approx(0.0));
}

TEST_CASE("associatedLaguerre: L_0^k = 1 for any k and x") {
    CHECK(associatedLaguerre(0, 0, 0.0) == doctest::Approx(1.0));
    CHECK(associatedLaguerre(0, 3, 7.5) == doctest::Approx(1.0));
    CHECK(associatedLaguerre(0, 5, 0.0) == doctest::Approx(1.0));
}

TEST_CASE("associatedLaguerre: L_1^k(x) = 1 + k - x") {
    // L_1^0(x) = 1 - x
    CHECK(associatedLaguerre(1, 0, 0.0) == doctest::Approx(1.0));
    CHECK(associatedLaguerre(1, 0, 2.0) == doctest::Approx(-1.0));
    // L_1^2(x) = 1 + 2 - x = 3 - x
    CHECK(associatedLaguerre(1, 2, 3.0) == doctest::Approx(0.0));
    CHECK(associatedLaguerre(1, 2, 1.0) == doctest::Approx(2.0));
    // L_1^3(1) = 1 + 3 - 1 = 3
    CHECK(associatedLaguerre(1, 3, 1.0) == doctest::Approx(3.0));
}

TEST_CASE("associatedLaguerre: L_2^0(x) = 1 - 2x + x^2/2") {
    // L_2^0(0) = 1
    CHECK(associatedLaguerre(2, 0, 0.0) == doctest::Approx(1.0));
    // L_2^0(2) = 1 - 4 + 2 = -1
    CHECK(associatedLaguerre(2, 0, 2.0) == doctest::Approx(-1.0));
    // L_2^0(4) = 1 - 8 + 8 = 1
    CHECK(associatedLaguerre(2, 0, 4.0) == doctest::Approx(1.0));
}

TEST_CASE("associatedLaguerre: L_2^1 from recurrence") {
    // L_2^1(x) = (x^2 - 6x + 6) / 2  (derived from recurrence)
    // L_2^1(0) = 3
    CHECK(associatedLaguerre(2, 1, 0.0) == doctest::Approx(3.0));
    // L_2^1(2) = (4 - 12 + 6) / 2 = -1
    CHECK(associatedLaguerre(2, 1, 2.0) == doctest::Approx(-1.0));
    // L_2^1(6) = (36 - 36 + 6) / 2 = 3
    CHECK(associatedLaguerre(2, 1, 6.0) == doctest::Approx(3.0));
}

// ===========================================================================
//  associatedLegendre
// ===========================================================================

TEST_CASE("associatedLegendre: invalid inputs return 0") {
    CHECK(associatedLegendre(-1, 0, 0.5) == doctest::Approx(0.0));
    CHECK(associatedLegendre(0, -1, 0.5) == doctest::Approx(0.0));
    CHECK(associatedLegendre(1,  2, 0.5) == doctest::Approx(0.0));  // m > l
    CHECK(associatedLegendre(2,  3, 0.5) == doctest::Approx(0.0));  // m > l
}

TEST_CASE("associatedLegendre: P_0^0 = 1") {
    CHECK(associatedLegendre(0, 0,  0.7) == doctest::Approx(1.0));
    CHECK(associatedLegendre(0, 0, -0.3) == doctest::Approx(1.0));
    CHECK(associatedLegendre(0, 0,  0.0) == doctest::Approx(1.0));
}

TEST_CASE("associatedLegendre: P_1^0(x) = x") {
    CHECK(associatedLegendre(1, 0,  0.5) == doctest::Approx( 0.5));
    CHECK(associatedLegendre(1, 0, -1.0) == doctest::Approx(-1.0));
    CHECK(associatedLegendre(1, 0,  0.0) == doctest::Approx( 0.0));
}

TEST_CASE("associatedLegendre: P_1^1(x) = -sqrt(1-x^2)  [Condon-Shortley]") {
    // At x=0: P_1^1(0) = -sqrt(1) = -1
    CHECK(associatedLegendre(1, 1, 0.0) == doctest::Approx(-1.0));
    // At x=0.5: P_1^1(0.5) = -sqrt(0.75)
    CHECK(associatedLegendre(1, 1, 0.5) == doctest::Approx(-std::sqrt(0.75)));
    // At x=1: P_1^1(1) = 0
    CHECK(associatedLegendre(1, 1, 1.0) == doctest::Approx(0.0).scale(1.0));
}

TEST_CASE("associatedLegendre: P_2^0(x) = (3x^2 - 1)/2") {
    // P_2^0(0) = -0.5
    CHECK(associatedLegendre(2, 0, 0.0) == doctest::Approx(-0.5));
    // P_2^0(1) = 1
    CHECK(associatedLegendre(2, 0, 1.0) == doctest::Approx(1.0));
    // P_2^0(0.5) = (3*0.25 - 1)/2 = -0.125
    CHECK(associatedLegendre(2, 0, 0.5) == doctest::Approx(-0.125));
}

// ===========================================================================
//  sphericalHarmonic
// ===========================================================================

TEST_CASE("sphericalHarmonic: invalid inputs return 0") {
    CHECK(std::abs(sphericalHarmonic(-1, 0, 1.0, 0.5)) < 1e-14);
    CHECK(std::abs(sphericalHarmonic(1,  2, 1.0, 0.5)) < 1e-14);
    CHECK(std::abs(sphericalHarmonic(1, -2, 1.0, 0.5)) < 1e-14);
}

TEST_CASE("sphericalHarmonic: Y_0^0 = 1/sqrt(4pi), purely real") {
    double expected = 1.0 / std::sqrt(4.0 * kPi);
    auto Y = sphericalHarmonic(0, 0, 1.0, 0.5);
    CHECK(Y.real() == doctest::Approx(expected).epsilon(1e-10));
    CHECK(std::abs(Y.imag()) < 1e-14);
    // Y_0^0 is independent of theta and phi
    auto Y2 = sphericalHarmonic(0, 0, 0.3, 2.7);
    CHECK(Y2.real() == doctest::Approx(expected).epsilon(1e-10));
}

TEST_CASE("sphericalHarmonic: normalization integral ~= 1") {
    // Numerical check: sum_theta,phi |Y_l^m|^2 sin(theta) dtheta dphi = 1
    auto checkNorm = [](int l, int m) {
        int N = 150;
        double dTheta = kPi / N;
        double dPhi   = 2.0 * kPi / N;
        double sum = 0.0;
        for (int i = 0; i < N; ++i) {
            double theta = (i + 0.5) * dTheta;
            for (int j = 0; j < N; ++j) {
                double phi = (j + 0.5) * dPhi;
                auto Y = sphericalHarmonic(l, m, theta, phi);
                sum += std::norm(Y) * std::sin(theta) * dTheta * dPhi;
            }
        }
        return sum;
    };
    CHECK(checkNorm(0,  0) == doctest::Approx(1.0).epsilon(0.01));
    CHECK(checkNorm(1,  0) == doctest::Approx(1.0).epsilon(0.01));
    CHECK(checkNorm(1,  1) == doctest::Approx(1.0).epsilon(0.01));
    CHECK(checkNorm(1, -1) == doctest::Approx(1.0).epsilon(0.01));
    CHECK(checkNorm(2,  0) == doctest::Approx(1.0).epsilon(0.01));
    CHECK(checkNorm(2,  2) == doctest::Approx(1.0).epsilon(0.01));
    CHECK(checkNorm(3,  1) == doctest::Approx(1.0).epsilon(0.01));
}

TEST_CASE("sphericalHarmonic: conjugate symmetry Y_l^{-m} = (-1)^m conj(Y_l^m)") {
    auto checkConj = [](int l, int m, double theta, double phi) {
        auto Y_pos  = sphericalHarmonic(l,  m, theta, phi);
        auto Y_neg  = sphericalHarmonic(l, -m, theta, phi);
        double sign = (m % 2 == 0) ? 1.0 : -1.0;
        auto expected = sign * std::conj(Y_pos);
        CHECK(Y_neg.real() == doctest::Approx(expected.real()).epsilon(1e-10));
        CHECK(Y_neg.imag() == doctest::Approx(expected.imag()).epsilon(1e-10));
    };
    checkConj(1, 1, 1.0, 0.7);
    checkConj(2, 1, 0.8, 1.2);
    checkConj(2, 2, 0.5, 2.0);
    checkConj(3, 2, 1.1, 0.3);
}

// ===========================================================================
//  radialWavefunction
// ===========================================================================

TEST_CASE("radialWavefunction: invalid inputs return 0") {
    CHECK(radialWavefunction(0, 0,  1.0) == doctest::Approx(0.0));  // n < 1
    CHECK(radialWavefunction(1, 1,  1.0) == doctest::Approx(0.0));  // l >= n
    CHECK(radialWavefunction(2, -1, 1.0) == doctest::Approx(0.0));  // l < 0
    CHECK(radialWavefunction(1, 0, -1.0) == doctest::Approx(0.0));  // r < 0
}

TEST_CASE("radialWavefunction: R_10(r) = 2 exp(-r)") {
    // Exact formula for n=1, l=0 in atomic units
    const double test_r[] = {0.0, 0.5, 1.0, 2.0, 5.0};
    for (double r : test_r) {
        double expected = 2.0 * std::exp(-r);
        CHECK(radialWavefunction(1, 0, r) == doctest::Approx(expected).epsilon(1e-10));
    }
}

TEST_CASE("radialWavefunction: R_21(r) = r/sqrt(24) * exp(-r/2)") {
    double inv_sqrt24 = 1.0 / std::sqrt(24.0);
    const double test_r[] = {0.5, 1.0, 2.0, 4.0};
    for (double r : test_r) {
        double expected = inv_sqrt24 * r * std::exp(-r / 2.0);
        CHECK(radialWavefunction(2, 1, r) == doctest::Approx(expected).epsilon(1e-10));
    }
}

TEST_CASE("radialWavefunction: normalization integral = 1") {
    // ∫_0^∞ R_{nl}^2 r^2 dr = 1
    auto checkNorm = [](int n, int l) {
        double rmax = static_cast<double>(n * n) * 25.0;
        int N = 5000;
        double dr = rmax / N;
        double sum = 0.0;
        for (int i = 0; i < N; ++i) {
            double r = (i + 0.5) * dr;
            double R = radialWavefunction(n, l, r);
            sum += R * R * r * r * dr;
        }
        return sum;
    };
    CHECK(checkNorm(1, 0) == doctest::Approx(1.0).epsilon(0.005));
    CHECK(checkNorm(2, 0) == doctest::Approx(1.0).epsilon(0.005));
    CHECK(checkNorm(2, 1) == doctest::Approx(1.0).epsilon(0.005));
    CHECK(checkNorm(3, 0) == doctest::Approx(1.0).epsilon(0.005));
    CHECK(checkNorm(3, 2) == doctest::Approx(1.0).epsilon(0.005));
}

// ===========================================================================
//  wavefunction / probabilityDensity
// ===========================================================================

TEST_CASE("wavefunction: invalid quantum numbers return 0") {
    CHECK(std::abs(wavefunction(0, 0, 0, 1.0, 0.0, 0.0)) < 1e-14);   // n < 1
    CHECK(std::abs(wavefunction(1, 1, 0, 1.0, 0.0, 0.0)) < 1e-14);   // l >= n
    CHECK(std::abs(wavefunction(2, 1, 2, 1.0, 0.0, 0.0)) < 1e-14);   // |m| > l
    CHECK(std::abs(wavefunction(2, 1, -2, 1.0, 0.0, 0.0)) < 1e-14);  // m < -l
}

TEST_CASE("probabilityDensity: s-orbitals non-zero at origin, others zero") {
    CHECK(probabilityDensity(1, 0, 0, 0.0, 0.0, 0.0) > 0.0);
    CHECK(probabilityDensity(2, 0, 0, 0.0, 0.0, 0.0) > 0.0);
    CHECK(probabilityDensity(2, 1,  0, 0.0, 0.0, 0.0) == doctest::Approx(0.0));
    CHECK(probabilityDensity(2, 1,  1, 0.0, 0.0, 0.0) == doctest::Approx(0.0));
    CHECK(probabilityDensity(3, 2,  0, 0.0, 0.0, 0.0) == doctest::Approx(0.0));
}

TEST_CASE("probabilityDensity: 1s at origin = 1/pi") {
    // |psi_100(0)|^2 = |R_10(0)|^2 / (4pi) = 4/(4pi) = 1/pi
    double expected = 1.0 / kPi;
    CHECK(probabilityDensity(1, 0, 0, 0.0, 0.0, 0.0) == doctest::Approx(expected).epsilon(1e-10));
}

TEST_CASE("probabilityDensity: s-orbitals are spherically symmetric") {
    double r = 2.5;
    double inv_sqrt3 = 1.0 / std::sqrt(3.0);
    double d0 = probabilityDensity(1, 0, 0, r, 0.0, 0.0);
    double d1 = probabilityDensity(1, 0, 0, 0.0, r, 0.0);
    double d2 = probabilityDensity(1, 0, 0, 0.0, 0.0, r);
    double d3 = probabilityDensity(1, 0, 0, r * inv_sqrt3, r * inv_sqrt3, r * inv_sqrt3);
    CHECK(d1 == doctest::Approx(d0).epsilon(1e-10));
    CHECK(d2 == doctest::Approx(d0).epsilon(1e-10));
    CHECK(d3 == doctest::Approx(d0).epsilon(1e-10));

    // Also check 2s
    double e0 = probabilityDensity(2, 0, 0, r, 0.0, 0.0);
    double e1 = probabilityDensity(2, 0, 0, 0.0, 0.0, r);
    CHECK(e1 == doctest::Approx(e0).epsilon(1e-10));
}

TEST_CASE("probabilityDensity: total probability ~= 1 (3D Riemann sum)") {
    // ∫|psi|^2 dV = 1 verified via a Cartesian grid
    auto checkNorm = [](int n, int l, int m) {
        int res    = 50;
        double hs  = suggestedHalfSize(n) * 1.5;
        double dx  = 2.0 * hs / (res - 1);
        double dV  = dx * dx * dx;
        double sum = 0.0;
        for (int iz = 0; iz < res; ++iz) {
            double z = -hs + iz * dx;
            for (int iy = 0; iy < res; ++iy) {
                double y = -hs + iy * dx;
                for (int ix = 0; ix < res; ++ix) {
                    double x = -hs + ix * dx;
                    sum += probabilityDensity(n, l, m, x, y, z) * dV;
                }
            }
        }
        return sum;
    };
    CHECK(checkNorm(1, 0,  0) == doctest::Approx(1.0).epsilon(0.05));
    CHECK(checkNorm(2, 0,  0) == doctest::Approx(1.0).epsilon(0.05));
    CHECK(checkNorm(2, 1,  0) == doctest::Approx(1.0).epsilon(0.05));
}

// ===========================================================================
//  radialWavefunctionZ / wavefunctionZ / probabilityDensityZ
// ===========================================================================

TEST_CASE("radialWavefunctionZ: Z < 1 returns 0") {
    CHECK(radialWavefunctionZ(1, 0, 1.0, 0)  == doctest::Approx(0.0));
    CHECK(radialWavefunctionZ(1, 0, 1.0, -1) == doctest::Approx(0.0));
}

TEST_CASE("radialWavefunctionZ: Z=1 matches radialWavefunction") {
    const double test_r[] = {0.0, 0.5, 1.0, 2.0, 4.0};
    for (double r : test_r) {
        CHECK(radialWavefunctionZ(1, 0, r, 1) ==
              doctest::Approx(radialWavefunction(1, 0, r)).epsilon(1e-10));
    }
    for (double r : test_r) {
        CHECK(radialWavefunctionZ(2, 1, r, 1) ==
              doctest::Approx(radialWavefunction(2, 1, r)).epsilon(1e-10));
    }
}

TEST_CASE("radialWavefunctionZ: higher Z gives higher peak density at origin") {
    double R_Z1 = radialWavefunctionZ(1, 0, 0.0, 1);
    double R_Z2 = radialWavefunctionZ(1, 0, 0.0, 2);
    double R_Z4 = radialWavefunctionZ(1, 0, 0.0, 4);
    CHECK(R_Z2 > R_Z1);
    CHECK(R_Z4 > R_Z2);
}

TEST_CASE("wavefunctionZ: Z=1 matches wavefunction") {
    const double pts[][3] = {{1.0, 0.0, 0.0}, {0.0, 2.0, 0.0}, {1.0, 1.0, 1.0}, {3.0, 4.0, 5.0}};
    for (const auto& p : pts) {
        double x = p[0], y = p[1], z = p[2];
        auto psi_H  = wavefunction(1, 0, 0, x, y, z);
        auto psi_Z1 = wavefunctionZ(1, 0, 0, x, y, z, 1);
        CHECK(psi_Z1.real() == doctest::Approx(psi_H.real()).epsilon(1e-10));
        CHECK(psi_Z1.imag() == doctest::Approx(psi_H.imag()).epsilon(1e-10));
    }
}

TEST_CASE("probabilityDensityZ: Z>1 gives higher peak density than hydrogen") {
    double d_Z1 = probabilityDensityZ(1, 0, 0, 0.0, 0.0, 0.0, 1);
    double d_Z2 = probabilityDensityZ(1, 0, 0, 0.0, 0.0, 0.0, 2);
    CHECK(d_Z2 > d_Z1);
}

// ===========================================================================
//  computeVolumeData
// ===========================================================================

TEST_CASE("computeVolumeData: output size is resolution^3") {
    std::vector<float> data;
    computeVolumeData(1, 0, 0, 5, 3.0, data);
    CHECK(data.size() == static_cast<size_t>(5 * 5 * 5));
    computeVolumeData(2, 1, 0, 8, 12.0, data);
    CHECK(data.size() == static_cast<size_t>(8 * 8 * 8));
}

TEST_CASE("computeVolumeData: all values in [0, 1] and maximum equals 1") {
    std::vector<float> data;
    computeVolumeData(1, 0, 0, 10, 5.0, data);

    float maxVal = 0.0f;
    for (float v : data) {
        CHECK(v >= 0.0f);
        CHECK(v <= 1.0f);
        if (v > maxVal) maxVal = v;
    }
    CHECK(maxVal == doctest::Approx(1.0f));
}

TEST_CASE("computeVolumeData: invalid quantum numbers produce all-zero output") {
    std::vector<float> data;

    // l >= n is invalid
    computeVolumeData(1, 1, 0, 5, 3.0, data);
    for (float v : data) CHECK(v == 0.0f);

    // |m| > l is invalid
    computeVolumeData(2, 1, 2, 5, 5.0, data);
    for (float v : data) CHECK(v == 0.0f);
}

// ===========================================================================
//  suggestedHalfSize
// ===========================================================================

TEST_CASE("suggestedHalfSize: returns 3n^2") {
    CHECK(suggestedHalfSize(1) == doctest::Approx(3.0));
    CHECK(suggestedHalfSize(2) == doctest::Approx(12.0));
    CHECK(suggestedHalfSize(3) == doctest::Approx(27.0));
    CHECK(suggestedHalfSize(4) == doctest::Approx(48.0));
}

TEST_CASE("suggestedHalfSize: monotonically increasing with n") {
    for (int n = 1; n <= 6; ++n)
        CHECK(suggestedHalfSize(n + 1) > suggestedHalfSize(n));
}
