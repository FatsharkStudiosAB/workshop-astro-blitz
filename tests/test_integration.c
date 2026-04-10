/*
 * test_integration.c -- Integration tests for cross-module game behavior
 *
 * These tests exercise game_update() and multi-module interactions using
 * Raylib stubs (no window or audio device required).  Each test inits a
 * full GameState, configures stub inputs, advances one or more frames,
 * and asserts on the resulting game state.
 */

#include "unity.h"

#include "bullet.h"
#include "enemy.h"
#include "game.h"
#include "player.h"
#include "raylib_stubs.h"

#include <math.h>

/* ── Helpers ───────────────────────────────────────────────────────────────── */

#define FLOAT_EPSILON 1e-3f

static GameState gs;

/* Run N frames of game_update with the current stub settings. */
static void advance_frames(int n) {
    for (int i = 0; i < n; i++) {
        game_update(&gs);
    }
}

/* ── Unity hooks ──────────────────────────────────────────────────────────── */

void setUp(void) {
    stub_reset();
    /* Point the mouse far away so aim_direction doesn't cause issues */
    stub_set_mouse_position(400.0f, 0.0f);
    game_init(&gs);
}

void tearDown(void) {}

/* ── Test: bullet kills enemy ─────────────────────────────────────────────── */

void test_bullet_kills_enemy(void) {
    /* Place an enemy directly to the right of the player */
    float enemy_x = gs.player.position.x + 50.0f;
    float enemy_y = gs.player.position.y;
    enemy_pool_spawn(&gs.enemies, ENEMY_SWARMER, (Vector2){enemy_x, enemy_y});

    TEST_ASSERT_EQUAL_INT(1, enemy_pool_active_count(&gs.enemies));

    /* Fire a bullet from the player toward the enemy */
    Vector2 direction = {1.0f, 0.0f};
    bool fired = bullet_pool_fire(&gs.bullets, gs.player.position, direction);
    TEST_ASSERT_TRUE(fired);

    /* Advance enough frames for the bullet to reach the enemy.
     * Distance = 50, speed = 500, dt = 1/60 => frames ~ 50/(500/60) = 6 */
    stub_set_frame_time(1.0f / 60.0f);
    for (int i = 0; i < 20; i++) {
        /* Don't hold fire while advancing or we'll spawn more bullets */
        stub_set_mouse_button_down(MOUSE_BUTTON_LEFT, false);
        game_update(&gs);
    }

    /* Enemy should be dead (swarmer has 1 HP) */
    TEST_ASSERT_EQUAL_INT(0, enemy_pool_active_count(&gs.enemies));
    TEST_ASSERT_GREATER_OR_EQUAL(1, gs.stats.kills);
}

/* ── Test: enemy damages player on contact ────────────────────────────────── */

void test_enemy_damages_player(void) {
    float initial_hp = gs.player.hp;

    /* Spawn an enemy directly on top of the player */
    enemy_pool_spawn(&gs.enemies, ENEMY_SWARMER,
                     (Vector2){gs.player.position.x, gs.player.position.y});

    /* One frame is enough for collision resolution */
    stub_set_frame_time(1.0f / 60.0f);
    advance_frames(1);

    /* Player should have taken SWARMER_DAMAGE */
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, initial_hp - SWARMER_DAMAGE, gs.player.hp);

    /* Swarmer should be dead (dies on contact) */
    TEST_ASSERT_EQUAL_INT(0, enemy_pool_active_count(&gs.enemies));
}

/* ── Test: dash grants invincibility ──────────────────────────────────────── */

void test_dash_invincibility(void) {
    float initial_hp = gs.player.hp;

    /* Put the player into dash state manually */
    gs.player.is_dashing = true;
    gs.player.dash_direction = (Vector2){1.0f, 0.0f};
    gs.player.dash_timer = DASH_DURATION;
    gs.player.dash_cooldown = DASH_COOLDOWN;

    /* Spawn an enemy on top of the player */
    enemy_pool_spawn(&gs.enemies, ENEMY_SWARMER,
                     (Vector2){gs.player.position.x, gs.player.position.y});

    /* Advance one frame -- player is dashing, so should take no damage */
    stub_set_frame_time(1.0f / 60.0f);
    advance_frames(1);

    if (DASH_INVINCIBLE) {
        TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, initial_hp, gs.player.hp);
    }
}

/* ── Test: game over on player death ──────────────────────────────────────── */

void test_game_over_on_death(void) {
    TEST_ASSERT_EQUAL(PHASE_PLAYING, gs.phase);

    /* Kill the player */
    gs.player.hp = 0.0f;

    stub_set_frame_time(1.0f / 60.0f);
    advance_frames(1);

    TEST_ASSERT_EQUAL(PHASE_GAME_OVER, gs.phase);
}

/* ── Test: restart from game over ─────────────────────────────────────────── */

void test_restart_from_game_over(void) {
    /* Transition to game over */
    gs.player.hp = 0.0f;
    stub_set_frame_time(1.0f / 60.0f);
    advance_frames(1);
    TEST_ASSERT_EQUAL(PHASE_GAME_OVER, gs.phase);

    /* Accumulate some stats to verify they reset */
    gs.stats.kills = 42;

    /* Press R to restart */
    stub_set_key_pressed(KEY_R, true);
    advance_frames(1);

    TEST_ASSERT_EQUAL(PHASE_PLAYING, gs.phase);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, PLAYER_MAX_HP, gs.player.hp);
    TEST_ASSERT_EQUAL_INT(0, gs.stats.kills);
}

/* ── Test: player movement with WASD via game_update ──────────────────────── */

void test_player_moves_with_input(void) {
    Vector2 start_pos = gs.player.position;

    /* Hold W (forward) -- player should move toward aim direction */
    stub_set_key_down(KEY_W, true);
    stub_set_frame_time(1.0f / 60.0f);
    advance_frames(10);

    /* Player should have moved from start position */
    float dx = gs.player.position.x - start_pos.x;
    float dy = gs.player.position.y - start_pos.y;
    float dist = sqrtf(dx * dx + dy * dy);
    TEST_ASSERT_GREATER_THAN(0.1f, dist);
}

/* ── Test: enemy wave spawns after timer expires ──────────────────────────── */

void test_enemy_wave_spawns(void) {
    TEST_ASSERT_EQUAL_INT(0, enemy_pool_active_count(&gs.enemies));
    TEST_ASSERT_EQUAL_INT(0, gs.stats.waves_spawned);

    /* Use random_value = 0 so spawns land at the "top" edge (case 0)
     * with coordinates clamped to the valid spawn bounds. */
    stub_set_random_value(0);

    /* Fast-forward past the spawn timer.
     * spawn_timer starts at SPAWN_INTERVAL (3.0s). */
    stub_set_frame_time(0.5f);
    advance_frames(7); /* 3.5 seconds > SPAWN_INTERVAL */

    /* The wave counter must have incremented regardless of whether
     * individual spawn positions were walkable. */
    TEST_ASSERT_GREATER_OR_EQUAL(1, gs.stats.waves_spawned);

    /* With top-edge spawning and clamped coordinates, at least some
     * enemies should have found walkable positions. */
    TEST_ASSERT_GREATER_THAN(0, enemy_pool_active_count(&gs.enemies));
}

/* ── Test: survival time accumulates ──────────────────────────────────────── */

void test_survival_time_accumulates(void) {
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.0f, gs.stats.survival_time);

    stub_set_frame_time(0.1f);
    advance_frames(10); /* 1.0 second */

    TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.0f, gs.stats.survival_time);
}

/* ── Test: multiple enemies damage player sequentially ────────────────────── */

void test_multiple_enemies_damage_player(void) {
    float initial_hp = gs.player.hp;

    /* Spawn 3 enemies directly on the player */
    for (int i = 0; i < 3; i++) {
        enemy_pool_spawn(&gs.enemies, ENEMY_SWARMER,
                         (Vector2){gs.player.position.x, gs.player.position.y});
    }
    TEST_ASSERT_EQUAL_INT(3, enemy_pool_active_count(&gs.enemies));

    stub_set_frame_time(1.0f / 60.0f);
    advance_frames(1);

    /* All 3 should deal damage on contact and die */
    float expected_hp = initial_hp - 3.0f * SWARMER_DAMAGE;
    if (expected_hp < 0.0f) {
        expected_hp = 0.0f;
    }
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, expected_hp, gs.player.hp);
    TEST_ASSERT_EQUAL_INT(0, enemy_pool_active_count(&gs.enemies));
}

/* ── Test: shooting via mouse button in game_update ───────────────────────── */

void test_shooting_via_game_update(void) {
    /* Verify no active bullets initially */
    int active_before = 0;
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (gs.bullets.bullets[i].active) {
            active_before++;
        }
    }
    TEST_ASSERT_EQUAL_INT(0, active_before);

    /* Hold left mouse button and advance one frame */
    stub_set_mouse_button_down(MOUSE_BUTTON_LEFT, true);
    stub_set_frame_time(1.0f / 60.0f);
    advance_frames(1);

    /* At least one bullet should be active */
    int active_after = 0;
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (gs.bullets.bullets[i].active) {
            active_after++;
        }
    }
    TEST_ASSERT_GREATER_THAN(0, active_after);
}

/* ── Test: full combat loop -- spawn, shoot, kill ─────────────────────────── */

void test_full_combat_loop(void) {
    /* Place enemy to the right */
    float enemy_x = gs.player.position.x + 40.0f;
    enemy_pool_spawn(&gs.enemies, ENEMY_SWARMER, (Vector2){enemy_x, gs.player.position.y});

    /* Aim right: set mouse position to the right of the player in screen
     * space.  Camera offset is (400, 300) and zoom is 1.0, so mouse at
     * (600, 300) maps to player.x + 200 in world space. */
    stub_set_mouse_position(600.0f, 300.0f);

    /* Hold fire */
    stub_set_mouse_button_down(MOUSE_BUTTON_LEFT, true);
    stub_set_frame_time(1.0f / 60.0f);

    /* Advance frames -- bullet should hit and kill the enemy */
    for (int i = 0; i < 30; i++) {
        game_update(&gs);
        /* Release fire after first frame to avoid cooldown issues */
        if (i == 0) {
            stub_set_mouse_button_down(MOUSE_BUTTON_LEFT, false);
        }
    }

    TEST_ASSERT_EQUAL_INT(0, enemy_pool_active_count(&gs.enemies));
    TEST_ASSERT_GREATER_OR_EQUAL(1, gs.stats.kills);
}

/* ── Main ─────────────────────────────────────────────────────────────────── */

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_bullet_kills_enemy);
    RUN_TEST(test_enemy_damages_player);
    RUN_TEST(test_dash_invincibility);
    RUN_TEST(test_game_over_on_death);
    RUN_TEST(test_restart_from_game_over);
    RUN_TEST(test_player_moves_with_input);
    RUN_TEST(test_enemy_wave_spawns);
    RUN_TEST(test_survival_time_accumulates);
    RUN_TEST(test_multiple_enemies_damage_player);
    RUN_TEST(test_shooting_via_game_update);
    RUN_TEST(test_full_combat_loop);
    return UNITY_END();
}
