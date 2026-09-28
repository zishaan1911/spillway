#pragma once

#include "game/level.hpp"
#include "physics/fluid.hpp"

namespace spill {

// Feeds fluid into the world through a nozzle. A new row of particles is laid
// down every time the previous one has travelled one particle spacing, so the
// stream starts at rest density instead of starting compressed and spraying.
class Emitter {
public:
    Emitter(const EmitterDef& def, float spacing);

    // Returns how many particles were added.
    int update(float dt, Fluid& fluid);

    const EmitterDef& def() const { return def_; }
    int remaining() const { return def_.total - emitted_; }
    bool empty() const { return remaining() <= 0; }
    int perRow() const { return perRow_; }
    float ratePerSecond() const;

private:
    EmitterDef def_;
    float spacing_;
    int perRow_;
    float travelled_ = 0.0f;
    int emitted_ = 0;
    int row_ = 0;
};

}  // namespace spill
