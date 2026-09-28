#pragma once

#include <cmath>

namespace spill {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    constexpr Vec2() = default;
    constexpr Vec2(float x_, float y_) : x(x_), y(y_) {}

    constexpr Vec2 operator+(Vec2 o) const { return {x + o.x, y + o.y}; }
    constexpr Vec2 operator-(Vec2 o) const { return {x - o.x, y - o.y}; }
    constexpr Vec2 operator*(float s) const { return {x * s, y * s}; }
    constexpr Vec2 operator/(float s) const { return {x / s, y / s}; }
    constexpr Vec2 operator-() const { return {-x, -y}; }

    constexpr Vec2& operator+=(Vec2 o) { x += o.x; y += o.y; return *this; }
    constexpr Vec2& operator-=(Vec2 o) { x -= o.x; y -= o.y; return *this; }
    constexpr Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
};

constexpr Vec2 operator*(float s, Vec2 v) { return v * s; }

constexpr float dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
constexpr float cross(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }
constexpr float lengthSq(Vec2 v) { return dot(v, v); }
inline float length(Vec2 v) { return std::sqrt(lengthSq(v)); }

// Counter-clockwise perpendicular.
constexpr Vec2 perp(Vec2 v) { return {-v.y, v.x}; }

// Returns the zero vector instead of NaNs for degenerate input.
inline Vec2 normalize(Vec2 v) {
    const float len = length(v);
    return len > 1e-8f ? v / len : Vec2{};
}

}  // namespace spill
