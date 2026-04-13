/*
 * Astro Blitz -- main entry point
 *
 * Pixel-perfect rendering pipeline:
 *   1. World renders at RENDER_WIDTH x RENDER_HEIGHT (400x300) into postfx target
 *   2. Lightmap composited at render resolution
 *   3. PostFX shader applied at render resolution
 *   4. Result upscaled 2x to SCREEN_WIDTH x SCREEN_HEIGHT (800x600) with nearest-neighbor
 *   5. UI drawn at full window resolution for crisp text
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

    /* Initialize rendering systems at internal render resolution */
    PostFX pfx;
    postfx_init(&pfx, RENDER_WIDTH, RENDER_HEIGHT);
    LightMap lm;
    lightmap_init(&lm, RENDER_WIDTH, RENDER_HEIGHT);

    /* Set nearest-neighbor filter once on postfx output textures */
    SetTextureFilter(pfx.output.texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(pfx.target.texture, TEXTURE_FILTER_POINT);

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
             gs.phase == PHASE_PICKUP_WEAPON || gs.phase == PHASE_PICKUP_UPGRADE ||
             (gs.phase == PHASE_SETTINGS && gs.settings_return_phase != PHASE_MAIN_MENU));

        if (has_world) {
            /* Push settings to postfx shader uniforms each frame */
            postfx_set_params(&pfx, gs.settings.bloom, gs.settings.scanlines,
                              gs.settings.chromatic_aberration, gs.settings.vignette);

            Camera2D draw_cam = gs.camera;
            draw_cam.target.x += gs.camera_kick.x;
            draw_cam.target.y += gs.camera_kick.y;

            /* Step 1: Build lightmap to its own texture (separate render target) */
            populate_lights(&lm, &gs);
            lightmap_build_scaled(&lm, draw_cam, gs.settings.lighting);

            /* Step 2: Render world into postfx target */
            postfx_begin(&pfx);
            game_draw_world(&gs);

            /* Step 3: Composite lightmap over world (just a textured quad,
             * no nested BeginTextureMode) */
            if (gs.settings.lighting > 0.0f) {
                lightmap_composite(&lm);
            }

            /* Step 4: Apply postfx shader (result stored in output texture) */
            postfx_end(&pfx, elapsed);

            /* Step 5: Upscale to window resolution + draw UI */
            BeginDrawing();
            ClearBackground(BLACK);

            Texture2D world_tex = postfx_get_texture(&pfx);
            DrawTexturePro(world_tex,
                           (Rectangle){0, 0, (float)world_tex.width, -(float)world_tex.height},
                           (Rectangle){0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT},
                           (Vector2){0, 0}, 0.0f, WHITE);

            /* UI drawn at full 800x600 resolution for crisp text */
            game_draw_ui(&gs);
            EndDrawing();
        } else {
            /* Menu-only: no postfx, no lightmap -- clean text at full resolution */
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
