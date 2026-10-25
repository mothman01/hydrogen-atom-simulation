#include "time_evolution.h"

#include <cmath>
#include <complex>
#include <algorithm>
#include <cstddef>

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

namespace {

struct FieldCache {
    int n1 = 0, l1 = 0, m1 = 0, n2 = 0, l2 = 0, m2 = 0, Z = 0, resolution = 0;
    double c1 = 0.0, halfSize = 0.0;
    std::vector<float> A, Cre, Cim;

    bool matches(const State& st, int z, int res, double hs) const {
        return n1 == st.n1 && l1 == st.l1 && m1 == st.m1 &&
               n2 == st.n2 && l2 == st.l2 && m2 == st.m2 &&
               c1 == st.c1 && Z == z && resolution == res && halfSize == hs &&
               !A.empty();
    }
};

FieldCache g_cache;

void buildCache(const State& st, int Z, int resolution, double halfSize)
{
    const std::size_t R = static_cast<std::size_t>(resolution);
    const std::size_t N = R * R * R;
    g_cache.A.assign(N, 0.0f);
    g_cache.Cre.assign(N, 0.0f);
    g_cache.Cim.assign(N, 0.0f);

    const OrbitalEvaluator orb1(st.n1, st.l1, st.m1, Z);
    const OrbitalEvaluator orb2(st.n2, st.l2, st.m2, Z);
    const double c1 = st.c1;
    const double c2 = std::sqrt(std::max(0.0, 1.0 - c1 * c1));
    const double dx = 2.0 * halfSize / (resolution - 1);
    const int rows = resolution * resolution;

#ifdef USE_OPENMP
    #pragma omp parallel for schedule(static)
#endif
    for (int row = 0; row < rows; ++row) {
        const int iz = row / resolution;
        const int iy = row % resolution;
        const double zc = -halfSize + iz * dx;
        const double y  = -halfSize + iy * dx;
        const std::size_t base = static_cast<std::size_t>(row) * R;
        for (int ix = 0; ix < resolution; ++ix) {
            const double x = -halfSize + ix * dx;
            const std::complex<double> a = c1 * orb1.psi(x, y, zc);
            const std::complex<double> b = c2 * orb2.psi(x, y, zc);
            const std::complex<double> c = a * std::conj(b);
            g_cache.A[base + ix]   = static_cast<float>(std::norm(a) + std::norm(b));
            g_cache.Cre[base + ix] = static_cast<float>(c.real());
            g_cache.Cim[base + ix] = static_cast<float>(c.imag());
        }
    }

    g_cache.n1 = st.n1; g_cache.l1 = st.l1; g_cache.m1 = st.m1;
    g_cache.n2 = st.n2; g_cache.l2 = st.l2; g_cache.m2 = st.m2;
    g_cache.c1 = st.c1; g_cache.Z = Z;
    g_cache.resolution = resolution; g_cache.halfSize = halfSize;
}

} // namespace

void computeFrame(const State& st, int Z,
                  double t, int resolution, double halfSize,
                  std::vector<float>& data) {{
    const std::size_t R = static_cast<std::size_t>(resolution);
    data.resize(R * R * R);
    if (resolution < 2) return;

    if (!g_cache.matches(st, Z, resolution, halfSize))
        buildCache(st, Z, resolution, halfSize);

    double z = static_cast<double>(Z);

    double E1 = -13.6 * z * z / (st.n1 * st.n1);
    double E2 = -13.6 * z * z / (st.n2 * st.n2);

    const double eV_to_Hartree = 1.0 / 27.2114;
    double omega1 = E1 * eV_to_Hartree;
    double omega2 = E2 * eV_to_Hartree;

    const double timeScale = 2.0e15;
    double t_scaled = t * timeScale;

    double phase1 = -omega1 * t_scaled;
    double phase2 = -omega2 * t_scaled + st.phaseOffset;
    const double dPhase = phase1 - phase2;
    const float cosD = static_cast<float>(2.0 * std::cos(dPhase));
    const float sinD = static_cast<float>(2.0 * std::sin(dPhase));

    const float* A   = g_cache.A.data();
    const float* Cre = g_cache.Cre.data();
    const float* Cim = g_cache.Cim.data();
    float* out = data.data();
    const int N = static_cast<int>(data.size());

#ifdef USE_OPENMP
    #pragma omp parallel for schedule(static)
#endif
    for (int i = 0; i < N; ++i) {
        const float d = A[i] + Cre[i] * cosD - Cim[i] * sinD;
        out[i] = d > 0.0f ? d : 0.0f;
    }

    normaliseToUnitMax(data);
}

} // namespace TimeEvolution
