#pragma once

#include <string>
#include <vector>

#include "game/emitter.hpp"
#include "game/level.hpp"
#include "physics/world.hpp"

namespace spill {

// A wall the player drew: a polyline turned into a chain of capsules.
struct Stroke {
    std::vector<Vec2> points;
    float length = 0.0f;
};

// One attempt at a level: owns the physics world and applies the rules.
// Nothing in here knows about rendering or input devices.
class Session {
public:
    enum class State { Planning, Flowing, Won, Lost };

    static constexpr float kFrameDt = 1.0f / 60.0f;
    static constexpr Aabb kArena{{0.0f, 0.0f}, {1280.0f, 720.0f}};
    static constexpr float kStrokeRadius = 5.0f;
    static constexpr float kStrokeStep = 10.0f;  // distance between stroke points
    static constexpr float kSettleTime = 2.5f;   // stillness before declaring a loss
    static constexpr float kSettleSpeed = 12.0f;
    static constexpr float kDryTimeout = 15.0f;  // after the taps run dry

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

    // Drawing. A stroke grows in fixed steps while ink lasts and stops at
    // no-draw regions, balls and the arena edge.
    bool canDrawAt(Vec2 p) const;
    bool beginStroke(Vec2 p);
    void extendStroke(Vec2 p);
    void endStroke();
    // Draws a whole polyline as one stroke, as if traced with the mouse.
    void drawPolyline(const std::vector<Vec2>& points);
    bool drawing() const { return drawing_; }
    const std::vector<Stroke>& strokes() const { return strokes_; }
    // Removes the stroke passing nearest to p (within reach). No take-backs:
    // erasing and undo only work before the taps open, and the ink a stroke
    // cost is never refunded.
    bool eraseAt(Vec2 p, float reach = 12.0f);
    void undo();
    float inkUsed() const { return inkSpent_; }
    float inkLeft() const { return level_.ink - inkSpent_; }

    // Where goal i is right now; goals can ride on movers.
    Aabb goalArea(size_t i) const;

    int inGoal() const { return inGoal_; }
    float elapsed() const { return elapsed_; }

    // 0..1 while the goal is full and the win is being confirmed.
    float holdProgress() const { return holdTimer_ / level_.hold; }
    // Time from release until the goal first filled, for the winning fill.
    float finishTime() const { return finishTime_; }
    const std::string& lossReason() const { return lossReason_; }

    // One star for finishing, one for using at most half the ink, one for
    // beating par. Zero unless the level is won.
    int stars() const;

private:
    void rebuildWalls();
    int countInGoals() const;
    void judge();
    void lose(std::string reason);

    Level level_;
    World world_;
    std::vector<Emitter> emitters_;
    std::vector<Stroke> strokes_;
    bool drawing_ = false;
    float inkSpent_ = 0.0f;
    State state_ = State::Planning;
    int inGoal_ = 0;
    float elapsed_ = 0.0f;
    float holdTimer_ = 0.0f;
    float holdStart_ = 0.0f;
    float finishTime_ = 0.0f;
    float stillTimer_ = 0.0f;
    float dryTimer_ = 0.0f;
    std::string lossReason_;
};

}  // namespace spill
