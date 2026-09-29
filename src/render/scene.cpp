#include "render/scene.hpp"

#include <algorithm>
#include <cmath>

namespace spill {

namespace {

void drawGrid() {
    const Aabb& a = Session::kArena;
    for (float x = a.min.x; x <= a.max.x; x += 40.0f) {
        DrawLineV({x, a.min.y}, {x, a.max.y}, palette::kGrid);
    }
    for (float y = a.min.y; y <= a.max.y; y += 40.0f) {
        DrawLineV({a.min.x, y}, {a.max.x, y}, palette::kGrid);
    }
}

void drawHatch(const Aabb& area, Color c, float spacing) {
    BeginScissorMode(static_cast<int>(area.min.x), static_cast<int>(area.min.y),
                     static_cast<int>(area.size().x), static_cast<int>(area.size().y));
    const float h = area.size().y;
    for (float x = area.min.x - h; x < area.max.x; x += spacing) {
        DrawLineEx({x, area.max.y}, {x + h, area.min.y}, 2.0f, c);
    }
    EndScissorMode();
}

void drawZone(const ZoneDef& z, float time) {
    const Rectangle r = toRect(z.area);
    DrawRectangleRec(r, Fade(palette::kZone, 0.06f));
    DrawRectangleLinesEx(r, 1.0f, Fade(palette::kZone, 0.25f));
    // Chevrons drifting along the push direction.
    const Vec2 dir = normalize(z.accel);
    const Vec2 side = perp(dir) * 7.0f;
    const float strength = std::min(length(z.accel) / 1500.0f, 1.0f);
    const float step = 46.0f;
    const float shift = std::fmod(time * 60.0f * (0.5f + strength), step);
    BeginScissorMode(static_cast<int>(r.x), static_cast<int>(r.y), static_cast<int>(r.width),
                     static_cast<int>(r.height));
    for (float y = r.y + step * 0.5f; y < r.y + r.height; y += step) {
        for (float x = r.x + step * 0.5f; x < r.x + r.width; x += step) {
            const Vec2 c = Vec2{x, y} + dir * (shift - step * 0.5f);
            const Vec2 tip = c + dir * 6.0f;
            const Color col = Fade(palette::kZone, 0.2f + 0.25f * strength);
            DrawLineEx(toRl(tip), toRl(c - dir * 3.0f + side), 2.0f, col);
            DrawLineEx(toRl(tip), toRl(c - dir * 3.0f - side), 2.0f, col);
        }
    }
    EndScissorMode();
}

void drawGoal(const Aabb& area, float fill, float hold) {
    const Rectangle r = toRect(area);
    DrawRectangleRec(r, Fade(palette::kGoal, 0.07f + 0.12f * hold));
    // Progress bar along the bottom edge; it pulses while the win is confirmed.
    const float f = std::clamp(fill, 0.0f, 1.0f);
    const Color bar = hold > 0.0f ? ColorBrightness(palette::kGoal, 0.3f * hold) : palette::kGoal;
    DrawRectangleRec({r.x, r.y + r.height + 8.0f, r.width, 4.0f}, Fade(palette::kGoal, 0.2f));
    DrawRectangleRec({r.x, r.y + r.height + 8.0f, r.width * f, 4.0f}, bar);
    // Dashed outline.
    const float dash = 8.0f;
    const Color c = Fade(palette::kGoal, 0.7f);
    for (float x = r.x; x < r.x + r.width; x += dash * 2) {
        const float e = std::min(x + dash, r.x + r.width);
        DrawLineEx({x, r.y}, {e, r.y}, 2.0f, c);
        DrawLineEx({x, r.y + r.height}, {e, r.y + r.height}, 2.0f, c);
    }
    for (float y = r.y; y < r.y + r.height; y += dash * 2) {
        const float e = std::min(y + dash, r.y + r.height);
        DrawLineEx({r.x, y}, {r.x, e}, 2.0f, c);
        DrawLineEx({r.x + r.width, y}, {r.x + r.width, e}, 2.0f, c);
    }
}

void drawEmitter(const Emitter& e) {
    const EmitterDef& d = e.def();
    const float pipeWidth = d.width + 14.0f;
    const Vec2 side = perp(d.dir) * (pipeWidth * 0.5f);
    const Vec2 mouth = d.pos - d.dir * 4.0f;
    // The pipe runs back off the edge of the screen; the frame clips it.
    const Vec2 back = mouth - d.dir * 2000.0f;
    DrawLineEx(toRl(back), toRl(mouth), pipeWidth + 3.0f, palette::kWallEdge);
    DrawLineEx(toRl(back), toRl(mouth), pipeWidth, palette::kWall);  // a thick line is a rotated box
    DrawLineEx(toRl(mouth - side * 1.2f), toRl(mouth + side * 1.2f), 8.0f, palette::kWallEdge);
    // Remaining fluid, as a gauge running up the pipe from the mouth.
    const float frac = static_cast<float>(e.remaining()) / std::max(1, d.total);
    const Vec2 g0 = mouth - d.dir * 10.0f;
    if (frac > 0.0f) {
        DrawLineEx(toRl(g0), toRl(g0 - d.dir * (60.0f * frac)), d.width * 0.5f, Fade(SKYBLUE, 0.8f));
    }
}

void drawBall(const Body& b) {
    DrawCircleV(toRl(b.pos), b.radius, palette::kBall);
    DrawCircleV(toRl(b.pos), b.radius * 0.72f, ColorBrightness(palette::kBall, -0.15f));
    // A spoke makes rotation visible.
    const Vec2 spoke{std::cos(b.angle), std::sin(b.angle)};
    DrawLineEx(toRl(b.pos - spoke * (b.radius * 0.7f)), toRl(b.pos + spoke * (b.radius * 0.7f)),
               3.0f, ColorBrightness(palette::kBall, 0.3f));
    DrawCircleLinesV(toRl(b.pos), b.radius, ColorBrightness(palette::kBall, 0.25f));
}

}  // namespace

void drawCapsule(const Capsule& c, Color fill) {
    DrawLineEx(toRl(c.a), toRl(c.b), c.radius * 2.0f, fill);
    DrawCircleV(toRl(c.a), c.radius, fill);
    DrawCircleV(toRl(c.b), c.radius, fill);
}

void drawBackdrop(const Session& session, float time) {
    const Level& level = session.level();
    drawGrid();
    for (const ZoneDef& z : level.zones) drawZone(z, time);
    for (const Aabb& d : level.drains) {
        DrawRectangleRec(toRect(d), Fade(palette::kDrain, 0.18f));
        drawHatch(d, Fade(palette::kDrain, 0.35f), 12.0f);
    }
    for (const Aabb& n : level.noDraw) {
        DrawRectangleRec(toRect(n), Fade(palette::kNoDraw, 0.05f));
        drawHatch(n, Fade(palette::kNoDraw, 0.12f), 16.0f);
    }
    const float fill = static_cast<float>(session.inGoal()) / std::max(1, level.target);
    for (size_t i = 0; i < level.goals.size(); ++i) {
        drawGoal(session.goalArea(i), fill, session.holdProgress());
    }
}

void drawForeground(const Session& session, float time) {
    (void)time;
    const Level& level = session.level();
    // Everything solid-looking, including moving walls and fakes, which are
    // deliberately indistinguishable from the real thing.
    std::vector<Capsule> solid = level.walls;
    solid.insert(solid.end(), level.fakes.begin(), level.fakes.end());
    for (const Mover& m : session.world().movers) {
        solid.insert(solid.end(), m.capsules().begin(), m.capsules().end());
    }
    // Walls get a lighter outline pass first so joints read as one shape.
    for (const Capsule& c : solid) drawCapsule({c.a, c.b, c.radius + 1.5f}, palette::kWallEdge);
    for (const Capsule& c : solid) drawCapsule(c, palette::kWall);
    for (const Stroke& s : session.strokes()) {
        for (size_t i = 1; i < s.points.size(); ++i) {
            drawCapsule({s.points[i - 1], s.points[i], Session::kStrokeRadius}, palette::kStroke);
        }
    }
    for (const Emitter& e : session.emitters()) drawEmitter(e);
    for (const Body& b : session.world().bodies.items) drawBall(b);
}

}  // namespace spill
