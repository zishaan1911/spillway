#pragma once

#include <vector>

#include "physics/geometry.hpp"

namespace spill {

// A solid disc. Discs are the only dynamic rigid shape: they are enough for
// balls and boulders, and every contact against them has a closed form.
struct Body {
    Vec2 pos;
    Vec2 vel;
    float angle = 0.0f;
    float angVel = 0.0f;
    float radius = 10.0f;
    float invMass = 1.0f;
    float invInertia = 1.0f;
    float restitution = 0.15f;
    float friction = 0.5f;

    float mass() const { return invMass > 0.0f ? 1.0f / invMass : 0.0f; }

    // Velocity of the material point at world position p.
    Vec2 velocityAt(Vec2 p) const { return vel + perp(p - pos) * angVel; }

    // Impulse j applied at world point p.
    void applyImpulse(Vec2 j, Vec2 p) {
        vel += j * invMass;
        angVel += cross(p - pos, j) * invInertia;
    }
};

// Builds a disc whose mass is `mass` and whose inertia is that of a uniform disc.
Body makeDisc(Vec2 pos, float radius, float mass);

class RigidBodies {
public:
    std::vector<Body> items;

    void integrate(float dt, Vec2 gravity, float linearDamping, float angularDamping);
    // Discs are few and large, so they simply test every wall.
    void collideStatic(const std::vector<Capsule>& walls);
    void collidePairs();
};

// Contact between a disc and a static capsule. Corrects position fully and
// applies a normal impulse with restitution plus Coulomb friction.
bool collideDiscCapsule(Body& body, const Capsule& wall);

// Contact between two discs, splitting the correction by inverse mass.
bool collideDiscDisc(Body& a, Body& b);

}  // namespace spill
