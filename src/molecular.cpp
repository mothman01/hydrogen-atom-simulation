#include "molecular.h"
#include "wavefunction.h"
#include "element_data.h"
#include <complex>
#include <cmath>
#include <algorithm>
#include <iostream>

using namespace std::complex_literals;

namespace MolecularMode {

bool enabled = false;
std::vector<MolAtom> atoms;
VolumeRenderer* sharedRenderer = nullptr;

void init() {
    if (!sharedRenderer) {
        sharedRenderer = new VolumeRenderer();
        sharedRenderer->init(128);
    }
    std::cout << "Molecular mode initialised — build molecules atom by atom\n";
}

void addAtom(const ElementInfo* elem, glm::vec3 pos, int n, int l, int m) {
    if (!elem) return;
    MolAtom a;
    a.element = elem;
    a.position = pos;
    a.Z_eff = static_cast<int>(elem->Z_eff);
    if (a.Z_eff < 1) a.Z_eff = elem->Z;  // fallback to nuclear charge
    a.n = n;
    a.l = l;
    a.m = m;
    atoms.push_back(a);

    static const char* labels = "spdfgh";
    std::cout << "MolAtom: " << elem->symbol << " at ("
              << pos.x << ", " << pos.y << ", " << pos.z << ")"
              << "  orbital " << n << labels[l]
              << "  Z_eff=" << a.Z_eff << "\n";
}

void removeLast() {
    if (!atoms.empty()) atoms.pop_back();
}

void clear() {
    atoms.clear();
}

void computeCombined(int resolution, double halfSize, std::vector<float>& output) {
    output.resize(resolution * resolution * resolution);
    if (atoms.empty()) {
        std::fill(output.begin(), output.end(), 0.0f);
        return;
    }

    double dx = 2.0 * halfSize / (resolution - 1);

#ifdef USE_OPENMP
    #pragma omp parallel for collapse(3) schedule(dynamic)
#endif
    for (int iz = 0; iz < resolution; ++iz) {
        for (int iy = 0; iy < resolution; ++iy) {
            for (int ix = 0; ix < resolution; ++ix) {
                // World-space voxel centre
                double wx = -halfSize + ix * dx;
                double wy = -halfSize + iy * dx;
                double wz = -halfSize + iz * dx;

                // LCAO: sum wavefunctions from all atoms
                std::complex<double> psi_total = 0.0;
                for (size_t i = 0; i < atoms.size(); ++i) {
                    const auto& a = atoms[i];

                    // Shift to atom-local coordinates
                    double ax = wx - static_cast<double>(a.position.x);
                    double ay = wy - static_cast<double>(a.position.y);
                    double az = wz - static_cast<double>(a.position.z);

                    auto psi = wavefunctionZ(a.n, a.l, a.m, ax, ay, az, a.Z_eff);

                    // LCAO coefficient: alternating signs based on atom index
                    // to show both bonding (+) and antibonding (-) contributions
                    double coeff = (i % 2 == 0) ? 1.0 : -1.0;
                    psi_total += coeff * psi;
                }

                double dens = std::norm(psi_total);
                output[iz * resolution * resolution + iy * resolution + ix] =
                    static_cast<float>(dens);
            }
        }
    }

    // Normalise so max = 1.0 for optimal rendering
    float maxVal = 0.0f;
#ifdef USE_OPENMP
    #pragma omp parallel for reduction(max:maxVal)
#endif
    for (size_t i = 0; i < output.size(); ++i)
        if (output[i] > maxVal) maxVal = output[i];
    if (maxVal > 0.0f) {
        float invMax = 1.0f / maxVal;
#ifdef USE_OPENMP
        #pragma omp parallel for
#endif
        for (int i = 0; i < static_cast<int>(output.size()); ++i)
            output[i] *= invMax;
    }
}

void updateAndRender(glm::mat4 view, glm::mat4 proj, glm::vec3 eye,
                     int resolution, double halfSize) {
    if (!sharedRenderer || atoms.empty()) return;

    // Skip frames — only recompute every 3 frames to reduce lag
    static int frameSkip = 0;
    frameSkip++;
    if (frameSkip % 3 != 0) {
        // Keep rendering with existing data
        sharedRenderer->render(view, proj, eye);
        return;
    }

    std::vector<float> data;
    computeCombined(resolution, halfSize, data);
    sharedRenderer->uploadRawData(data, halfSize);
    sharedRenderer->render(view, proj, eye);
}

void shutdown() {
    delete sharedRenderer;
    sharedRenderer = nullptr;
    enabled = false;
}

} // namespace MolecularMode
