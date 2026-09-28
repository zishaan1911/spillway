#pragma once

#include "game/session.hpp"
#include "raylib.h"

namespace spill {

inline Vector2 toRl(Vec2 v) { return {v.x, v.y}; }
inline Vec2 fromRl(Vector2 v) { return {v.x, v.y}; }
inline Rectangle toRect(const Aabb& a) { return {a.min.x, a.min.y, a.size().x, a.size().y}; }

namespace palette {
inline constexpr Color kBackground{14, 18, 27, 255};
inline constexpr Color kGrid{22, 28, 40, 255};
inline constexpr Color kWall{120, 134, 158, 255};
inline constexpr Color kWallEdge{168, 182, 204, 255};
inline constexpr Color kStroke{242, 176, 72, 255};
inline constexpr Color kGoal{80, 220, 140, 255};
inline constexpr Color kDrain{190, 60, 90, 255};
inline constexpr Color kZone{140, 170, 255, 255};
inline constexpr Color kNoDraw{220, 70, 70, 255};
inline constexpr Color kBall{236, 110, 80, 255};
inline constexpr Color kText{226, 232, 240, 255};
inline constexpr Color kMuted{130, 142, 162, 255};
}  // namespace palette

// Things drawn behind the fluid.
void drawBackdrop(const Session& session, float time);
// Things drawn on top of the fluid.
void drawForeground(const Session& session, float time);

void drawCapsule(const Capsule& c, Color fill);

}  // namespace spill
