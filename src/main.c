/*
 * Astro Blitz -- main entry point
 *
 * Initializes the window and audio device, runs the game loop, and cleans up.
 * Post-processing effects (bloom, CRT, vignette) wrap the entire render.
 * Multiplicative light map adds atmospheric 2D point lighting.
 */

#include "audio.h"
#include "bullet.h"
#include "enemy.h"
#include "game.h"
#include "lightmap.h"
#include "player.h"
#include "postfx.h"
#include "raylib.h"
#include "settings.h"
#include <string.h>

/* Populate the light map with all current light sources from the game state. */
static void populate_lights(LightMap *lm, const GameState *gs) {
    lightmap_clear(lm);

    /* Only add lights during gameplay phases */
    if (gs->phase != PHASE_PLAYING && gs->phase != PHASE_PAUSED && gs->phase != PHASE_GAME_OVER) {
        return;
    }

    /* Player: warm white glow */
    lightmap_add(lm, gs->player.position, (Color){200, 220, 255, 200}, 120.0f);

    /* Active player bullets: weapon-colored glow */
    for (int i = 0; i < MAX_BULLETS; i++) {
        const Bullet *b = &gs->bullets.bullets[i];
        if (b->active) {
            Color lc = b->color;
            lc.a = 140;
            lightmap_add(lm, b->position, lc, 35.0f);
        }
    }

    /* Active enemy bullets: red glow */
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        const EnemyBullet *eb = &gs->enemy_bullets.bullets[i];
        if (eb->active) {
            lightmap_add(lm, eb->position, (Color){255, 80, 60, 120}, 30.0f);
        }
    }

    /* Enemies: dim colored glow per type */
    for (int i = 0; i < MAX_ENEMIES; i++) {
        const Enemy *e = &gs->enemies.enemies[i];
        if (!e->active) {
            continue;
        }
        Color ec;
        float er;
        switch (e->type) {
        case ENEMY_SWARMER:
            ec = (Color){255, 40, 20, 60};
            er = 25.0f;
            break;
        case ENEMY_GRUNT:
            ec = (Color){80, 80, 255, 60};
            er = 35.0f;
            break;
        case ENEMY_STALKER:
            ec = (Color){50, 255, 50, 50};
            er = 25.0f;
            break;
        case ENEMY_BOMBER:
            ec = e->is_charging ? (Color){255, 100, 20, 160} : (Color){255, 150, 30, 80};
            er = e->is_charging ? 60.0f : 35.0f;
            break;
        default:
            ec = (Color){255, 255, 255, 40};
            er = 20.0f;
            break;
        }
        lightmap_add(lm, e->position, ec, er);
    }

    /* Exit portal: green glow */
    if (gs->exit_active) {
        lightmap_add(lm, gs->exit_position, (Color){0, 255, 150, 180}, 80.0f);
    }
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Astro Blitz");
    InitAudioDevice();
    SetTargetFPS(60);

    /* Disable Raylib's default ESC-to-close so we can use ESC for menus */
    SetExitKey(0);

    /* Initialize rendering systems */
    PostFX pfx;
    postfx_init(&pfx, SCREEN_WIDTH, SCREEN_HEIGHT);
    LightMap lm;
    lightmap_init(&lm, SCREEN_WIDTH, SCREEN_HEIGHT);

    GameState gs;
    memset(&gs, 0, sizeof(gs));

    /* Load persistent settings before game_init (which preserves them) */
    bool has_settings = settings_init(&gs.settings);

    /* Initialize gameplay state but start at the main menu (or first-run picker) */
    audio_init(&gs.audio);
    game_init(&gs);
    gs.phase = has_settings ? PHASE_MAIN_MENU : PHASE_FIRST_RUN;

    float elapsed = 0.0f;

    while (!WindowShouldClose() && !game_should_quit(&gs)) {
        float dt = GetFrameTime();
        elapsed += dt;

        audio_update(&gs.audio);
        game_update(&gs);

        /* Toggle post-processing with F1 */
        if (IsKeyPressed(KEY_F1)) {
            postfx_toggle(&pfx);
        }

        /* Determine if we have world content (gameplay phases) */
        bool has_world =
            (gs.phase == PHASE_PLAYING || gs.phase == PHASE_PAUSED || gs.phase == PHASE_GAME_OVER ||
             (gs.phase == PHASE_SETTINGS && gs.settings_return_phase != PHASE_MAIN_MENU));

        if (has_world) {
            /* Push settings to postfx shader uniforms each frame */
            postfx_set_params(&pfx, gs.settings.bloom, gs.settings.scanlines,
                              gs.settings.chromatic_aberration, gs.settings.vignette);

            /* Full pipeline: postfx -> world -> lightmap -> UI -> postfx end */
            postfx_begin(&pfx);
            game_draw_world(&gs);

            /* Light map: populate and composite over the world layer */
            Camera2D draw_cam = gs.camera;
            draw_cam.target.x += gs.camera_kick.x;
            draw_cam.target.y += gs.camera_kick.y;
            populate_lights(&lm, &gs);
            lightmap_render_scaled(&lm, draw_cam, gs.settings.lighting);

            /* UI drawn after lightmap so it's not darkened */
            game_draw_ui(&gs);
            postfx_end(&pfx, elapsed);
        } else {
            /* Menu-only: no postfx, no lightmap -- clean text */
            BeginDrawing();
            ClearBackground(BLACK);
            game_draw_ui(&gs);
            EndDrawing();
        }
    }

    lightmap_cleanup(&lm);
    postfx_cleanup(&pfx);
    audio_cleanup(&gs.audio);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
