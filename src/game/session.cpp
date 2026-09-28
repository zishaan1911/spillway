#include "game/session.hpp"

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
    world_.setWalls(level_.walls);
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
