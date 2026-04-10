/*
 * test_bullet.c -- Unit tests for the bullet pool module
 *
 * Tests pool init, firing, rate limiting, update/movement, lifetime expiry,
 * arena bounds deactivation, and pool-full behavior.
 */

#include "bullet.h"
#include "tilemap.h"
#include "unity.h"
#include <string.h>

/* ── Test helpers ──────────────────────────────────────────────────────────── */

static const Rectangle TEST_ARENA = {0.0f, 0.0f, (float)WORLD_WIDTH, (float)WORLD_HEIGHT};
static const float FLOAT_TOLERANCE = 0.001f;

/* ── setUp / tearDown ──────────────────────────────────────────────────────── */

void setUp(void) {}
void tearDown(void) {}

/* ── bullet_pool_init tests ────────────────────────────────────────────────── */

void test_pool_init_all_bullets_inactive(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    for (int i = 0; i < MAX_BULLETS; i++) {
        TEST_ASSERT_FALSE(pool.bullets[i].active);
    }
}

void test_pool_init_fire_cooldown_zero(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 0.0f, pool.fire_cooldown);
}

/* ── bullet_pool_fire tests ────────────────────────────────────────────────── */

void test_fire_activates_one_bullet(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    Vector2 origin = {100.0f, 100.0f};
    Vector2 dir = {1.0f, 0.0f};
    bullet_pool_fire(&pool, origin, dir);

    int active_count = 0;
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (pool.bullets[i].active) {
            active_count++;
        }
    }
    TEST_ASSERT_EQUAL_INT(1, active_count);
}

void test_fire_sets_bullet_position(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    Vector2 origin = {150.0f, 250.0f};
    Vector2 dir = {0.0f, -1.0f};
    bullet_pool_fire(&pool, origin, dir);

    /* First bullet should be slot 0 */
    TEST_ASSERT_TRUE(pool.bullets[0].active);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 150.0f, pool.bullets[0].position.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 250.0f, pool.bullets[0].position.y);
}

void test_fire_sets_bullet_velocity(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    Vector2 origin = {0.0f, 0.0f};
    Vector2 dir = {1.0f, 0.0f};
    bullet_pool_fire(&pool, origin, dir);

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, BULLET_SPEED, pool.bullets[0].velocity.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 0.0f, pool.bullets[0].velocity.y);
}

void test_fire_sets_bullet_lifetime(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    bullet_pool_fire(&pool, (Vector2){0, 0}, (Vector2){1, 0});

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, BULLET_LIFETIME, pool.bullets[0].lifetime);
}

void test_fire_sets_cooldown(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    bullet_pool_fire(&pool, (Vector2){0, 0}, (Vector2){1, 0});

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, PISTOL_FIRE_RATE, pool.fire_cooldown);
}

void test_fire_rate_limited(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    Vector2 origin = {0.0f, 0.0f};
    Vector2 dir = {1.0f, 0.0f};

    /* First shot succeeds */
    bullet_pool_fire(&pool, origin, dir);
    TEST_ASSERT_TRUE(pool.bullets[0].active);

    /* Second shot immediately should be blocked (cooldown active) */
    bullet_pool_fire(&pool, origin, dir);

    int active_count = 0;
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (pool.bullets[i].active) {
            active_count++;
        }
    }
    TEST_ASSERT_EQUAL_INT(1, active_count);
}

void test_fire_after_cooldown_expires(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    Vector2 origin = {100.0f, 100.0f};
    Vector2 dir = {1.0f, 0.0f};

    bullet_pool_fire(&pool, origin, dir);
    TEST_ASSERT_EQUAL_INT(1, pool.bullets[0].active);

    /* Simulate cooldown expiring */
    float dt = PISTOL_FIRE_RATE + 0.01f;
    bullet_pool_update(&pool, dt, TEST_ARENA, NULL);

    /* Now should be able to fire again */
    bullet_pool_fire(&pool, origin, dir);

    int active_count = 0;
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (pool.bullets[i].active) {
            active_count++;
        }
    }
    TEST_ASSERT_EQUAL_INT(2, active_count);
}

void test_fire_pool_full_silently_drops(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    /* Fill every slot manually */
    for (int i = 0; i < MAX_BULLETS; i++) {
        pool.bullets[i].active = true;
        pool.bullets[i].position = (Vector2){100.0f, 100.0f};
        pool.bullets[i].velocity = (Vector2){100.0f, 0.0f};
        pool.bullets[i].lifetime = 1.0f;
    }
    pool.fire_cooldown = 0.0f;

    /* Attempting to fire should not crash and should return false */
    bool fired = bullet_pool_fire(&pool, (Vector2){0, 0}, (Vector2){1, 0});
    TEST_ASSERT_FALSE(fired);

    /* All bullets should still be the originals */
    for (int i = 0; i < MAX_BULLETS; i++) {
        TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 100.0f, pool.bullets[i].position.x);
    }
}

void test_fire_returns_true_on_success(void)
{
    BulletPool pool;
    bullet_pool_init(&pool);

    bool fired = bullet_pool_fire(&pool, (Vector2){100, 100}, (Vector2){1, 0});
    TEST_ASSERT_TRUE(fired);
}

void test_fire_returns_false_when_rate_limited(void)
{
    BulletPool pool;
    bullet_pool_init(&pool);

    bullet_pool_fire(&pool, (Vector2){0, 0}, (Vector2){1, 0});

    /* Second shot immediately should be blocked */
    bool fired = bullet_pool_fire(&pool, (Vector2){0, 0}, (Vector2){1, 0});
    TEST_ASSERT_FALSE(fired);
}

/* ── bullet_pool_update tests ──────────────────────────────────────────────── */

void test_update_moves_bullet(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    bullet_pool_fire(&pool, (Vector2){400.0f, 300.0f}, (Vector2){1.0f, 0.0f});

    float dt = 0.1f;
    bullet_pool_update(&pool, dt, TEST_ARENA, NULL);

    float expected_x = 400.0f + BULLET_SPEED * dt;
    TEST_ASSERT_FLOAT_WITHIN(0.1f, expected_x, pool.bullets[0].position.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 300.0f, pool.bullets[0].position.y);
}

void test_update_decreases_lifetime(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    bullet_pool_fire(&pool, (Vector2){400.0f, 300.0f}, (Vector2){0.0f, -1.0f});

    float dt = 0.5f;
    bullet_pool_update(&pool, dt, TEST_ARENA, NULL);

    float expected_lifetime = BULLET_LIFETIME - dt;
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, expected_lifetime, pool.bullets[0].lifetime);
}

void test_update_deactivates_expired_bullet(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    /* Place bullet in center so it won't leave arena */
    bullet_pool_fire(&pool, (Vector2){400.0f, 300.0f}, (Vector2){0.0f, 0.0f});

    /* Advance past lifetime */
    float dt = BULLET_LIFETIME + 0.1f;
    bullet_pool_update(&pool, dt, TEST_ARENA, NULL);

    TEST_ASSERT_FALSE(pool.bullets[0].active);
}

void test_update_deactivates_bullet_leaving_arena_right(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    /* Place bullet near right edge, moving right */
    pool.bullets[0].active = true;
    pool.bullets[0].position = (Vector2){4095.0f, 1500.0f};
    pool.bullets[0].velocity = (Vector2){BULLET_SPEED, 0.0f};
    pool.bullets[0].lifetime = 10.0f;

    bullet_pool_update(&pool, 0.1f, TEST_ARENA, NULL);

    TEST_ASSERT_FALSE(pool.bullets[0].active);
}

void test_update_deactivates_bullet_leaving_arena_left(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    pool.bullets[0].active = true;
    pool.bullets[0].position = (Vector2){1.0f, 1500.0f};
    pool.bullets[0].velocity = (Vector2){-BULLET_SPEED, 0.0f};
    pool.bullets[0].lifetime = 10.0f;

    bullet_pool_update(&pool, 0.1f, TEST_ARENA, NULL);

    TEST_ASSERT_FALSE(pool.bullets[0].active);
}

void test_update_deactivates_bullet_leaving_arena_top(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    pool.bullets[0].active = true;
    pool.bullets[0].position = (Vector2){2000.0f, 1.0f};
    pool.bullets[0].velocity = (Vector2){0.0f, -BULLET_SPEED};
    pool.bullets[0].lifetime = 10.0f;

    bullet_pool_update(&pool, 0.1f, TEST_ARENA, NULL);

    TEST_ASSERT_FALSE(pool.bullets[0].active);
}

void test_update_deactivates_bullet_leaving_arena_bottom(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    pool.bullets[0].active = true;
    pool.bullets[0].position = (Vector2){2000.0f, 3071.0f};
    pool.bullets[0].velocity = (Vector2){0.0f, BULLET_SPEED};
    pool.bullets[0].lifetime = 10.0f;

    bullet_pool_update(&pool, 0.1f, TEST_ARENA, NULL);

    TEST_ASSERT_FALSE(pool.bullets[0].active);
}

void test_update_keeps_bullet_inside_arena(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    /* Bullet in center, not moving fast enough to leave */
    pool.bullets[0].active = true;
    pool.bullets[0].position = (Vector2){400.0f, 300.0f};
    pool.bullets[0].velocity = (Vector2){10.0f, 0.0f};
    pool.bullets[0].lifetime = 10.0f;

    bullet_pool_update(&pool, 0.016f, TEST_ARENA, NULL);

    TEST_ASSERT_TRUE(pool.bullets[0].active);
}

void test_update_skips_inactive_bullets(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    /* Leave all inactive -- update should not crash */
    bullet_pool_update(&pool, 0.016f, TEST_ARENA, NULL);

    for (int i = 0; i < MAX_BULLETS; i++) {
        TEST_ASSERT_FALSE(pool.bullets[i].active);
    }
}

void test_update_ticks_fire_cooldown(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    pool.fire_cooldown = 0.5f;
    bullet_pool_update(&pool, 0.3f, TEST_ARENA, NULL);

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 0.2f, pool.fire_cooldown);
}

void test_update_clamps_fire_cooldown_to_zero(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    pool.fire_cooldown = 0.1f;
    bullet_pool_update(&pool, 0.5f, TEST_ARENA, NULL);

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 0.0f, pool.fire_cooldown);
}

/* ── Constants sanity checks ───────────────────────────────────────────────── */

void test_max_bullets_is_positive(void) {
    TEST_ASSERT_TRUE(MAX_BULLETS > 0);
}

void test_bullet_speed_is_positive(void) {
    TEST_ASSERT_TRUE(BULLET_SPEED > 0.0f);
}

void test_bullet_lifetime_is_positive(void) {
    TEST_ASSERT_TRUE(BULLET_LIFETIME > 0.0f);
}

void test_fire_rate_is_positive(void) {
    TEST_ASSERT_TRUE(PISTOL_FIRE_RATE > 0.0f);
}

/* ── Runner ────────────────────────────────────────────────────────────────── */

int main(void) {
    UNITY_BEGIN();

    /* Init */
    RUN_TEST(test_pool_init_all_bullets_inactive);
    RUN_TEST(test_pool_init_fire_cooldown_zero);

    /* Fire */
    RUN_TEST(test_fire_activates_one_bullet);
    RUN_TEST(test_fire_sets_bullet_position);
    RUN_TEST(test_fire_sets_bullet_velocity);
    RUN_TEST(test_fire_sets_bullet_lifetime);
    RUN_TEST(test_fire_sets_cooldown);
    RUN_TEST(test_fire_rate_limited);
    RUN_TEST(test_fire_after_cooldown_expires);
    RUN_TEST(test_fire_pool_full_silently_drops);
    RUN_TEST(test_fire_returns_true_on_success);
    RUN_TEST(test_fire_returns_false_when_rate_limited);

    /* Update */
    RUN_TEST(test_update_moves_bullet);
    RUN_TEST(test_update_decreases_lifetime);
    RUN_TEST(test_update_deactivates_expired_bullet);
    RUN_TEST(test_update_deactivates_bullet_leaving_arena_right);
    RUN_TEST(test_update_deactivates_bullet_leaving_arena_left);
    RUN_TEST(test_update_deactivates_bullet_leaving_arena_top);
    RUN_TEST(test_update_deactivates_bullet_leaving_arena_bottom);
    RUN_TEST(test_update_keeps_bullet_inside_arena);
    RUN_TEST(test_update_skips_inactive_bullets);
    RUN_TEST(test_update_ticks_fire_cooldown);
    RUN_TEST(test_update_clamps_fire_cooldown_to_zero);

    /* Constants */
    RUN_TEST(test_max_bullets_is_positive);
    RUN_TEST(test_bullet_speed_is_positive);
    RUN_TEST(test_bullet_lifetime_is_positive);
    RUN_TEST(test_fire_rate_is_positive);

    return UNITY_END();
}
