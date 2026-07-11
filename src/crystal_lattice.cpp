#include "crystal_lattice.h"

#include <cmath>
#include <algorithm>

namespace CrystalLattice {

Type currentType = SC;

const char* name(Type t) {
    switch (t) {
        case SC:          return "Simple Cubic";
        case BCC:         return "BCC (Body-Centred)";
        case FCC:         return "FCC (Face-Centred)";
        case HCP:         return "HCP (Hexagonal)";
        case DIAMOND:     return "Diamond Cubic";
        case NaCl_STYLE:  return "NaCl (Rocksalt)";
        case CsCl_STYLE:  return "CsCl";
        case ZINCBLENDE:  return "Zincblende (GaAs)";
        case P_TYPE_SI:   return "p-type Si (B-doped)";
        case N_TYPE_SI:   return "n-type Si (P-doped)";
        case PN_JUNCTION: return "p–n Junction";
        default:          return "Unknown";
    }
}

float defaultA(Type t) {
    switch (t) {
        case SC: case BCC: case FCC:       return 6.0f;
        case HCP:                           return 5.0f;
        case DIAMOND:                       return 10.26f; // Si: 5.43 Å ≈ 10.26 a₀
        case NaCl_STYLE: case CsCl_STYLE:   return 6.0f;
        case ZINCBLENDE:                    return 10.7f;  // GaAs: 5.65 Å
        case P_TYPE_SI: case N_TYPE_SI:
        case PN_JUNCTION:                   return 10.26f;
        default:                            return 6.0f;
    }
}

int defaultN(Type t) {
    switch (t) {
        case HCP:         return 3;
        case PN_JUNCTION: return 3;
        default:          return 2;
    }
}

// ── Basis atoms in fractional coordinates ──────────────────────────────
struct Basis {
    float x, y, z;
    bool isAlt;  // alternate element (for compounds/semiconductors)
};

static std::vector<glm::vec3> generateUnitCell(Type t, float a) {
    std::vector<Basis> basis;

    switch (t) {
    case SC:
        basis = {{0,0,0,false}};
        break;

    case BCC:
        basis = {{0,0,0,false}, {0.5f,0.5f,0.5f,false}};
        break;

    case FCC:
        basis = {
            {0,0,0,false}, {0,0.5f,0.5f,false},
            {0.5f,0,0.5f,false}, {0.5f,0.5f,0,false}
        };
        break;

    case HCP: {
        float sr3_2 = std::sqrt(3.0f) / 2.0f;
        basis = {
            {0,0,0,false}, {2.0f/3.0f, 1.0f/3.0f, 0.5f,false},
            // second-layer offsets (for multi-cell)
        };
        break;
    }

    case DIAMOND:
        basis = {
            {0,0,0,false}, {0,0.5f,0.5f,false},
            {0.5f,0,0.5f,false}, {0.5f,0.5f,0,false},
            {0.25f,0.25f,0.25f,false}, {0.25f,0.75f,0.75f,false},
            {0.75f,0.25f,0.75f,false}, {0.75f,0.75f,0.25f,false}
        };
        break;

    case NaCl_STYLE:
        // Na at FCC sites, Cl at octahedral holes
        basis = {
            {0,0,0,false}, {0,0.5f,0.5f,false},
            {0.5f,0,0.5f,false}, {0.5f,0.5f,0,false},
            {0.5f,0.5f,0.5f,true}, {0.5f,0,0,true},
            {0,0.5f,0,true}, {0,0,0.5f,true}
        };
        break;

    case CsCl_STYLE:
        basis = {{0,0,0,false}, {0.5f,0.5f,0.5f,true}};
        break;

    case ZINCBLENDE:
        // Ga at FCC, As at (¼,¼,¼) offset
        basis = {
            {0,0,0,false}, {0,0.5f,0.5f,false},
            {0.5f,0,0.5f,false}, {0.5f,0.5f,0,false},
            {0.25f,0.25f,0.25f,true}, {0.25f,0.75f,0.75f,true},
            {0.75f,0.25f,0.75f,true}, {0.75f,0.75f,0.25f,true}
        };
        break;

    case P_TYPE_SI:
        // Diamond lattice + one Boron substitution per unit cell
        basis = {
            {0,0,0,false}, {0,0.5f,0.5f,false},
            {0.5f,0,0.5f,false}, {0.5f,0.5f,0,false},
            {0.25f,0.25f,0.25f,false}, {0.25f,0.75f,0.75f,false},
            {0.75f,0.25f,0.75f,false}, {0.75f,0.75f,0.25f,false},
            // Boron acceptor — replace one Si with B (shown in different color)
            {0.125f,0.125f,0.125f,true}
        };
        break;

    case N_TYPE_SI:
        basis = {
            {0,0,0,false}, {0,0.5f,0.5f,false},
            {0.5f,0,0.5f,false}, {0.5f,0.5f,0,false},
            {0.25f,0.25f,0.25f,false}, {0.25f,0.75f,0.75f,false},
            {0.75f,0.25f,0.75f,false}, {0.75f,0.75f,0.25f,false},
            // Phosphorus donor
            {0.625f,0.625f,0.625f,true}
        };
        break;

    case PN_JUNCTION:
        // Left half p-type, right half n-type
        for (int i = 0; i < 8; ++i) {
            float x = (i & 1) ? 0.5f : 0.0f;
            float y = (i & 2) ? 0.5f : 0.0f;
            float z = (i & 4) ? 0.5f : 0.0f;
            basis.push_back({x, y, z, false});
        }
        // p-side acceptor (left)
        basis.push_back({0.125f, 0.125f, 0.125f, true});
        // n-side donor (right)
        basis.push_back({0.625f, 0.625f, 0.625f, true});
        break;

    default:
        basis = {{0,0,0,false}};
        break;
    }

    std::vector<glm::vec3> out;
    for (auto& b : basis)
        out.push_back(glm::vec3(b.x * a, b.y * a, b.z * a));
    return out;
}

std::vector<glm::vec3> generate(Type t, float a, int nx, int ny, int nz) {
    std::vector<glm::vec3> out;
    auto basis = generateUnitCell(t, a);
    float offsetX = -(nx - 1) * a * 0.5f;
    float offsetY = -(ny - 1) * a * 0.5f;
    float offsetZ = -(nz - 1) * a * 0.5f;

    for (int ix = 0; ix < nx; ++ix) {
        for (int iy = 0; iy < ny; ++iy) {
            for (int iz = 0; iz < nz; ++iz) {
                glm::vec3 cellOrigin(
                    offsetX + ix * a,
                    offsetY + iy * a,
                    offsetZ + iz * a
                );
                for (auto& b : basis)
                    out.push_back(cellOrigin + b);
            }
        }
    }
    return out;
}

} // namespace CrystalLattice
