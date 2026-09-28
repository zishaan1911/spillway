#include "game/session.hpp"

#include <algorithm>

namespace spill {

Session::Session(const Level& level) : level_(level) {
    world_.configure(kArena, WorldParams{}, FluidParams{});
    for (const EmitterDef& e : level_.emitters) {
        emitters_.emplace_back(e, world_.params().particleSpacing);
    }
    for (const Aabb& d : level_.drains) world_.drains.push_back({d});
    for (const ZoneDef& z : level_.zones) world_.zones.push_back({z.area, z.accel});
    for (const BallDef& b : level_.balls) world_.addDisc(b.pos, b.radius, b.density);
    rebuildWalls();
}

void Session::rebuildWalls() {
    std::vector<Capsule> walls = level_.walls;
    for (const Stroke& s : strokes_) {
        for (size_t i = 1; i < s.points.size(); ++i) {
            walls.push_back({s.points[i - 1], s.points[i], kStrokeRadius});
        }
    }
    world_.setWalls(std::move(walls));
}

bool Session::canDrawAt(Vec2 p) const {
    if (state_ == State::Won || state_ == State::Lost) return false;
    if (!kArena.contains(p)) return false;
    for (const Aabb& a : level_.noDraw) {
        if (a.contains(p)) return false;
    }
    return true;
}

float Session::inkUsed() const {
    float used = 0.0f;
    for (const Stroke& s : strokes_) used += s.length;
    return used;
}

bool Session::beginStroke(Vec2 p) {
    if (drawing_ || !canDrawAt(p) || inkLeft() <= 0.0f) return false;
    strokes_.push_back({{p}, 0.0f});
    drawing_ = true;
    return true;
}

void Session::extendStroke(Vec2 p) {
    if (!drawing_) return;
    Stroke& s = strokes_.back();
    bool changed = false;
    for (;;) {
        const Vec2 last = s.points.back();
        const Vec2 d = p - last;
        const float dist = length(d);
        const float step = std::min({kStrokeStep, dist, inkLeft()});
        // Wait for the pointer to move a full step, unless ink is about to run
        // out, in which case spend the remainder.
        if (step <= 0.5f || (dist < kStrokeStep && step == dist)) break;
        const Vec2 next = last + d * (step / dist);
        if (!canDrawAt(next)) {
            endStroke();
            break;
        }
        s.points.push_back(next);
        s.length += step;
        changed = true;
    }
    if (changed) rebuildWalls();
}

void Session::endStroke() {
    if (!drawing_) return;
    drawing_ = false;
    if (strokes_.back().points.size() < 2) strokes_.pop_back();
}

void Session::release() {
    if (state_ == State::Planning) state_ = State::Flowing;
}

int Session::countInGoals() const {
    int count = 0;
    for (const Vec2& p : world_.fluid.pos) {
        for (const GoalDef& g : level_.goals) {
            if (g.area.contains(p)) {
                ++count;
                break;
            }
        }
    }
    return count;
}

void Session::update() {
    if (state_ == State::Flowing) {
        elapsed_ += kFrameDt;
        for (Emitter& e : emitters_) e.update(kFrameDt, world_.fluid);
    }
    // The world keeps running after the result so the water settles naturally,
    // and before release so balls can come to rest.
    world_.step(kFrameDt);
    inGoal_ = countInGoals();
}

}  // namespace spill
