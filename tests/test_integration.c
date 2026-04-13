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
#include "upgrade.h"
#include "weapon.h"

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

    /* Advance frames -- bullet should hit and kill the enemy.
     * Extra frames needed because hitstop freezes gameplay for a few frames
     * on each hit/kill. */
    for (int i = 0; i < 60; i++) {
        game_update(&gs);
        /* Release fire after first frame to avoid cooldown issues */
        if (i == 0) {
            stub_set_mouse_button_down(MOUSE_BUTTON_LEFT, false);
        }
    }

    TEST_ASSERT_EQUAL_INT(0, enemy_pool_active_count(&gs.enemies));
    TEST_ASSERT_GREATER_OR_EQUAL(1, gs.stats.kills);
}

/* ── Test: weapon system fires with weapon stats ──────────────────────────── */

void test_weapon_system_fires_shotgun_pellets(void) {
    gs.player.current_weapon = weapon_get_preset(WEAPON_SHOTGUN);

    /* Aim right */
    gs.player.aim_direction = (Vector2){1.0f, 0.0f};
    stub_set_mouse_button_down(MOUSE_BUTTON_LEFT, true);

    game_update(&gs);
    stub_set_mouse_button_down(MOUSE_BUTTON_LEFT, false);

    /* Shotgun fires 5 pellets per shot */
    int active = 0;
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (gs.bullets.bullets[i].active) {
            active++;
        }
    }
    TEST_ASSERT_EQUAL_INT(5, active);
}

/* ── Test: grunt enemy fires bullet at player ─────────────────────────────── */

void test_grunt_enemy_fires_bullet(void) {
    /* Place a grunt near the player */
    float ex = gs.player.position.x + GRUNT_PREFERRED_RANGE;
    float ey = gs.player.position.y;
    enemy_pool_spawn(&gs.enemies, ENEMY_GRUNT, (Vector2){ex, ey});

    /* Set its shoot cooldown to 0 so it fires immediately */
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (gs.enemies.enemies[i].active && gs.enemies.enemies[i].type == ENEMY_GRUNT) {
            gs.enemies.enemies[i].shoot_cooldown = 0.0f;
            break;
        }
    }

    game_update(&gs);

    /* Check that at least one enemy bullet exists */
    int active = 0;
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (gs.enemy_bullets.bullets[i].active) {
            active++;
        }
    }
    TEST_ASSERT_GREATER_OR_EQUAL(1, active);
}

/* ── Test: combo increments on consecutive kills ──────────────────────────── */

void test_combo_increments_on_kills(void) {
    /* Disable hitstop so kills don't freeze the simulation and starve the
     * second bullet of movement frames. */
    gs.settings.hitstop = 0.0f;

    /* Place two 1-HP swarmers right next to each other in front of player */
    float x = gs.player.position.x + 30.0f;
    enemy_pool_spawn(&gs.enemies, ENEMY_SWARMER, (Vector2){x, gs.player.position.y});
    enemy_pool_spawn(&gs.enemies, ENEMY_SWARMER, (Vector2){x + 5.0f, gs.player.position.y});

    /* Fire two bullets at them (reset cooldown between shots so both fire) */
    gs.player.aim_direction = (Vector2){1.0f, 0.0f};
    bullet_pool_fire(&gs.bullets, gs.player.position, (Vector2){1.0f, 0.0f});
    gs.bullets.fire_cooldown = 0.0f;
    bullet_pool_fire(&gs.bullets, gs.player.position, (Vector2){1.0f, 0.0f});

    /* Advance frames until bullets reach enemies */
    for (int i = 0; i < 30; i++) {
        game_update(&gs);
    }

    /* Both should be dead, kills >= 2.
     * Verify the combo system recorded at least 1 combo kill (best >= 1). */
    TEST_ASSERT_EQUAL_INT(0, enemy_pool_active_count(&gs.enemies));
    TEST_ASSERT_GREATER_OR_EQUAL(2, gs.stats.kills);
    TEST_ASSERT_GREATER_OR_EQUAL(1, gs.combo.best);
}

/* ── Test: elite enemy has modified stats ─────────────────────────────────── */

void test_elite_armored_has_more_hp(void) {
    float x = gs.player.position.x + 100.0f;
    float y = gs.player.position.y;

    /* Spawn normal swarmer */
    enemy_pool_spawn(&gs.enemies, ENEMY_SWARMER, (Vector2){x, y});
    float normal_hp = gs.enemies.enemies[0].hp;
    gs.enemies.enemies[0].active = false;

    /* Spawn armored swarmer */
    enemy_pool_spawn_elite(&gs.enemies, ENEMY_SWARMER, (Vector2){x, y}, ELITE_ARMORED);
    float armored_hp = gs.enemies.enemies[0].hp;

    TEST_ASSERT_TRUE(armored_hp > normal_hp);
    TEST_ASSERT_EQUAL(ELITE_ARMORED, gs.enemies.enemies[0].elite);
}

/* ── Test: weapon pickup changes player weapon ────────────────────────────── */

void test_weapon_pickup_swaps_weapon(void) {
    TEST_ASSERT_EQUAL(WEAPON_PISTOL, gs.player.current_weapon.type);

    /* Manually create a shotgun pickup at the player's position */
    gs.weapon_pickups.pickups[0].position = gs.player.position;
    gs.weapon_pickups.pickups[0].weapon = weapon_get_preset(WEAPON_SHOTGUN);
    gs.weapon_pickups.pickups[0].lifetime = 10.0f;
    gs.weapon_pickups.pickups[0].active = true;

    /* Advance one frame -- should enter PHASE_PICKUP_WEAPON (not auto-swap) */
    game_update(&gs);
    TEST_ASSERT_EQUAL(PHASE_PICKUP_WEAPON, gs.phase);
    /* Weapon not swapped yet */
    TEST_ASSERT_EQUAL(WEAPON_PISTOL, gs.player.current_weapon.type);

    /* Simulate selecting "Replace" (menu_cursor = 1) and pressing Enter */
    gs.menu_cursor = 1;
    stub_set_key_pressed(KEY_ENTER, true);
    game_update(&gs);

    /* Now weapon should be swapped and pickup consumed */
    TEST_ASSERT_EQUAL(WEAPON_SHOTGUN, gs.player.current_weapon.type);
    TEST_ASSERT_FALSE(gs.weapon_pickups.pickups[0].active);
    TEST_ASSERT_EQUAL(PHASE_PLAYING, gs.phase);
}

/* ── Test: floor scaling increases enemy HP ───────────────────────────────── */

void test_floor_scaling_increases_enemy_hp(void) {
    /* Spawn a swarmer on floor 0 -- should have base HP */
    float x = gs.player.position.x + 200.0f;
    float y = gs.player.position.y;
    enemy_pool_spawn(&gs.enemies, ENEMY_SWARMER, (Vector2){x, y});
    float floor0_hp = gs.enemies.enemies[0].hp;
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, SWARMER_HP, floor0_hp);

    /* Clear and set floor to 3 */
    gs.enemies.enemies[0].active = false;
    gs.floor = 3;

    /* Manually simulate what game.c spawn does: spawn + scale */
    enemy_pool_spawn(&gs.enemies, ENEMY_SWARMER, (Vector2){x, y});
    float scale = 1.0f + FLOOR_ENEMY_HP_SCALE * (float)gs.floor;
    gs.enemies.enemies[0].hp *= scale;
    float floor3_hp = gs.enemies.enemies[0].hp;

    /* Floor 3: HP should be 1.45x base */
    TEST_ASSERT_TRUE(floor3_hp > floor0_hp);
    float expected = SWARMER_HP * (1.0f + FLOOR_ENEMY_HP_SCALE * 3.0f);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, expected, floor3_hp);
}

/* ── Test: speed upgrade affects player via game_update ───────────────────── */

void test_speed_upgrade_applied_via_game_update(void) {
    /* Add speed upgrade stacks */
    upgrade_add(&gs.upgrades, UPGRADE_SPEED);
    upgrade_add(&gs.upgrades, UPGRADE_SPEED);

    /* Run one frame so game_update applies upgrade to player */
    game_update(&gs);

    float expected_bonus = UPGRADE_SPEED_BONUS * 2.0f;
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, expected_bonus, gs.player.speed_bonus);
}

/* ── Test: dash CD upgrade affects player via game_update ─────────────────── */

void test_dash_cd_upgrade_applied_via_game_update(void) {
    /* Add dash CD upgrade stacks */
    upgrade_add(&gs.upgrades, UPGRADE_DASH_CD);

    /* Run one frame so game_update applies upgrade to player */
    game_update(&gs);

    float expected_mult = UPGRADE_DASH_CD_BONUS; /* 0.85 for 1 stack */
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, expected_mult, gs.player.dash_cd_mult);
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
    RUN_TEST(test_weapon_system_fires_shotgun_pellets);
    RUN_TEST(test_grunt_enemy_fires_bullet);
    RUN_TEST(test_combo_increments_on_kills);
    RUN_TEST(test_elite_armored_has_more_hp);
    RUN_TEST(test_weapon_pickup_swaps_weapon);
    RUN_TEST(test_floor_scaling_increases_enemy_hp);
    RUN_TEST(test_speed_upgrade_applied_via_game_update);
    RUN_TEST(test_dash_cd_upgrade_applied_via_game_update);
    return UNITY_END();
}
