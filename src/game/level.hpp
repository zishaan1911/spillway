#pragma once

#include <optional>
#include <string>
#include <vector>

#include "physics/geometry.hpp"
#include "physics/mover.hpp"

namespace spill {

struct EmitterDef {
    Vec2 pos;
    Vec2 dir{0.0f, 1.0f};
    float speed = 200.0f;
    int total = 600;
    float width = 24.0f;  // nozzle width; with speed this sets the flow rate
};

struct GoalDef {
    Aabb area;
    int mover = -1;  // index into Level::movers if the goal rides on one
};

// Walls that move together along a scripted path.
struct MoverDef {
    Motion motion;
    std::vector<Capsule> walls;
};

struct ZoneDef {
    Aabb area;
    Vec2 accel;
};

struct BallDef {
    Vec2 pos;
    float radius = 16.0f;
    float density = 0.5f;  // relative to the fluid
};

// Everything that defines a puzzle. Loaded from a small line-based text
// format; see levels/README.md for the syntax.
struct Level {
    std::string id;
    std::string name = "Untitled";
    std::string hint;
    float ink = 600.0f;  // how much wall the player may draw, in world units
    int target = 300;    // particles needed in the goal
    float par = 30.0f;   // seconds, for the time star
    float hold = 1.0f;   // seconds the goal must stay full

    std::vector<EmitterDef> emitters;
    std::vector<Capsule> walls;
    std::vector<MoverDef> movers;
    std::vector<Capsule> fakes;  // drawn like walls; water goes straight through
    std::vector<GoalDef> goals;
    std::vector<Aabb> drains;
    std::vector<ZoneDef> zones;
    std::vector<BallDef> balls;
    std::vector<Aabb> noDraw;

    // Reference strokes that solve the level. The tests replay them to prove
    // every shipped level can be won; players never see them.
    std::vector<std::vector<Vec2>> solution;

    int totalFluid() const;
};

struct ParseResult {
    std::optional<Level> level;
    std::string error;  // "line N: message" when level is empty
};

ParseResult parseLevel(const std::string& text, const std::string& id);

}  // namespace spill
