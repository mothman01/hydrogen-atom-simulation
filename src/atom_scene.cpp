#include "atom_scene.h"
#include <iostream>

bool AtomScene::init(int volumeResolution) {
    volumeRes_ = volumeResolution;
    return true;
}

void AtomScene::addAtom(const ElementInfo* elem, glm::vec3 pos) {
    if (!elem) return;
    PlacedAtom atom;
    atom.element = elem;
    atom.position = pos;
    atom.n = currentN_;
    atom.l = currentL_;
    atom.m = currentM_;

    if (!atom.renderer) atom.renderer = std::make_unique<VolumeRenderer>();
    if (!atom.renderer->init(volumeRes_)) {
        std::cerr << "Failed to init renderer for " << elem->symbol << "\n";
        return;
    }
    atom.renderer->setOrbital(elem->Z, atom.n, atom.l, atom.m);
    atom.renderer->setPosition(pos);

    static const char* labels = "spdf";
    int atomN = atom.n;
    int atomL = atom.l;
    atoms_.push_back(std::move(atom));

    std::cout << "Placed " << elem->symbol << " (" << elem->name
              << ") at (" << pos.x << ", " << pos.y << ", " << pos.z << ")"
              << " orbital " << atomN << labels[atomL]
              << " Z_eff=" << elem->Z_eff << "\n";
}

void AtomScene::removeLast() {
    if (!atoms_.empty()) atoms_.pop_back();
}

void AtomScene::clear() {
    atoms_.clear();
}

void AtomScene::render(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& eye) {
    for (auto& atom : atoms_) {
        if (atom.renderer)
            atom.renderer->render(view, proj, eye);
    }
}
