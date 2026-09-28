#pragma once

#include "physics/geometry.hpp"

namespace spill {

// Pushes a particle out of a capsule and adjusts its previous position so the
// velocity recovered from (pos - prev) has no component into the wall and a
// reduced tangential component. `radius` is the particle's own radius.
//
// If the particle crossed the capsule's centre line during this step it is
// pushed back to the side it came from rather than the nearer side, which
// stops fast particles tunnelling through thin walls.
//
// Returns true if the particle was touching the capsule.
bool resolveParticleCapsule(Vec2& pos, Vec2& prev, float radius, const Capsule& wall,
                            float friction);

}  // namespace spill
