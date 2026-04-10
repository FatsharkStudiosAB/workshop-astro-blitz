/*
 * Astro Blitz -- main entry point
 *
 * Initializes the window and audio device, runs the game loop, and cleans up.
 */

#include "audio.h"
#include "game.h"
#include "raylib.h"
#include "settings.h"
#include <string.h>

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Astro Blitz");
    InitAudioDevice();
    SetTargetFPS(60);

    /* Disable Raylib's default ESC-to-close so we can use ESC for menus */
    SetExitKey(0);

    GameState gs;
    memset(&gs, 0, sizeof(gs));

    /* Load persistent settings before game_init (which preserves them) */
    bool has_settings = settings_init(&gs.settings);

    /* Initialize gameplay state but start at the main menu (or first-run picker) */
    audio_init(&gs.audio);
    game_init(&gs);
    gs.phase = has_settings ? PHASE_MAIN_MENU : PHASE_FIRST_RUN;

    while (!WindowShouldClose() && !game_should_quit(&gs)) {
        audio_update(&gs.audio);
        game_update(&gs);
        game_draw(&gs);
    }

    audio_cleanup(&gs.audio);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
