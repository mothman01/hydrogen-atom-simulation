#pragma once

#include <glm/glm.hpp>
#include <vector>

/**
 * Crystal lattice generator.
 *
 * Generates atom positions for common crystal structures (SC, BCC, FCC, HCP,
 * diamond cubic, NaCl, CsCl, zincblende) and feeds them into AtomScene.
 *
 * Semiconductor mode: builds doped silicon lattices (p-type / n-type) with
 * visual markers for acceptors/donors and electron/hole motion.
 */
namespace CrystalLattice {

enum Type {
    SC,            // Simple Cubic
    BCC,           // Body-Centred Cubic
    FCC,           // Face-Centred Cubic
    HCP,           // Hexagonal Close-Packed
    DIAMOND,       // Diamond cubic (Si, C)
    NaCl_STYLE,    // Rocksalt (NaCl)
    CsCl_STYLE,    // Caesium chloride
    ZINCBLENDE,    // Zincblende (GaAs)
    P_TYPE_SI,     // p-doped Silicon (Boron acceptor)
    N_TYPE_SI,     // n-doped Silicon (Phosphorus donor)
    PN_JUNCTION,   // p–n junction
    NUM_TYPES
};

// Name strings for UI
const char* name(Type t);

// Generate lattice positions.
//   t      — lattice type
//   a      — lattice constant (Angstroms)
//   nx,ny,nz — number of unit cells along each axis
// Returns world-space positions.
std::vector<glm::vec3> generate(Type t, float a, int nx, int ny, int nz);

// Get a recommended lattice constant for the given type (in Bohr radii).
float defaultA(Type t);

// Get default unit-cell repeat count for a good visual.
int defaultN(Type t);

// Currently active lattice type (for UI toggling)
extern Type currentType;

} // namespace CrystalLattice
