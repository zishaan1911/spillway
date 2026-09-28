#pragma once

#include <vector>

#include "physics/collider_grid.hpp"
#include "physics/fluid.hpp"
#include "physics/rigid.hpp"

namespace spill {

// A region that accelerates everything inside it: fans, currents, updrafts.
struct ForceZone {
    Aabb area;
    Vec2 accel;
};

// A region that swallows fluid.
struct Drain {
    Aabb area;
};

struct WorldParams {
    Vec2 gravity{0.0f, 980.0f};
    float particleRadius = 3.0f;   // collision radius of a fluid particle
    float particleSpacing = 7.0f;  // typical rest spacing, used to size bodies
    float wallFriction = 0.06f;
    float bodyFriction = 0.15f;
    float maxSpeed = 1100.0f;
    int substeps = 4;
    float bodyLinearDamping = 0.1f;
    float bodyAngularDamping = 0.6f;
    float escapeMargin = 60.0f;  // how far outside the bounds a particle is lost
};

// Everything that moves, advanced with a fixed number of substeps per frame.
class World {
public:
    void configure(Aabb bounds, const WorldParams& params, const FluidParams& fluidParams);

    const Aabb& bounds() const { return bounds_; }
    const WorldParams& params() const { return params_; }

    void setWalls(std::vector<Capsule> walls);
    const std::vector<Capsule>& walls() const { return walls_; }

    // Mass of a disc of the given radius whose density is `relativeDensity`
    // times the fluid's. Below 1 floats, above 1 sinks.
    float discMass(float radius, float relativeDensity) const;
    Body& addDisc(Vec2 pos, float radius, float relativeDensity);

    void step(float frameDt);

    Fluid fluid;
    RigidBodies bodies;
    std::vector<ForceZone> zones;
    std::vector<Drain> drains;

    // Particles removed so far, by cause.
    int drained = 0;
    int escaped = 0;

private:
    void substep(float dt);
    void collideParticles(float dt);
    void removeLostParticles();

    Aabb bounds_{};
    WorldParams params_{};
    std::vector<Capsule> walls_;
    ColliderGrid colliderGrid_;
};

}  // namespace spill
