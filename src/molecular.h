#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "volume_renderer.h"

struct MolAtom {
    const struct ElementInfo* element;
    glm::vec3 position;
    int Z_eff;
    int n, l, m;
};

namespace MolecularMode {
    extern bool enabled;
    extern std::vector<MolAtom> atoms;
    extern VolumeRenderer* sharedRenderer;

    void init();
    void addAtom(const ElementInfo* elem, glm::vec3 pos, int n, int l, int m);
    void removeLast();
    void clear();
    void computeCombined(int resolution, double halfSize, std::vector<float>& output);
    void updateAndRender(glm::mat4 view, glm::mat4 proj, glm::vec3 eye,
                         int resolution, double halfSize);
    void shutdown();
}
