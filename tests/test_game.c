/*
 * test_game.c -- Unit tests for the game module
 *
 * Tests game_init (arena dimensions, player placement, bullet pool state)
 * and game_check_death (phase transitions on player death).
 * Does NOT test game_update/game_draw (require Raylib window context).
 */

#include "game.h"
#include "tilemap.h"
#include "unity.h"
#include <string.h>

/* ── Test helpers ──────────────────────────────────────────────────────────── */

static const float FLOAT_TOLERANCE = 0.001f;

/* ── setUp / tearDown ──────────────────────────────────────────────────────── */

void setUp(void) {}
void tearDown(void) {}

/* ── game_init tests ───────────────────────────────────────────────────────── */

void test_game_init_arena_position(void) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    game_init(&gs);

    /* Arena origin is at world origin (0,0) */
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 0.0f, gs.arena.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 0.0f, gs.arena.y);
}

void test_game_init_arena_dimensions(void) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    game_init(&gs);

    /* Arena matches full world bounds from tilemap */
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, (float)WORLD_WIDTH, gs.arena.width);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, (float)WORLD_HEIGHT, gs.arena.height);
}

void test_game_init_player_centered_in_arena(void) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    game_init(&gs);

    /* Player spawns at center of the center tile */
    float expected_x = (float)(WORLD_COLS / 2) * TILE_SIZE + TILE_SIZE / 2.0f;
    float expected_y = (float)(WORLD_ROWS / 2) * TILE_SIZE + TILE_SIZE / 2.0f;

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, expected_x, gs.player.position.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, expected_y, gs.player.position.y);
}

void test_game_init_player_hp_full(void) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    game_init(&gs);

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, gs.player.max_hp, gs.player.hp);
}

void test_game_init_player_not_dashing(void) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    game_init(&gs);

    TEST_ASSERT_FALSE(gs.player.is_dashing);
}

void test_game_init_bullet_pool_empty(void) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    game_init(&gs);

    for (int i = 0; i < MAX_BULLETS; i++) {
        TEST_ASSERT_FALSE(gs.bullets.bullets[i].active);
    }
}

void test_game_init_bullet_pool_cooldown_zero(void) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    game_init(&gs);

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 0.0f, gs.bullets.fire_cooldown);
}

/* ── Constants sanity checks ───────────────────────────────────────────────── */

void test_screen_dimensions_positive(void) {
    TEST_ASSERT_TRUE(SCREEN_WIDTH > 0);
    TEST_ASSERT_TRUE(SCREEN_HEIGHT > 0);
}

void test_world_larger_than_screen(void) {
    TEST_ASSERT_TRUE(WORLD_WIDTH > SCREEN_WIDTH);
    TEST_ASSERT_TRUE(WORLD_HEIGHT > SCREEN_HEIGHT);
}

/* ── Camera tests ──────────────────────────────────────────────────────────── */

void test_game_init_camera_offset(void) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    game_init(&gs);

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, SCREEN_WIDTH / 2.0f, gs.camera.offset.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, SCREEN_HEIGHT / 2.0f, gs.camera.offset.y);
}

void test_game_init_camera_zoom(void) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    game_init(&gs);

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 1.0f, gs.camera.zoom);
}

void test_game_init_camera_rotation(void) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    game_init(&gs);

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 0.0f, gs.camera.rotation);
}

/* ── Tilemap tests ─────────────────────────────────────────────────────────── */

void test_game_init_tilemap_dimensions(void) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    game_init(&gs);

    TEST_ASSERT_EQUAL_INT(WORLD_COLS, gs.tilemap.cols);
    TEST_ASSERT_EQUAL_INT(WORLD_ROWS, gs.tilemap.rows);
    TEST_ASSERT_EQUAL_INT(TILE_SIZE, gs.tilemap.tile_size);
}

void test_game_init_spawn_area_clear(void) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    game_init(&gs);

    /* The center tiles around spawn should be empty */
    int sx = WORLD_COLS / 2;
    int sy = WORLD_ROWS / 2;
    TEST_ASSERT_EQUAL_INT(TILE_EMPTY, gs.tilemap.tiles[sy][sx]);
}

/* ── Phase / game-over tests ───────────────────────────────────────────────── */

void test_game_init_phase_is_playing(void) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    game_init(&gs);

    TEST_ASSERT_EQUAL_INT(PHASE_PLAYING, gs.phase);
}

void test_game_init_stats_zeroed(void) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    game_init(&gs);

    TEST_ASSERT_EQUAL_INT(0, gs.stats.kills);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 0.0f, gs.stats.survival_time);
    TEST_ASSERT_EQUAL_INT(0, gs.stats.waves_spawned);
}

void test_game_init_resets_phase_after_game_over(void) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    game_init(&gs);

    /* Simulate a game-over state with some stats */
    gs.phase = PHASE_GAME_OVER;
    gs.stats.kills = 42;
    gs.stats.survival_time = 99.0f;
    gs.stats.waves_spawned = 10;

    /* Restart */
    game_init(&gs);

    TEST_ASSERT_EQUAL_INT(PHASE_PLAYING, gs.phase);
    TEST_ASSERT_EQUAL_INT(0, gs.stats.kills);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 0.0f, gs.stats.survival_time);
    TEST_ASSERT_EQUAL_INT(0, gs.stats.waves_spawned);
}

void test_phase_game_over_on_zero_hp(void) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    game_init(&gs);

    /* Player alive -- phase stays PLAYING */
    TEST_ASSERT_EQUAL_INT(PHASE_PLAYING, gs.phase);
    game_check_death(&gs);
    TEST_ASSERT_EQUAL_INT(PHASE_PLAYING, gs.phase);

    /* Set HP to zero and run the death check */
    gs.player.hp = 0.0f;
    game_check_death(&gs);
    TEST_ASSERT_EQUAL_INT(PHASE_GAME_OVER, gs.phase);
}

void test_game_check_death_ignores_game_over_phase(void) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    game_init(&gs);

    /* Already in GAME_OVER -- should not re-trigger */
    gs.phase = PHASE_GAME_OVER;
    gs.player.hp = 50.0f; /* alive but phase is already over */
    game_check_death(&gs);
    TEST_ASSERT_EQUAL_INT(PHASE_GAME_OVER, gs.phase);
}

void test_game_check_death_negative_hp(void) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    game_init(&gs);

    gs.player.hp = -5.0f;
    game_check_death(&gs);
    TEST_ASSERT_EQUAL_INT(PHASE_GAME_OVER, gs.phase);
}

/* ── New phase enum tests ──────────────────────────────────────────────────── */

void test_phase_enum_values_are_distinct(void) {
    TEST_ASSERT_NOT_EQUAL(PHASE_MAIN_MENU, PHASE_PLAYING);
    TEST_ASSERT_NOT_EQUAL(PHASE_PLAYING, PHASE_PAUSED);
    TEST_ASSERT_NOT_EQUAL(PHASE_PAUSED, PHASE_SETTINGS);
    TEST_ASSERT_NOT_EQUAL(PHASE_SETTINGS, PHASE_GAME_OVER);
    TEST_ASSERT_NOT_EQUAL(PHASE_MAIN_MENU, PHASE_GAME_OVER);
}

void test_game_init_preserves_settings(void) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    gs.settings.movement_layout = MOVEMENT_8DIR;
    game_init(&gs);

    /* Settings should be preserved across game_init */
    TEST_ASSERT_EQUAL_INT(MOVEMENT_8DIR, gs.settings.movement_layout);
}

void test_game_init_menu_cursor_zero(void) {
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    game_init(&gs);

    TEST_ASSERT_EQUAL_INT(0, gs.menu_cursor);
}

void test_game_check_death_only_during_playing(void) {
    /* Death check should NOT trigger in PAUSED phase */
    GameState gs;
    memset(&gs, 0, sizeof(gs));
    game_init(&gs);

    gs.phase = PHASE_PAUSED;
    gs.player.hp = 0.0f;
    game_check_death(&gs);
    TEST_ASSERT_EQUAL_INT(PHASE_PAUSED, gs.phase);
}

/* ── Runner ────────────────────────────────────────────────────────────────── */

int main(void) {
    UNITY_BEGIN();

    /* game_init */
    RUN_TEST(test_game_init_arena_position);
    RUN_TEST(test_game_init_arena_dimensions);
    RUN_TEST(test_game_init_player_centered_in_arena);
    RUN_TEST(test_game_init_player_hp_full);
    RUN_TEST(test_game_init_player_not_dashing);
    RUN_TEST(test_game_init_bullet_pool_empty);
    RUN_TEST(test_game_init_bullet_pool_cooldown_zero);

    /* Constants */
    RUN_TEST(test_screen_dimensions_positive);
    RUN_TEST(test_world_larger_than_screen);

    /* Camera */
    RUN_TEST(test_game_init_camera_offset);
    RUN_TEST(test_game_init_camera_zoom);
    RUN_TEST(test_game_init_camera_rotation);

    /* Tilemap */
    RUN_TEST(test_game_init_tilemap_dimensions);
    RUN_TEST(test_game_init_spawn_area_clear);

    /* Phase / game-over */
    RUN_TEST(test_game_init_phase_is_playing);
    RUN_TEST(test_game_init_stats_zeroed);
    RUN_TEST(test_game_init_resets_phase_after_game_over);
    RUN_TEST(test_phase_game_over_on_zero_hp);
    RUN_TEST(test_game_check_death_ignores_game_over_phase);
    RUN_TEST(test_game_check_death_negative_hp);

    /* New phases */
    RUN_TEST(test_phase_enum_values_are_distinct);
    RUN_TEST(test_game_init_preserves_settings);
    RUN_TEST(test_game_init_menu_cursor_zero);
    RUN_TEST(test_game_check_death_only_during_playing);

    return UNITY_END();
}
