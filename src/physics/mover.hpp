#pragma once

#include <vector>

#include "physics/geometry.hpp"

namespace spill {

// A scripted path for a group of walls.
struct Motion {
    enum class Kind { Slide, Spin, Shift };
    Kind kind = Kind::Slide;
    Vec2 offset;                // slide and shift: full travel
    float period = 2.0f;        // slide: seconds for there and back
    float phase = 0.0f;         // slide: starting point, as a fraction of the period
    Vec2 pivot;                 // spin: centre of rotation
    float angularSpeed = 0.0f;  // spin: radians per second
    float duration = 1.0f;      // shift: seconds to complete the move
    Aabb trigger{};             // shift: starts once fluid enters this box
};

// A rigid group of capsules moved along a Motion. Movers are kinematic: they
// push water and balls around, and nothing pushes back.
class Mover {
public:
    Mover(std::vector<Capsule> shape, const Motion& motion);

    // Poses the group for the given time. Shapes are given in world space at
    // time zero.
    void update(float time);

    // Starts a Shift. Does nothing for other kinds or if already started.
    void trigger(float time);
    bool triggered() const { return triggerTime_ >= 0.0f; }

    const Motion& motion() const { return motion_; }
    const std::vector<Capsule>& capsules() const { return current_; }
    const std::vector<Capsule>& shape() const { return shape_; }

    // Where a point that was at p at time zero is now.
    Vec2 transform(Vec2 p) const;
    // Velocity of the group's material at world point p.
    Vec2 velocityAt(Vec2 p) const;
    Vec2 translation() const { return translation_; }

private:
    std::vector<Capsule> shape_;
    std::vector<Capsule> current_;
    Motion motion_;
    float triggerTime_ = -1.0f;

    Vec2 translation_;
    float angle_ = 0.0f;
    Vec2 linearVel_;
    float angularVel_ = 0.0f;
};

}  // namespace spill
