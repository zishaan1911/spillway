#pragma once

#include <algorithm>

#include "physics/vec2.hpp"

namespace spill {

struct Aabb {
    Vec2 min;
    Vec2 max;

    bool contains(Vec2 p) const {
        return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y;
    }
    bool overlaps(const Aabb& o) const {
        return min.x <= o.max.x && max.x >= o.min.x && min.y <= o.max.y && max.y >= o.min.y;
    }
    Aabb expanded(float r) const { return {{min.x - r, min.y - r}, {max.x + r, max.y + r}}; }
    Vec2 center() const { return (min + max) * 0.5f; }
    Vec2 size() const { return max - min; }
};

// A line segment with thickness. Every static wall in the game is one of these,
// including the strokes the player draws.
struct Capsule {
    Vec2 a;
    Vec2 b;
    float radius = 1.0f;

    Aabb bounds() const {
        return {{std::min(a.x, b.x) - radius, std::min(a.y, b.y) - radius},
                {std::max(a.x, b.x) + radius, std::max(a.y, b.y) + radius}};
    }
};

// Parameter t in [0, 1] of the point on segment ab closest to p.
inline float closestSegmentParam(Vec2 p, Vec2 a, Vec2 b) {
    const Vec2 ab = b - a;
    const float denom = lengthSq(ab);
    if (denom < 1e-12f) return 0.0f;
    return std::clamp(dot(p - a, ab) / denom, 0.0f, 1.0f);
}

inline Vec2 closestPointOnSegment(Vec2 p, Vec2 a, Vec2 b) {
    return a + (b - a) * closestSegmentParam(p, a, b);
}

inline float distanceToSegment(Vec2 p, Vec2 a, Vec2 b) {
    return length(p - closestPointOnSegment(p, a, b));
}

}  // namespace spill
