/*
 * Astro Blitz -- main entry point
 *
 * Initializes the window and audio device, runs the game loop, and cleans up.
 */

#include "audio.h"
#include "game.h"
#include "raylib.h"

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Astro Blitz");
    InitAudioDevice();
    SetTargetFPS(60);

    GameState gs;
    game_init(&gs);
    audio_init(&gs.audio);

    while (!WindowShouldClose()) {
        audio_update(&gs.audio);
        game_update(&gs);
        game_draw(&gs);
    }

    audio_cleanup(&gs.audio);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
