#pragma once

// Full periodic table data for all 118 known elements.
// Each entry provides the atomic number, symbol, name, category,
// electron configuration, and an effective nuclear charge (Z_eff)
// for hydrogenic-approximation wavefunction rendering.
//
// Z_eff values are approximate Slater/Zener effective nuclear charges
// for the outermost electron.

struct ElementInfo {
    int Z;                    // atomic number 1–118
    const char* symbol;       // e.g. "H", "He", "Li"
    const char* name;         // e.g. "Hydrogen"
    const char* category;     // "Alkali Metal", "Noble Gas", etc.
    const char* config;       // e.g. "1s¹"
    int period;               // 1–7
    int group;                // 1–18 (or 0 for lanthanides/actinides)
    double Z_eff;             // effective nuclear charge for outermost electron
    int outermost_n;          // principal quantum number of outermost shell
    int outermost_l;          // angular momentum of outermost orbital (0=s,1=p,2=d,3=f)
};

extern const ElementInfo ELEMENTS[119];  // index 0 is a dummy, 1–118 are real
extern const int NUM_ELEMENTS;           // = 118

// Lookup helpers
const ElementInfo* findElementBySymbol(const char* symbol);
const ElementInfo* findElementByZ(int Z);
