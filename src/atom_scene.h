#pragma once

#include "wavefunction.h"
#include "volume_renderer.h"
#include "element_data.h"
#include <vector>
#include <memory>
#include <glm/glm.hpp>

/**
 * One atom placed in the 3D scene.
 */
struct PlacedAtom {
    const ElementInfo* element;
    glm::vec3 position;
    int n;  // principal quantum number for orbital display
    int l;  // angular momentum
    int m;  // magnetic
    std::unique_ptr<VolumeRenderer> renderer;

    PlacedAtom() : element(nullptr), position(0), n(1), l(0), m(0) {}
};

/**
 * Manages the collection of atoms in the scene.
 */
class AtomScene {
public:
    bool init(int volumeResolution);
    void addAtom(const ElementInfo* elem, glm::vec3 pos);
    void removeLast();
    void clear();
    void render(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& eye);

    size_t count() const { return atoms_.size(); }
    const std::vector<PlacedAtom>& atoms() const { return atoms_; }

    // Current orbital being applied to new atoms
    int currentN() const { return currentN_; }
    int currentL() const { return currentL_; }
    int currentM() const { return currentM_; }
    void setOrbital(int n, int l, int m) { currentN_=n; currentL_=l; currentM_=m; }

private:
    std::vector<PlacedAtom> atoms_;
    int volumeRes_ = 128;
    int currentN_ = 1, currentL_ = 0, currentM_ = 0;
};
