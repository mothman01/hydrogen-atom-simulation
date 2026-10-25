#include "molecular.h"
#include "wavefunction.h"
#include "parallel.h"
#include "element_data.h"
#include <complex>
#include <cmath>
#include <algorithm>
#include <utility>
#include <iostream>

using namespace std::complex_literals;

namespace MolecularMode {

bool enabled = false;
std::vector<MolAtom> atoms;
VolumeRenderer* sharedRenderer = nullptr;

namespace {
struct Key { double Z; int n, l, m; float x, y, z; };
std::vector<Key> lastAtoms;
int    lastRes   = -1;
double lastHalf  = -1.0;
bool   haveCache = false;
}

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
    a.Z_eff = elem->Z_eff;
    if (!(a.Z_eff > 0.0)) a.Z_eff = elem->Z;  // fallback to nuclear charge
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
    const std::vector<float>::size_type resSize = static_cast<std::vector<float>::size_type>(resolution);
    output.resize(resSize * resSize * resSize);
    if (atoms.empty() || resolution < 2) {
        std::fill(output.begin(), output.end(), 0.0f);
        return;
    }

    struct Source { OrbitalEvaluator orb; double px, py, pz, coeff; };
    std::vector<Source> sources;
    sources.reserve(atoms.size());
    for (const auto& a : atoms) {
        sources.push_back({OrbitalEvaluator(a.n, a.l, a.m, a.Z_eff),
                           static_cast<double>(a.position.x),
                           static_cast<double>(a.position.y),
                           static_cast<double>(a.position.z),
                           1.0});   // in-phase (bonding) LCAO combination
    }

    const double dx = 2.0 * halfSize / (resolution - 1);
    const int rows = resolution * resolution;

    parallelFor(0, rows, [&](int row) {
        const int iz = row / resolution;
        const int iy = row % resolution;
        const double wz = -halfSize + iz * dx;
        const double wy = -halfSize + iy * dx;
        const std::size_t base = static_cast<std::size_t>(row) * resSize;
        for (int ix = 0; ix < resolution; ++ix) {
            const double wx = -halfSize + ix * dx;

            std::complex<double> psi_total = 0.0;
            for (const auto& s : sources)
                psi_total += s.coeff * s.orb.psi(wx - s.px, wy - s.py, wz - s.pz);

            output[base + ix] = static_cast<float>(std::norm(psi_total));
        }
    });

    normaliseToUnitMax(output);
}

void updateAndRender(glm::mat4 view, glm::mat4 proj, glm::vec3 eye,
                     int resolution, double halfSize) {
    if (!sharedRenderer || atoms.empty()) return;

    std::vector<Key> cur;
    cur.reserve(atoms.size());
    for (const auto& a : atoms)
        cur.push_back({a.Z_eff, a.n, a.l, a.m, a.position.x, a.position.y, a.position.z});

    bool same = haveCache && (resolution == lastRes) && (halfSize == lastHalf) &&
                (cur.size() == lastAtoms.size());
    for (size_t i = 0; same && i < cur.size(); ++i) {
        const Key& p = cur[i];
        const Key& q = lastAtoms[i];
        same = p.Z == q.Z && p.n == q.n && p.l == q.l && p.m == q.m &&
               p.x == q.x && p.y == q.y && p.z == q.z;
    }

    if (!same) {
        std::vector<float> data;
        computeCombined(resolution, halfSize, data);
        sharedRenderer->uploadRawData(data, halfSize);
        lastAtoms = std::move(cur);
        lastRes = resolution;
        lastHalf = halfSize;
        haveCache = true;
    }
    sharedRenderer->render(view, proj, eye);
}

void shutdown() {
    haveCache = false;
    delete sharedRenderer;
    sharedRenderer = nullptr;
    enabled = false;
}

} // namespace MolecularMode
