#include "game/emitter.hpp"

#include <algorithm>
#include <cmath>

namespace spill {

Emitter::Emitter(const EmitterDef& def, float spacing)
    : def_(def),
      spacing_(spacing),
      perRow_(std::max(1, static_cast<int>(std::lround(def.width / spacing)))) {
    // Start one spacing "due" so the first row appears immediately.
    travelled_ = spacing_;
}

float Emitter::ratePerSecond() const {
    return static_cast<float>(perRow_) * def_.speed / spacing_;
}

int Emitter::update(float dt, Fluid& fluid) {
    if (empty() || def_.speed <= 0.0f) return 0;
    travelled_ += def_.speed * dt;
    const Vec2 across = perp(def_.dir);
    int added = 0;
    while (travelled_ >= spacing_ && !empty()) {
        travelled_ -= spacing_;
        // A row laid down part-way through the frame has already moved.
        const Vec2 base = def_.pos + def_.dir * travelled_;
        // Stagger alternate rows by half a spacing so the stream packs
        // hexagonally rather than in a square lattice.
        const float stagger = (row_ % 2 == 0) ? 0.0f : 0.5f * spacing_;
        const int count = std::min(perRow_, remaining());
        for (int k = 0; k < count; ++k) {
            const float offset = (static_cast<float>(k) - 0.5f * (perRow_ - 1)) * spacing_ + stagger;
            fluid.add(base + across * offset, def_.dir * def_.speed);
        }
        emitted_ += count;
        added += count;
        ++row_;
    }
    return added;
}

}  // namespace spill
