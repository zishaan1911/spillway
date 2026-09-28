#include "raylib.h"

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 720, "Spillway");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground({18, 22, 30, 255});
        DrawText("Spillway", 40, 40, 40, RAYWHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
