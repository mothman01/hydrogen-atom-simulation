#include "atom_scene.h"
#include <algorithm>
#include <iostream>
#include <utility>

bool AtomScene::init(int volumeResolution) {
    volumeRes_ = volumeResolution;
    return true;
}

void AtomScene::setAsyncComputer(AsyncVolumeComputer* async) {
    async_ = async;
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
    atom.renderer->setOrbital(elem->Z_eff, atom.n, atom.l, atom.m);
    if (async_) atom.renderer->setAsyncComputer(async_);
    atom.renderer->setPosition(pos);

    static const char* labels = "spdfgh";
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

void AtomScene::updateAllOrbitals(int n, int l, int m) {
    currentN_ = n; currentL_ = l; currentM_ = m;
    for (auto& atom : atoms_) {
        atom.n = n;
        atom.l = l;
        atom.m = m;
        if (atom.renderer && atom.element) {
            atom.renderer->setOrbital(atom.element->Z_eff, n, l, m);
        }
    }
}

void AtomScene::render(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& eye) {
    // Large scenes (crystal lattices) trade a little ray-march detail for a
    // usable frame rate; the shader corrects opacity for the step size.
    const float stepScale = atoms_.size() > 16 ? 2.0f : 1.0f;

    // Translucent volumes must be composited back-to-front.
    std::vector<std::pair<float, VolumeRenderer*>> order;
    order.reserve(atoms_.size());
    for (auto& atom : atoms_)
        if (atom.renderer) {
            glm::vec3 d = atom.position - eye;
            order.emplace_back(glm::dot(d, d), atom.renderer.get());
        }
    std::sort(order.begin(), order.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });

    for (auto& entry : order) {
        entry.second->setStepScale(stepScale);
        entry.second->render(view, proj, eye);
    }
}
