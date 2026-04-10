#include "raylib.h"

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Astro Blitz");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawText("Astro Blitz - Coming Soon", 220, 280, 20, RAYWHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
