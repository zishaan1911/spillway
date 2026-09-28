#pragma once

#include <vector>

#include "game/emitter.hpp"
#include "game/level.hpp"
#include "physics/world.hpp"

namespace spill {

// One attempt at a level: owns the physics world and applies the rules.
// Nothing in here knows about rendering or input devices.
class Session {
public:
    enum class State { Planning, Flowing, Won, Lost };

    static constexpr float kFrameDt = 1.0f / 60.0f;
    static constexpr Aabb kArena{{0.0f, 0.0f}, {1280.0f, 720.0f}};

    explicit Session(const Level& level);

    const Level& level() const { return level_; }
    const World& world() const { return world_; }
    World& world() { return world_; }
    const std::vector<Emitter>& emitters() const { return emitters_; }
    State state() const { return state_; }

    // Opens the taps. Before this the player can plan without the clock running.
    void release();

    // Advances one fixed frame.
    void update();

    int inGoal() const { return inGoal_; }
    float elapsed() const { return elapsed_; }

private:
    void rebuildWalls();
    int countInGoals() const;

    Level level_;
    World world_;
    std::vector<Emitter> emitters_;
    State state_ = State::Planning;
    int inGoal_ = 0;
    float elapsed_ = 0.0f;
};

}  // namespace spill
