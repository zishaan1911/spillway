#include "physics/contact.hpp"

#include <cmath>

#include "physics/rigid.hpp"

namespace spill {

namespace {

// True if the path prev -> pos crosses the segment ab.
bool crossesSegment(Vec2 prev, Vec2 pos, Vec2 a, Vec2 b) {
    const Vec2 ab = b - a;
    const float s0 = cross(ab, prev - a);
    const float s1 = cross(ab, pos - a);
    if (s0 * s1 >= 0.0f) return false;
    const Vec2 path = pos - prev;
    const float denom = cross(ab, path);
    if (std::fabs(denom) < 1e-12f) return false;
    const float t = cross(prev - a, path) / denom;
    return t >= 0.0f && t <= 1.0f;
}

}  // namespace

bool resolveParticleCapsule(Vec2& pos, Vec2& prev, float radius, const Capsule& wall,
                            float friction) {
    const float reach = wall.radius + radius;
    const Vec2 closest = closestPointOnSegment(pos, wall.a, wall.b);
    Vec2 offset = pos - closest;
    const float dist2 = lengthSq(offset);
    const bool crossed = crossesSegment(prev, pos, wall.a, wall.b);
    if (dist2 >= reach * reach && !crossed) return false;

    Vec2 normal;
    if (crossed) {
        // Back to the side the particle came from.
        normal = normalize(perp(wall.b - wall.a));
        if (dot(prev - wall.a, normal) < 0.0f) normal = -normal;
    } else if (dist2 > 1e-12f) {
        normal = offset / std::sqrt(dist2);
    } else {
        normal = normalize(perp(wall.b - wall.a));
        if (lengthSq(normal) == 0.0f) normal = {0.0f, -1.0f};
    }

    pos = closest + normal * reach;

    // Rewrite this step's displacement: no motion into the wall, and friction
    // on the sliding part.
    const Vec2 disp = pos - prev;
    float along = dot(disp, normal);
    const Vec2 tangential = disp - normal * along;
    if (along < 0.0f) along = 0.0f;
    prev = pos - (normal * along + tangential * (1.0f - friction));
    return true;
}

bool resolveParticleDisc(Vec2& pos, Vec2& prev, float radius, float particleMass, Body& body,
                         float friction, float dt) {
    const Vec2 d = pos - body.pos;
    const float reach = body.radius + radius;
    const float dist2 = lengthSq(d);
    if (dist2 >= reach * reach) return false;
    const float dist = std::sqrt(dist2);
    const Vec2 normal = dist > 1e-6f ? d / dist : Vec2{0.0f, -1.0f};
    const Vec2 contact = body.pos + normal * body.radius;

    // Velocity of the particle relative to the surface it is touching.
    const Vec2 v = (pos - prev) / dt;
    const Vec2 rel = v - body.velocityAt(contact);
    const float vn = dot(rel, normal);
    const Vec2 relTangent = rel - normal * vn;

    Vec2 dv = relTangent * -friction;
    if (vn < 0.0f) dv -= normal * vn;

    // Reaction on the body. Split by effective mass so a light body is not
    // kicked harder than the particle itself; the normal passes through the
    // centre so only the tangential part spins the disc.
    const float share = body.invMass > 0.0f
                            ? (1.0f / particleMass) / (1.0f / particleMass + body.invMass)
                            : 1.0f;
    const Vec2 impulse = dv * (particleMass * share);
    body.applyImpulse(-impulse, contact);

    const Vec2 vAfter = v + dv * share;
    pos = body.pos + normal * reach;
    prev = pos - vAfter * dt;
    return true;
}

}  // namespace spill
