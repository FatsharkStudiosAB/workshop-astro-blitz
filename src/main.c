/*
 * Astro Blitz -- main entry point
 *
 * Minimal "hello window" to verify Raylib build tooling works.
 */

#include "raylib.h"

int main(void)
{
    const int screenWidth  = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Astro Blitz");
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground(BLACK);
            DrawText("ASTRO BLITZ", screenWidth / 2 - MeasureText("ASTRO BLITZ", 40) / 2, screenHeight / 2 - 20, 40, RAYWHITE);
            DrawText("Press ESC to exit", screenWidth / 2 - MeasureText("Press ESC to exit", 20) / 2, screenHeight / 2 + 30, 20, GRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
