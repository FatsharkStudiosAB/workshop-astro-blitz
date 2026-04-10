/*
 * Astro Blitz -- main entry point
 *
 * Initializes the window, runs the game loop, and cleans up.
 */

#include "raylib.h"
#include "game.h"

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Astro Blitz");
    SetTargetFPS(60);

    GameState gs;
    game_init(&gs);

    while (!WindowShouldClose()) {
        game_update(&gs);
        game_draw(&gs);
    }

    CloseWindow();
    return 0;
}
