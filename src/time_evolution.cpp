#include "time_evolution.h"

#include <cmath>
#include <complex>
#include <algorithm>

namespace TimeEvolution {

bool enabled = false;
State currentState = {2, 0, 0,   2, 1, 0,   0.707, 0.0};

void reset() {
    currentState = {2, 0, 0,   2, 1, 0,   0.707, 0.0};
}

double beatPeriod(int Z) {
    double z = static_cast<double>(Z);
    double E1 = -13.6 * z * z / (currentState.n1 * currentState.n1);
    double E2 = -13.6 * z * z / (currentState.n2 * currentState.n2);
    double dE = std::abs(E2 - E1);
    if (dE < 1e-12) return 1.0;
    // E in eV, convert to angular frequency:  ω = dE / ħ
    // ħ = 6.582119569e-16 eV·s
    // Period T = 2π / ω = 2π ħ / dE
    // But for visual purposes, we scale it up dramatically
    const double hbar_eVs = 6.582119569e-16;
    double T_real = 2.0 * M_PI * hbar_eVs / dE;  // real period (~femtoseconds)
    // Scale to ~2 seconds for visibility
    const double scale = 2.0e15;
    return T_real * scale;
}

void computeFrame(const State& st, int Z,
                  double t, int resolution, double halfSize,
                  std::vector<float>& data) {
    data.resize(resolution * resolution * resolution);

    double z = static_cast<double>(Z);

    // Energy eigenvalues
    double E1 = -13.6 * z * z / (st.n1 * st.n1);
    double E2 = -13.6 * z * z / (st.n2 * st.n2);

    // Convert eV to atomic units for the phase
    // 1 eV = 1/27.2114 Hartree
    const double eV_to_Hartree = 1.0 / 27.2114;
    double omega1 = E1 * eV_to_Hartree;  // in Hartree = atomic frequency units
    double omega2 = E2 * eV_to_Hartree;

    // Real frequency: 1 Hartree / ħ = 4.1341e16 rad/s
    // For visual appeal, scale time so period ~2 seconds
    const double timeScale = 2.0e15;
    double t_scaled = t * timeScale;

    double c2 = std::sqrt(std::max(0.0, 1.0 - st.c1 * st.c1));
    double phase1 = -omega1 * t_scaled;
    double phase2 = -omega2 * t_scaled + st.phaseOffset;

    double dx = 2.0 * halfSize / (resolution - 1);

#ifdef USE_OPENMP
    #pragma omp parallel for collapse(3) schedule(dynamic)
#endif
    for (int iz = 0; iz < resolution; ++iz) {
        for (int iy = 0; iy < resolution; ++iy) {
            for (int ix = 0; ix < resolution; ++ix) {
                double x = -halfSize + ix * dx;
                double y = -halfSize + iy * dx;
                double zc = -halfSize + iz * dx;

                double r = std::sqrt(x*x + y*y + zc*zc);

                // Radial wavefunctions (real-valued)
                double R1 = radialWavefunctionZ(st.n1, st.l1, r, Z);
                double R2 = radialWavefunctionZ(st.n2, st.l2, r, Z);

                if (r < 1e-12) {
                    // At origin: only s-states contribute
                    double dens = 0.0;
                    if (st.l1 == 0) dens += st.c1 * R1 / std::sqrt(4.0 * M_PI);
                    if (st.l2 == 0) dens += c2 * R2 / std::sqrt(4.0 * M_PI);
                    data[iz * resolution * resolution + iy * resolution + ix] =
                        static_cast<float>(dens * dens);
                    continue;
                }

                double theta = std::acos(zc / r);
                double phi   = std::atan2(y, x);

                // Spherical harmonics (complex)
                auto Y1 = sphericalHarmonic(st.l1, st.m1, theta, phi);
                auto Y2 = sphericalHarmonic(st.l2, st.m2, theta, phi);

                // Superposition
                std::complex<double> psi =
                    st.c1 * R1 * Y1 * std::complex<double>(std::cos(phase1), std::sin(phase1)) +
                    c2    * R2 * Y2 * std::complex<double>(std::cos(phase2), std::sin(phase2));

                double dens = std::norm(psi);
                data[iz * resolution * resolution + iy * resolution + ix] =
                    static_cast<float>(dens);
            }
        }
    }

    // Normalise
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

} // namespace TimeEvolution
