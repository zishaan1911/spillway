#include "physics/mover.hpp"

#include <algorithm>
#include <cmath>

namespace spill {

namespace {
constexpr float kTwoPi = 6.28318531f;
}  // namespace

Mover::Mover(std::vector<Capsule> shape, const Motion& motion)
    : shape_(std::move(shape)), current_(shape_), motion_(motion) {}

void Mover::trigger(float time) {
    if (motion_.kind == Motion::Kind::Shift && !triggered()) triggerTime_ = time;
}

void Mover::update(float time) {
    translation_ = {};
    angle_ = 0.0f;
    linearVel_ = {};
    angularVel_ = 0.0f;

    switch (motion_.kind) {
        case Motion::Kind::Slide: {
            // Eased there-and-back: s goes 0 -> 1 -> 0 once per period.
            const float w = kTwoPi / std::max(motion_.period, 1e-3f);
            const float theta = w * time + kTwoPi * motion_.phase;
            translation_ = motion_.offset * (0.5f - 0.5f * std::cos(theta));
            linearVel_ = motion_.offset * (0.5f * w * std::sin(theta));
            break;
        }
        case Motion::Kind::Spin:
            angle_ = motion_.angularSpeed * time;
            angularVel_ = motion_.angularSpeed;
            break;
        case Motion::Kind::Shift:
            if (triggered()) {
                const float d = std::max(motion_.duration, 1e-3f);
                const float u = std::clamp((time - triggerTime_) / d, 0.0f, 1.0f);
                translation_ = motion_.offset * (u * u * (3.0f - 2.0f * u));  // smoothstep
                linearVel_ = motion_.offset * (6.0f * u * (1.0f - u) / d);
            }
            break;
    }

    for (size_t i = 0; i < shape_.size(); ++i) {
        current_[i] = {transform(shape_[i].a), transform(shape_[i].b), shape_[i].radius};
    }
}

Vec2 Mover::transform(Vec2 p) const {
    if (angle_ == 0.0f) return p + translation_;
    const float c = std::cos(angle_);
    const float s = std::sin(angle_);
    const Vec2 r = p - motion_.pivot;
    return motion_.pivot + Vec2{r.x * c - r.y * s, r.x * s + r.y * c} + translation_;
}

Vec2 Mover::velocityAt(Vec2 p) const {
    return linearVel_ + perp(p - (motion_.pivot + translation_)) * angularVel_;
}

}  // namespace spill
