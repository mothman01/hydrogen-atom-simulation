#include "energy_diagram.h"
#include "text_renderer.h"
#include "wavefunction.h"

#include <cstdio>
#include <cmath>
#include <algorithm>

namespace EnergyDiagram {

bool visible = false;
float panelX = 10.0f;
float panelY = 10.0f;
float panelW = 220.0f;
float panelH = 380.0f;

// ── Colour per angular momentum ─────────────────────────────────────────
static void lColor(int l, float &r, float &g, float &b) {
    switch (l) {
        case 0: r=1.0f; g=0.75f; b=0.25f; break;  // s — gold
        case 1: r=0.25f; g=0.75f; b=1.0f; break;  // p — cyan
        case 2: r=0.25f; g=1.0f; b=0.35f; break;  // d — green
        case 3: r=0.90f; g=0.35f; b=0.90f; break;  // f — magenta
        case 4: r=1.0f; g=0.40f; b=0.30f; break;  // g — red-orange
        default: r=0.6f; g=0.6f; b=0.6f; break;
    }
}

static const char* lLabel(int l) {
    switch (l) {
        case 0: return "s";
        case 1: return "p";
        case 2: return "d";
        case 3: return "f";
        case 4: return "g";
        case 5: return "h";
        default: return "?";
    }
}

void render(int screenW, int screenH, double Z_eff, int nMax) {
    if (!visible) return;
    if (Z_eff <= 0.0) Z_eff = 1.0;

    // Clamp nMax
    if (nMax < 1) nMax = 1;
    if (nMax > 7) nMax = 7;

    const double E0 = 13.6;  // eV
    double Z2 = Z_eff * Z_eff;

    // Compute energy range
    double E_min = -E0 * Z2;           // n=1
    double E_max = -E0 * Z2 / (nMax * nMax); // highest n

    // Panel dimensions — adjusted dynamically
    panelH = 60.0f + 50.0f * nMax;
    panelX = screenW - panelW - 10.0f;
    panelY = 10.0f;

    // Map energy to pixel Y  (E=0 at top, more negative = lower)
    double E_range = std::abs(E_min - E_max);
    if (E_range < 0.01) E_range = 1.0;
    float margin = 30.0f;
    float graphH = panelH - 2.0f * margin;

    auto energyToY = [&](double E) -> float {
        double frac = (E - E_max) / (E_min - E_max);  // 0=top(high E), 1=bottom(low E)
        return panelY + margin + (float)(frac * graphH);
    };

    // ── Title ─────────────────────────────────────────────────────────
    char buf[128];
    std::snprintf(buf, sizeof(buf), "ENERGY LEVELS  Z_eff=%.2f", Z_eff);
    drawText(buf, panelX + 8, panelY + 4, 1.0f, 1.0f, 0.3f, screenW, screenH);

    // ── Axis line ────────────────────────────────────────────────────
    float axisX = panelX + 50.0f;
    float topY  = panelY + margin;
    float botY  = panelY + margin + graphH;

    // ── Level lines ──────────────────────────────────────────────────
    for (int n = 1; n <= nMax; ++n) {
        double En = -E0 * Z2 / (n * n);
        float y = energyToY(En);

        // Level label
        std::snprintf(buf, sizeof(buf), "n=%d", n);
        drawText(buf, panelX + 4, y - 8, 0.5f, 0.5f, 0.5f, screenW, screenH);

        // Energy value
        std::snprintf(buf, sizeof(buf), "%.1f eV", En);
        drawText(buf, panelX + 36, y - 8, 0.5f, 0.5f, 0.5f, screenW, screenH);

        // Draw sub-levels for each l (0 .. n-1), up to l=4
        for (int l = 0; l < n && l <= 4; ++l) {
            float lx = axisX + 10.0f + l * 22.0f;
            float r, g, b;
            lColor(l, r, g, b);

            // Dim non-ground states slightly
            if (l > 0) { r *= 0.8f; g *= 0.8f; b *= 0.8f; }
            drawText(lLabel(l), lx, y - 8, r, g, b, screenW, screenH);
        }

        // Horizontal dashed line (approximated by dots)
        // We skip actual line drawing here — the text alone is informative enough
    }

    // ── Key ────────────────────────────────────────────────────────────
    float ky = botY + 8;
    for (int l = 0; l <= 4; ++l) {
        float r, g, b;
        lColor(l, r, g, b);
        std::snprintf(buf, sizeof(buf), "%s ", lLabel(l));
        float kx = panelX + 4 + l * 45.0f;
        drawText(buf, kx, ky, r, g, b, screenW, screenH);
        }
    }

    int hitTest(float mouseX, float mouseY, int screenW, int screenH,
                double Z_eff, int nMax) {
        if (!visible) return -1;
        if (Z_eff <= 0.0) Z_eff = 1.0;
        if (nMax < 1) nMax = 1;
        if (nMax > 7) nMax = 7;

        // Must be within the panel bounds
        if (mouseX < panelX || mouseX > panelX + panelW) return -1;
        if (mouseY < panelY || mouseY > panelY + panelH) return -1;

        const double E0 = 13.6;
        double Z2 = Z_eff * Z_eff;
        double E_min = -E0 * Z2;
        double E_max = -E0 * Z2 / (nMax * nMax);
        float margin = 30.0f;
        float graphH = panelH - 2.0f * margin;

        // Find which n level the mouse Y corresponds to
        float mouseGraphY = mouseY - panelY - margin;
        if (mouseGraphY < 0 || mouseGraphY > graphH) return -1;

        // Map mouse Y to energy, find closest n
        double frac = (double)mouseGraphY / graphH;
        double E_click = E_max + frac * (E_min - E_max);

        int bestN = 1;
        double bestDist = 1e9;
        for (int n = 1; n <= nMax; ++n) {
            double En = -E0 * Z2 / (n * n);
            double d = std::abs(E_click - En);
            if (d < bestDist) { bestDist = d; bestN = n; }
        }

        // Find which l sub-level was clicked (based on X position)
        float axisX = panelX + 50.0f;
        float relX = mouseX - axisX - 10.0f;
        int l = (int)(relX / 22.0f);
        if (l < 0) l = 0;
        if (l >= bestN) l = bestN - 1;
        if (l > 4) l = 4;

        // Find orbital index matching n, l, m=0
        for (int i = 0; i < NUM_ORBITALS; ++i) {
            if (ORBITALS[i].n == bestN && ORBITALS[i].l == l && ORBITALS[i].m == 0)
                return i;
        }
        return -1;
    }

    } // namespace EnergyDiagram
