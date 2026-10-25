// Headless sanity tests for the orbital math and the element table.
// Run with `ctest` after building; no OpenGL context is needed.

#include "wavefunction.h"
#include "element_data.h"
#include "time_evolution.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>

static int g_failures = 0;

#define CHECK(cond, ...)                                       \
    do {                                                       \
        if (!(cond)) {                                         \
            ++g_failures;                                      \
            std::printf("FAIL %s:%d  ", __FILE__, __LINE__);   \
            std::printf(__VA_ARGS__);                          \
            std::printf("\n");                                 \
        }                                                      \
    } while (0)

// ∫ R_nl(r)^2 r^2 dr from 0 to rMax (Simpson's rule)
static double radialProbability(const OrbitalEvaluator& orb, double rMax, int steps = 20000)
{
    const double h = rMax / steps;
    double sum = 0.0;
    for (int i = 0; i <= steps; ++i) {
        double r = i * h;
        double R = orb.radial(r);
        double f = R * R * r * r;
        sum += f * ((i == 0 || i == steps) ? 1.0 : (i % 2 ? 4.0 : 2.0));
    }
    return sum * h / 3.0;
}

// ∫ |Y_lm|^2 dΩ, using the evaluator's angular part at fixed r = 1
// (|psi|^2 / R(1)^2) on a θ–φ grid.
static double angularNorm(const OrbitalEvaluator& orb)
{
    const double R1 = orb.radial(1.0);
    if (std::abs(R1) < 1e-300) return -1.0;
    const int NT = 400, NP = 400;
    double sum = 0.0;
    for (int it = 0; it < NT; ++it) {
        double th = (it + 0.5) * M_PI / NT;
        for (int ip = 0; ip < NP; ++ip) {
            double ph = (ip + 0.5) * 2.0 * M_PI / NP;
            double x = std::sin(th) * std::cos(ph), y = std::sin(th) * std::sin(ph),
                   z = std::cos(th);
            sum += orb.density(x, y, z) / (R1 * R1) * std::sin(th);
        }
    }
    return sum * (M_PI / NT) * (2.0 * M_PI / NP);
}

int main()
{
    // 1. Every catalogued orbital is valid and normalised (Z = 1 and fractional Z).
    for (int i = 0; i < NUM_ORBITALS; ++i) {
        const auto& o = ORBITALS[i];
        for (double Z : {1.0, 2.6, 10.75}) {
            OrbitalEvaluator orb(o.n, o.l, o.m, Z);
            CHECK(orb.valid(), "%s Z=%.2f invalid", o.label, Z);
            double pr = radialProbability(orb, 12.0 * o.n * o.n / Z);
            CHECK(std::abs(pr - 1.0) < 1e-3, "%s Z=%.2f radial norm %.6f", o.label, Z, pr);
        }
        OrbitalEvaluator orb(o.n, o.l, o.m, 1.0);
        // Pick a radius where R != 0 for the angular check
        double an = angularNorm(orb);
        CHECK(std::abs(an - 1.0) < 2e-3, "%s angular norm %.6f", o.label, an);

        // psi() and density() must agree
        double x = 0.7, y = -1.3, z = 2.1;
        double d1 = orb.density(x, y, z), d2 = std::norm(orb.psi(x, y, z));
        CHECK(std::abs(d1 - d2) <= 1e-12 * std::max(1.0, d1), "%s psi/density mismatch", o.label);

        // Fast evaluator must match the reference implementation (Z = 1)
        double ref = probabilityDensity(o.n, o.l, o.m, x, y, z);
        CHECK(std::abs(ref - d1) <= 1e-10 * std::max(1e-30, ref) + 1e-300,
              "%s fast %.6e vs reference %.6e", o.label, d1, ref);
    }

    // 2. Every element: valence orbital is in the catalogue, renders a finite,
    //    non-empty, unit-max volume, and distinct Z_eff gives distinct clouds.
    std::vector<float> data;
    for (int Z = 1; Z <= NUM_ELEMENTS; ++Z) {
        const ElementInfo* e = findElementByZ(Z);
        CHECK(e && e->Z == Z, "element %d lookup", Z);
        if (!e) continue;
        CHECK(findElementBySymbol(e->symbol) == e, "%s symbol lookup", e->symbol);
        CHECK(e->Z_eff > 0.0 && e->Z_eff <= Z, "%s Z_eff %.2f out of range", e->symbol, e->Z_eff);

        int idx = -1;
        for (int i = 0; i < NUM_ORBITALS; ++i)
            if (ORBITALS[i].n == e->outermost_n && ORBITALS[i].l == e->outermost_l &&
                ORBITALS[i].m == 0) { idx = i; break; }
        CHECK(idx >= 0, "%s valence orbital n=%d l=%d not in catalogue",
              e->symbol, e->outermost_n, e->outermost_l);
        if (idx < 0) continue;

        double half = std::max(1.0, suggestedHalfSize(e->outermost_n) / e->Z_eff);
        computeOrbital(e->outermost_n, e->outermost_l, 0, 32, half, e->Z_eff, data);
        float mx = 0.0f; bool finite = true;
        for (float v : data) { finite = finite && std::isfinite(v); mx = std::max(mx, v); }
        CHECK(finite, "%s volume has non-finite values", e->symbol);
        CHECK(std::abs(mx - 1.0f) < 1e-6f, "%s volume max %.6f (expected 1)", e->symbol, mx);

        // Most of the probability must lie inside the rendered box.
        OrbitalEvaluator orb(e->outermost_n, e->outermost_l, 0, e->Z_eff);
        double inside = radialProbability(orb, half);
        CHECK(inside > 0.9, "%s only %.1f%% of probability inside box", e->symbol, inside * 100);
    }

    // He (Z_eff 1.7) must no longer render identically to H (Z_eff 1.0).
    {
        OrbitalEvaluator h(1, 0, 0, ELEMENTS[1].Z_eff), he(1, 0, 0, ELEMENTS[2].Z_eff);
        CHECK(std::abs(h.radial(1.0) - he.radial(1.0)) > 1e-3, "He identical to H");
    }

    // 3. Time evolution actually changes over time (non-degenerate default).
    {
        TimeEvolution::reset();
        std::vector<float> a, b;
        double hs = suggestedHalfSize(2);
        TimeEvolution::computeFrame(TimeEvolution::currentState, 1, 0.0, 32, hs, a);
        TimeEvolution::computeFrame(TimeEvolution::currentState, 1,
                                    TimeEvolution::beatPeriod(1) * 0.5, 32, hs, b);
        double diff = 0.0;
        for (std::size_t i = 0; i < a.size(); ++i) diff += std::abs(a[i] - b[i]);
        CHECK(diff > 1.0, "time evolution is static (diff %.3g)", diff);
        CHECK(TimeEvolution::beatPeriod(1) > 0.5 && TimeEvolution::beatPeriod(1) < 20.0,
              "beat period %.3g s not watchable", TimeEvolution::beatPeriod(1));
    }

    // 4. Performance: a 128^3 volume (what the app computes per atom/orbital).
    {
        auto t0 = std::chrono::steady_clock::now();
        computeOrbital(4, 3, 0, 128, suggestedHalfSize(4), 1.0, data);
        auto ms = std::chrono::duration<double, std::milli>(
                      std::chrono::steady_clock::now() - t0).count();
        std::printf("128^3 4f volume computed in %.1f ms\n", ms);
    }

    if (g_failures == 0) std::printf("All physics tests passed.\n");
    else                 std::printf("%d failure(s).\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
