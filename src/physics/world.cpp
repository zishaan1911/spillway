#include "physics/world.hpp"

#include "physics/contact.hpp"

namespace spill {

namespace {
constexpr float kPi = 3.14159265f;
constexpr float kParticleMass = 1.0f;
}  // namespace

void World::configure(Aabb bounds, const WorldParams& params, const FluidParams& fluidParams) {
    bounds_ = bounds;
    params_ = params;
    // The fluid grid covers the escape margin too, so particles on their way
    // out still find their neighbours.
    fluid.configure(bounds.expanded(params.escapeMargin), fluidParams);
    colliderGrid_.configure(bounds.expanded(params.escapeMargin), 32.0f, params.particleRadius);
    colliderGrid_.rebuild(walls_);
}

void World::setWalls(std::vector<Capsule> walls) {
    walls_ = std::move(walls);
    colliderGrid_.rebuild(walls_);
}

float World::discMass(float radius, float relativeDensity) const {
    const float particlesPerArea = 1.0f / (params_.particleSpacing * params_.particleSpacing);
    return relativeDensity * kPi * radius * radius * particlesPerArea * kParticleMass;
}

Body& World::addDisc(Vec2 pos, float radius, float relativeDensity) {
    bodies.items.push_back(makeDisc(pos, radius, discMass(radius, relativeDensity)));
    return bodies.items.back();
}

void World::step(float frameDt) {
    const int n = params_.substeps > 0 ? params_.substeps : 1;
    const float dt = frameDt / static_cast<float>(n);
    for (int i = 0; i < n; ++i) substep(dt);
    removeLostParticles();
}

void World::substep(float dt) {
    // Bodies first, so the fluid sees where they are this step.
    for (Body& b : bodies.items) {
        for (const ForceZone& z : zones) {
            if (z.area.contains(b.pos)) b.vel += z.accel * dt;
        }
    }
    bodies.integrate(dt, params_.gravity, params_.bodyLinearDamping, params_.bodyAngularDamping);
    bodies.collidePairs();
    bodies.collideStatic(walls_);

    for (size_t i = 0; i < fluid.size(); ++i) {
        Vec2 accel = params_.gravity;
        for (const ForceZone& z : zones) {
            if (z.area.contains(fluid.pos[i])) accel += z.accel;
        }
        fluid.vel[i] += accel * dt;
    }

    fluid.predict(dt);
    fluid.findPairs();
    fluid.relax(dt);
    collideParticles(dt);
    fluid.deriveVelocities(dt, params_.maxSpeed);
    fluid.applyViscosity(dt);
}

void World::collideParticles(float dt) {
    const float r = params_.particleRadius;
    for (size_t i = 0; i < fluid.size(); ++i) {
        Vec2& pos = fluid.pos[i];
        Vec2& prev = fluid.prev[i];
        colliderGrid_.forEachCandidate(pos, [&](uint32_t w) {
            resolveParticleCapsule(pos, prev, r, walls_[w], params_.wallFriction);
        });
        for (Body& b : bodies.items) {
            resolveParticleDisc(pos, prev, r, kParticleMass, b, params_.bodyFriction, dt);
        }
    }
}

void World::removeLostParticles() {
    const Aabb keep = bounds_.expanded(params_.escapeMargin);
    int drainedNow = 0;
    int escapedNow = 0;
    fluid.compact([&](size_t i) {
        const Vec2 p = fluid.pos[i];
        if (!keep.contains(p)) {
            ++escapedNow;
            return false;
        }
        for (const Drain& d : drains) {
            if (d.area.contains(p)) {
                ++drainedNow;
                return false;
            }
        }
        return true;
    });
    drained += drainedNow;
    escaped += escapedNow;
}

}  // namespace spill
