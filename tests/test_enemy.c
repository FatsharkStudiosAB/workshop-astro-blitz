/*
 * test_enemy.c -- Unit tests for the enemy pool module
 *
 * Tests pool init, spawning, swarmer movement, collision detection,
 * active count, and constant sanity checks.
 */

#include "enemy.h"
#include "tilemap.h"
#include "unity.h"
#include <math.h>

/* ── Test helpers ──────────────────────────────────────────────────────────── */

static const Rectangle TEST_ARENA = {0.0f, 0.0f, (float)WORLD_WIDTH, (float)WORLD_HEIGHT};
static const float FLOAT_TOLERANCE = 0.001f;

/* ── setUp / tearDown ──────────────────────────────────────────────────────── */

void setUp(void) {}
void tearDown(void) {}

/* ── enemy_pool_init tests ─────────────────────────────────────────────────── */

void test_pool_init_all_enemies_inactive(void) {
    EnemyPool pool;
    enemy_pool_init(&pool);

    for (int i = 0; i < MAX_ENEMIES; i++) {
        TEST_ASSERT_FALSE(pool.enemies[i].active);
    }
}

void test_pool_init_active_count_zero(void) {
    EnemyPool pool;
    enemy_pool_init(&pool);

    TEST_ASSERT_EQUAL_INT(0, enemy_pool_active_count(&pool));
}

/* ── enemy_pool_spawn tests ────────────────────────────────────────────────── */

void test_spawn_activates_one_enemy(void) {
    EnemyPool pool;
    enemy_pool_init(&pool);

    enemy_pool_spawn(&pool, ENEMY_SWARMER, (Vector2){100.0f, 200.0f});

    TEST_ASSERT_EQUAL_INT(1, enemy_pool_active_count(&pool));
}

void test_spawn_sets_position(void) {
    EnemyPool pool;
    enemy_pool_init(&pool);

    enemy_pool_spawn(&pool, ENEMY_SWARMER, (Vector2){150.0f, 250.0f});

    TEST_ASSERT_TRUE(pool.enemies[0].active);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 150.0f, pool.enemies[0].position.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 250.0f, pool.enemies[0].position.y);
}

void test_spawn_swarmer_sets_hp(void) {
    EnemyPool pool;
    enemy_pool_init(&pool);

    enemy_pool_spawn(&pool, ENEMY_SWARMER, (Vector2){0.0f, 0.0f});

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, SWARMER_HP, pool.enemies[0].hp);
}

void test_spawn_swarmer_sets_radius(void) {
    EnemyPool pool;
    enemy_pool_init(&pool);

    enemy_pool_spawn(&pool, ENEMY_SWARMER, (Vector2){0.0f, 0.0f});

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, SWARMER_RADIUS, pool.enemies[0].radius);
}

void test_spawn_swarmer_sets_speed(void) {
    EnemyPool pool;
    enemy_pool_init(&pool);

    enemy_pool_spawn(&pool, ENEMY_SWARMER, (Vector2){0.0f, 0.0f});

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, SWARMER_SPEED, pool.enemies[0].speed);
}

void test_spawn_swarmer_sets_damage(void) {
    EnemyPool pool;
    enemy_pool_init(&pool);

    enemy_pool_spawn(&pool, ENEMY_SWARMER, (Vector2){0.0f, 0.0f});

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, SWARMER_DAMAGE, pool.enemies[0].damage);
}

void test_spawn_swarmer_sets_type(void) {
    EnemyPool pool;
    enemy_pool_init(&pool);

    enemy_pool_spawn(&pool, ENEMY_SWARMER, (Vector2){0.0f, 0.0f});

    TEST_ASSERT_EQUAL_INT(ENEMY_SWARMER, pool.enemies[0].type);
}

void test_spawn_multiple_uses_next_slot(void) {
    EnemyPool pool;
    enemy_pool_init(&pool);

    enemy_pool_spawn(&pool, ENEMY_SWARMER, (Vector2){10.0f, 20.0f});
    enemy_pool_spawn(&pool, ENEMY_SWARMER, (Vector2){30.0f, 40.0f});

    TEST_ASSERT_EQUAL_INT(2, enemy_pool_active_count(&pool));
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 10.0f, pool.enemies[0].position.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 30.0f, pool.enemies[1].position.x);
}

void test_spawn_pool_full_silently_drops(void) {
    EnemyPool pool;
    enemy_pool_init(&pool);

    /* Fill all slots */
    for (int i = 0; i < MAX_ENEMIES; i++) {
        pool.enemies[i].active = true;
    }

    /* Should not crash */
    enemy_pool_spawn(&pool, ENEMY_SWARMER, (Vector2){0.0f, 0.0f});

    TEST_ASSERT_EQUAL_INT(MAX_ENEMIES, enemy_pool_active_count(&pool));
}

/* ── enemy_pool_update tests ───────────────────────────────────────────────── */

void test_update_swarmer_moves_toward_target(void) {
    EnemyPool pool;
    enemy_pool_init(&pool);

    /* Place enemy at (100, 300), target at (500, 300) -- should move right */
    enemy_pool_spawn(&pool, ENEMY_SWARMER, (Vector2){100.0f, 300.0f});

    Vector2 target = {500.0f, 300.0f};
    float dt = 0.1f;
    enemy_pool_update(&pool, dt, target, TEST_ARENA, NULL);

    float expected_x = 100.0f + SWARMER_SPEED * dt;
    TEST_ASSERT_FLOAT_WITHIN(0.5f, expected_x, pool.enemies[0].position.x);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 300.0f, pool.enemies[0].position.y);
}

void test_update_swarmer_moves_toward_target_diagonal(void) {
    EnemyPool pool;
    enemy_pool_init(&pool);

    /* Place enemy at origin, target diagonally away */
    enemy_pool_spawn(&pool, ENEMY_SWARMER, (Vector2){100.0f, 100.0f});

    Vector2 target = {200.0f, 200.0f};
    float dt = 0.1f;
    enemy_pool_update(&pool, dt, target, TEST_ARENA, NULL);

    /* Should have moved closer to target */
    float old_dist = 141.42f; /* sqrt(100^2 + 100^2) */
    Vector2 new_pos = pool.enemies[0].position;
    float dx = target.x - new_pos.x;
    float dy = target.y - new_pos.y;
    float new_dist = sqrtf(dx * dx + dy * dy);

    TEST_ASSERT_TRUE(new_dist < old_dist);
}

void test_update_skips_inactive_enemies(void) {
    EnemyPool pool;
    enemy_pool_init(&pool);

    /* All inactive -- update should not crash */
    Vector2 target = {400.0f, 300.0f};
    enemy_pool_update(&pool, 0.016f, target, TEST_ARENA, NULL);

    TEST_ASSERT_EQUAL_INT(0, enemy_pool_active_count(&pool));
}

void test_update_clamps_enemy_to_arena(void) {
    EnemyPool pool;
    enemy_pool_init(&pool);

    /* Place enemy near left edge, target far to the left (outside arena) */
    enemy_pool_spawn(&pool, ENEMY_SWARMER, (Vector2){SWARMER_RADIUS + 1.0f, 300.0f});

    Vector2 target = {-1000.0f, 300.0f};
    enemy_pool_update(&pool, 1.0f, target, TEST_ARENA, NULL);

    /* Should be clamped at arena left edge + radius */
    TEST_ASSERT_TRUE(pool.enemies[0].position.x >= SWARMER_RADIUS);
}

void test_update_enemy_stops_at_target(void) {
    EnemyPool pool;
    enemy_pool_init(&pool);

    /* Place enemy essentially on top of target */
    enemy_pool_spawn(&pool, ENEMY_SWARMER, (Vector2){400.0f, 300.0f});

    Vector2 target = {400.0f, 300.0f};
    enemy_pool_update(&pool, 0.016f, target, TEST_ARENA, NULL);

    /* Velocity should be zero when at target */
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 400.0f, pool.enemies[0].position.x);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 300.0f, pool.enemies[0].position.y);
}

/* ── Collision tests ───────────────────────────────────────────────────────── */

void test_circle_collision_overlapping(void) {
    Vector2 a = {100.0f, 100.0f};
    Vector2 b = {105.0f, 100.0f};
    /* Distance 5, combined radii 10+10 = 20. Should collide. */
    TEST_ASSERT_TRUE(check_circle_collision(a, 10.0f, b, 10.0f));
}

void test_circle_collision_not_overlapping(void) {
    Vector2 a = {100.0f, 100.0f};
    Vector2 b = {200.0f, 100.0f};
    /* Distance 100, combined radii 10+10 = 20. Should not collide. */
    TEST_ASSERT_FALSE(check_circle_collision(a, 10.0f, b, 10.0f));
}

void test_circle_collision_touching(void) {
    Vector2 a = {100.0f, 100.0f};
    Vector2 b = {120.0f, 100.0f};
    /* Distance 20, combined radii 10+10 = 20. Exactly touching = collide. */
    TEST_ASSERT_TRUE(check_circle_collision(a, 10.0f, b, 10.0f));
}

void test_circle_collision_same_position(void) {
    Vector2 a = {100.0f, 100.0f};
    /* Distance 0, any positive radii should collide. */
    TEST_ASSERT_TRUE(check_circle_collision(a, 5.0f, a, 5.0f));
}

void test_circle_collision_different_radii(void) {
    Vector2 a = {100.0f, 100.0f};
    Vector2 b = {110.0f, 100.0f};
    /* Distance 10, combined radii 3+8 = 11. Should collide (10 <= 11). */
    TEST_ASSERT_TRUE(check_circle_collision(a, 3.0f, b, 8.0f));
}

void test_circle_collision_different_radii_no_overlap(void) {
    Vector2 a = {100.0f, 100.0f};
    Vector2 b = {110.0f, 100.0f};
    /* Distance 10, combined radii 3+5 = 8. Should not collide (10 > 8). */
    TEST_ASSERT_FALSE(check_circle_collision(a, 3.0f, b, 5.0f));
}

/* ── active_count tests ────────────────────────────────────────────────────── */

void test_active_count_empty_pool(void) {
    EnemyPool pool;
    enemy_pool_init(&pool);

    TEST_ASSERT_EQUAL_INT(0, enemy_pool_active_count(&pool));
}

void test_active_count_after_spawns(void) {
    EnemyPool pool;
    enemy_pool_init(&pool);

    enemy_pool_spawn(&pool, ENEMY_SWARMER, (Vector2){0, 0});
    enemy_pool_spawn(&pool, ENEMY_SWARMER, (Vector2){10, 10});
    enemy_pool_spawn(&pool, ENEMY_SWARMER, (Vector2){20, 20});

    TEST_ASSERT_EQUAL_INT(3, enemy_pool_active_count(&pool));
}

void test_active_count_after_deactivation(void) {
    EnemyPool pool;
    enemy_pool_init(&pool);

    enemy_pool_spawn(&pool, ENEMY_SWARMER, (Vector2){0, 0});
    enemy_pool_spawn(&pool, ENEMY_SWARMER, (Vector2){10, 10});

    pool.enemies[0].active = false;

    TEST_ASSERT_EQUAL_INT(1, enemy_pool_active_count(&pool));
}

/* ── Constants sanity checks ───────────────────────────────────────────────── */

void test_max_enemies_is_positive(void) {
    TEST_ASSERT_TRUE(MAX_ENEMIES > 0);
}

void test_swarmer_speed_is_positive(void) {
    TEST_ASSERT_TRUE(SWARMER_SPEED > 0.0f);
}

void test_swarmer_hp_is_positive(void) {
    TEST_ASSERT_TRUE(SWARMER_HP > 0.0f);
}

void test_swarmer_radius_is_positive(void) {
    TEST_ASSERT_TRUE(SWARMER_RADIUS > 0.0f);
}

void test_swarmer_damage_is_positive(void) {
    TEST_ASSERT_TRUE(SWARMER_DAMAGE > 0.0f);
}

void test_spawn_interval_is_positive(void) {
    TEST_ASSERT_TRUE(SPAWN_INTERVAL > 0.0f);
}

void test_spawn_group_range_valid(void) {
    TEST_ASSERT_TRUE(SPAWN_MIN_GROUP > 0);
    TEST_ASSERT_TRUE(SPAWN_MAX_GROUP >= SPAWN_MIN_GROUP);
}

/* ── Runner ────────────────────────────────────────────────────────────────── */

int main(void) {
    UNITY_BEGIN();

    /* Init */
    RUN_TEST(test_pool_init_all_enemies_inactive);
    RUN_TEST(test_pool_init_active_count_zero);

    /* Spawn */
    RUN_TEST(test_spawn_activates_one_enemy);
    RUN_TEST(test_spawn_sets_position);
    RUN_TEST(test_spawn_swarmer_sets_hp);
    RUN_TEST(test_spawn_swarmer_sets_radius);
    RUN_TEST(test_spawn_swarmer_sets_speed);
    RUN_TEST(test_spawn_swarmer_sets_damage);
    RUN_TEST(test_spawn_swarmer_sets_type);
    RUN_TEST(test_spawn_multiple_uses_next_slot);
    RUN_TEST(test_spawn_pool_full_silently_drops);

    /* Update */
    RUN_TEST(test_update_swarmer_moves_toward_target);
    RUN_TEST(test_update_swarmer_moves_toward_target_diagonal);
    RUN_TEST(test_update_skips_inactive_enemies);
    RUN_TEST(test_update_clamps_enemy_to_arena);
    RUN_TEST(test_update_enemy_stops_at_target);

    /* Collision */
    RUN_TEST(test_circle_collision_overlapping);
    RUN_TEST(test_circle_collision_not_overlapping);
    RUN_TEST(test_circle_collision_touching);
    RUN_TEST(test_circle_collision_same_position);
    RUN_TEST(test_circle_collision_different_radii);
    RUN_TEST(test_circle_collision_different_radii_no_overlap);

    /* Active count */
    RUN_TEST(test_active_count_empty_pool);
    RUN_TEST(test_active_count_after_spawns);
    RUN_TEST(test_active_count_after_deactivation);

    /* Constants */
    RUN_TEST(test_max_enemies_is_positive);
    RUN_TEST(test_swarmer_speed_is_positive);
    RUN_TEST(test_swarmer_hp_is_positive);
    RUN_TEST(test_swarmer_radius_is_positive);
    RUN_TEST(test_swarmer_damage_is_positive);
    RUN_TEST(test_spawn_interval_is_positive);
    RUN_TEST(test_spawn_group_range_valid);

    return UNITY_END();
}
