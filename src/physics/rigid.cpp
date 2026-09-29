#include "physics/rigid.hpp"

#include <algorithm>
#include <cmath>

namespace spill {

Body makeDisc(Vec2 pos, float radius, float mass) {
    Body b;
    b.pos = pos;
    b.radius = radius;
    b.invMass = mass > 0.0f ? 1.0f / mass : 0.0f;
    const float inertia = 0.5f * mass * radius * radius;
    b.invInertia = inertia > 0.0f ? 1.0f / inertia : 0.0f;
    return b;
}

void RigidBodies::integrate(float dt, Vec2 gravity, float linearDamping, float angularDamping) {
    const float lin = std::exp(-linearDamping * dt);
    const float ang = std::exp(-angularDamping * dt);
    for (Body& b : items) {
        if (b.invMass == 0.0f) continue;
        b.vel = (b.vel + gravity * dt) * lin;
        b.angVel *= ang;
        b.pos += b.vel * dt;
        b.angle += b.angVel * dt;
    }
}

void RigidBodies::collideStatic(const std::vector<Capsule>& walls) {
    for (Body& b : items) {
        for (const Capsule& w : walls) collideDiscCapsule(b, w);
    }
}

void RigidBodies::collidePairs() {
    for (size_t i = 0; i < items.size(); ++i) {
        for (size_t j = i + 1; j < items.size(); ++j) collideDiscDisc(items[i], items[j]);
    }
}

namespace {

// Resolves a contact where `normal` points from the obstacle towards the body
// and `point` is where they touch. `other` is null for walls, which move with
// `surfaceVel` (zero for static ones).
void contactImpulse(Body& body, Body* other, Vec2 normal, Vec2 point, Vec2 surfaceVel = {}) {
    const Vec2 vA = body.velocityAt(point);
    const Vec2 vB = other ? other->velocityAt(point) : surfaceVel;
    const Vec2 rel = vA - vB;
    const float vn = dot(rel, normal);
    if (vn >= 0.0f) return;  // already separating

    const float restitution = other ? std::min(body.restitution, other->restitution)
                                    : body.restitution;
    const float friction = other ? std::sqrt(body.friction * other->friction) : body.friction;

    // Normal impulse. For discs the contact normal passes through the centre,
    // so there is no angular term in the normal effective mass.
    const float invMassSum = body.invMass + (other ? other->invMass : 0.0f);
    if (invMassSum == 0.0f) return;
    const float jn = -(1.0f + restitution) * vn / invMassSum;
    body.applyImpulse(normal * jn, point);
    if (other) other->applyImpulse(normal * -jn, point);

    // Friction along the tangent, clamped to the Coulomb cone. Here the lever
    // arm is perpendicular to the tangent, so rotation does enter.
    const Vec2 relAfter = body.velocityAt(point) - (other ? other->velocityAt(point) : surfaceVel);
    Vec2 tangent = relAfter - normal * dot(relAfter, normal);
    const float vt = length(tangent);
    if (vt < 1e-6f) return;
    tangent = tangent / vt;
    const float rA = cross(point - body.pos, tangent);
    float k = body.invMass + rA * rA * body.invInertia;
    if (other) {
        const float rB = cross(point - other->pos, tangent);
        k += other->invMass + rB * rB * other->invInertia;
    }
    const float jt = std::min(vt / k, friction * jn);
    body.applyImpulse(tangent * -jt, point);
    if (other) other->applyImpulse(tangent * jt, point);
}

}  // namespace

bool collideDiscCapsule(Body& body, const Capsule& wall, Vec2 surfaceVel) {
    const Vec2 closest = closestPointOnSegment(body.pos, wall.a, wall.b);
    const Vec2 d = body.pos - closest;
    const float reach = body.radius + wall.radius;
    const float dist2 = lengthSq(d);
    if (dist2 >= reach * reach) return false;
    const float dist = std::sqrt(dist2);
    Vec2 normal = dist > 1e-6f ? d / dist : Vec2{0.0f, -1.0f};
    body.pos = closest + normal * reach;
    contactImpulse(body, nullptr, normal, closest + normal * wall.radius, surfaceVel);
    return true;
}

bool collideDiscDisc(Body& a, Body& b) {
    const Vec2 d = a.pos - b.pos;
    const float reach = a.radius + b.radius;
    const float dist2 = lengthSq(d);
    if (dist2 >= reach * reach) return false;
    const float invMassSum = a.invMass + b.invMass;
    if (invMassSum == 0.0f) return false;
    const float dist = std::sqrt(dist2);
    const Vec2 normal = dist > 1e-6f ? d / dist : Vec2{0.0f, -1.0f};
    const float overlap = reach - dist;
    a.pos += normal * (overlap * a.invMass / invMassSum);
    b.pos -= normal * (overlap * b.invMass / invMassSum);
    contactImpulse(a, &b, normal, b.pos + normal * b.radius);
    return true;
}

}  // namespace spill
