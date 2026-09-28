#pragma once

#include "physics/vec2.hpp"
#include "raylib.h"

namespace spill::ui {

// The game renders at a fixed virtual resolution and is scaled to fit the
// window with letterboxing.
struct Viewport {
    float scale = 1.0f;
    Vector2 offset{0, 0};

    static Viewport fit(int windowW, int windowH, float virtualW, float virtualH);
    Vec2 toVirtual(Vector2 screen) const;
};

// Tries a few common system fonts and falls back to raylib's built-in one.
void loadFont();
void unloadFont();

Vector2 measure(const char* text, float size);
void text(const char* text, float x, float y, float size, Color color);
void textCentered(const char* text, float cx, float y, float size, Color color);
void textRight(const char* text, float right, float y, float size, Color color);

// Immediate-mode button. Returns true on the frame it is clicked.
bool button(Rectangle r, const char* label, Vec2 mouse, bool clicked, bool enabled = true);

// Five-pointed star, filled or outlined.
void star(Vec2 centre, float radius, bool filled, Color color);
void stars(Vec2 centre, float radius, int filled, int total, Color color);

}  // namespace spill::ui
