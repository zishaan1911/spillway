#include "app/ui.hpp"

#include <algorithm>
#include <cmath>

#include "render/scene.hpp"

namespace spill::ui {

namespace {
Font gFont{};
bool gCustomFont = false;
constexpr float kPi = 3.14159265f;
}  // namespace

Viewport Viewport::fit(int windowW, int windowH, float virtualW, float virtualH) {
    Viewport v;
    v.scale = std::min(windowW / virtualW, windowH / virtualH);
    v.offset = {(windowW - virtualW * v.scale) * 0.5f, (windowH - virtualH * v.scale) * 0.5f};
    return v;
}

Vec2 Viewport::toVirtual(Vector2 screen) const {
    return {(screen.x - offset.x) / scale, (screen.y - offset.y) / scale};
}

void loadFont() {
    const char* candidates[] = {
        "C:/Windows/Fonts/segoeuib.ttf",
        "C:/Windows/Fonts/arialbd.ttf",
        "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
    };
    for (const char* path : candidates) {
        if (!FileExists(path)) continue;
        gFont = LoadFontEx(path, 64, nullptr, 0);
        if (gFont.texture.id != 0) {
            SetTextureFilter(gFont.texture, TEXTURE_FILTER_BILINEAR);
            gCustomFont = true;
            return;
        }
    }
    gFont = GetFontDefault();
}

void unloadFont() {
    if (gCustomFont) UnloadFont(gFont);
    gCustomFont = false;
}

static float spacingFor(float size) { return gCustomFont ? 0.0f : size / 10.0f; }

Vector2 measure(const char* t, float size) { return MeasureTextEx(gFont, t, size, spacingFor(size)); }

void text(const char* t, float x, float y, float size, Color color) {
    DrawTextEx(gFont, t, {x, y}, size, spacingFor(size), color);
}

void textCentered(const char* t, float cx, float y, float size, Color color) {
    text(t, cx - measure(t, size).x * 0.5f, y, size, color);
}

void textRight(const char* t, float right, float y, float size, Color color) {
    text(t, right - measure(t, size).x, y, size, color);
}

bool button(Rectangle r, const char* label, Vec2 mouse, bool clicked, bool enabled) {
    const bool hover = enabled && CheckCollisionPointRec({mouse.x, mouse.y}, r);
    const Color base = enabled ? Color{38, 48, 66, 255} : Color{28, 33, 44, 255};
    DrawRectangleRounded(r, 0.25f, 8, hover ? ColorBrightness(base, 0.25f) : base);
    DrawRectangleRoundedLinesEx(r, 0.25f, 8, 2.0f,
                                hover ? palette::kStroke : Fade(palette::kMuted, 0.5f));
    const float size = std::min(26.0f, r.height * 0.5f);
    textCentered(label, r.x + r.width * 0.5f, r.y + (r.height - size) * 0.5f, size,
                 enabled ? palette::kText : palette::kMuted);
    return hover && clicked;
}

void star(Vec2 c, float radius, bool filled, Color color) {
    Vector2 pts[10];
    for (int i = 0; i < 10; ++i) {
        const float a = -kPi / 2.0f + i * kPi / 5.0f;
        const float r = (i % 2 == 0) ? radius : radius * 0.45f;
        pts[i] = {c.x + std::cos(a) * r, c.y + std::sin(a) * r};
    }
    if (filled) {
        // Fan from the centre; raylib wants counter-clockwise triangles.
        for (int i = 0; i < 10; ++i) DrawTriangle({c.x, c.y}, pts[(i + 1) % 10], pts[i], color);
    } else {
        for (int i = 0; i < 10; ++i) DrawLineEx(pts[i], pts[(i + 1) % 10], 2.0f, color);
    }
}

void stars(Vec2 centre, float radius, int filled, int total, Color color) {
    const float gap = radius * 2.4f;
    const float x0 = centre.x - gap * (total - 1) * 0.5f;
    for (int i = 0; i < total; ++i) {
        star({x0 + gap * i, centre.y}, radius, i < filled, i < filled ? color : Fade(color, 0.45f));
    }
}

}  // namespace spill::ui
