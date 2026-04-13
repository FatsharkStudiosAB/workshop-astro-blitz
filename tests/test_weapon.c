/*
 * test_weapon.c -- Unit tests for the weapon system
 */

#include "bullet.h"
#include "unity.h"
#include "weapon.h"

void setUp(void) {}
void tearDown(void) {}

/* ── Preset tests ──────────────────────────────────────────────────────────── */

void test_weapon_get_default_returns_pistol(void) {
    Weapon w = weapon_get_default();
    TEST_ASSERT_EQUAL(WEAPON_PISTOL, w.type);
    TEST_ASSERT_EQUAL_STRING("Pistol", w.name);
}

void test_weapon_get_preset_pistol(void) {
    Weapon w = weapon_get_preset(WEAPON_PISTOL);
    TEST_ASSERT_EQUAL(WEAPON_PISTOL, w.type);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.25f, w.fire_rate);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, w.damage);
    TEST_ASSERT_EQUAL(1, w.projectile_count);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, w.spread_angle);
}

void test_weapon_get_preset_smg(void) {
    Weapon w = weapon_get_preset(WEAPON_SMG);
    TEST_ASSERT_EQUAL(WEAPON_SMG, w.type);
    TEST_ASSERT_EQUAL_STRING("SMG", w.name);
    TEST_ASSERT_TRUE(w.fire_rate < 0.1f); /* faster than pistol */
    TEST_ASSERT_TRUE(w.damage < 1.0f);    /* less damage per bullet */
    TEST_ASSERT_EQUAL(1, w.projectile_count);
    TEST_ASSERT_TRUE(w.spread_angle > 0.0f); /* has spread */
}

void test_weapon_get_preset_shotgun(void) {
    Weapon w = weapon_get_preset(WEAPON_SHOTGUN);
    TEST_ASSERT_EQUAL(WEAPON_SHOTGUN, w.type);
    TEST_ASSERT_EQUAL_STRING("Shotgun", w.name);
    TEST_ASSERT_TRUE(w.projectile_count > 1); /* multiple pellets */
    TEST_ASSERT_TRUE(w.spread_angle > 0.0f);  /* wide spread */
    TEST_ASSERT_TRUE(w.fire_rate > 0.25f);    /* slower than pistol */
}

void test_weapon_get_preset_plasma(void) {
    Weapon w = weapon_get_preset(WEAPON_PLASMA);
    TEST_ASSERT_EQUAL(WEAPON_PLASMA, w.type);
    TEST_ASSERT_EQUAL_STRING("Plasma", w.name);
    TEST_ASSERT_TRUE(w.damage > 1.0f); /* high damage */
    TEST_ASSERT_EQUAL(1, w.projectile_count);
}

void test_weapon_get_preset_invalid_returns_pistol(void) {
    Weapon w = weapon_get_preset((WeaponType)999);
    TEST_ASSERT_EQUAL(WEAPON_PISTOL, w.type);
}

void test_weapon_count_is_four(void) {
    TEST_ASSERT_EQUAL(4, WEAPON_COUNT);
}

/* ── Bullet firing with weapon parameters ──────────────────────────────────── */

void test_fire_weapon_single_bullet(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    Weapon w = weapon_get_preset(WEAPON_PISTOL);
    Vector2 origin = {100.0f, 100.0f};
    Vector2 dir = {1.0f, 0.0f};

    int fired = bullet_pool_fire_weapon(&pool, origin, dir, w.fire_rate, w.damage, w.bullet_speed,
                                        w.spread_angle, w.projectile_count, w.bullet_lifetime,
                                        w.bullet_color);

    TEST_ASSERT_EQUAL(1, fired);

    /* Verify bullet properties */
    TEST_ASSERT_TRUE(pool.bullets[0].active);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, w.damage, pool.bullets[0].damage);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, w.bullet_lifetime, pool.bullets[0].lifetime);
}

void test_fire_weapon_shotgun_multiple_bullets(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    Weapon w = weapon_get_preset(WEAPON_SHOTGUN);
    Vector2 origin = {100.0f, 100.0f};
    Vector2 dir = {1.0f, 0.0f};

    int fired = bullet_pool_fire_weapon(&pool, origin, dir, w.fire_rate, w.damage, w.bullet_speed,
                                        w.spread_angle, w.projectile_count, w.bullet_lifetime,
                                        w.bullet_color);

    TEST_ASSERT_EQUAL(w.projectile_count, fired);

    /* Count active bullets */
    int active = 0;
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (pool.bullets[i].active) {
            active++;
        }
    }
    TEST_ASSERT_EQUAL(w.projectile_count, active);
}

void test_fire_weapon_respects_cooldown(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    Weapon w = weapon_get_preset(WEAPON_PISTOL);
    Vector2 origin = {100.0f, 100.0f};
    Vector2 dir = {1.0f, 0.0f};

    /* First shot succeeds */
    int fired1 = bullet_pool_fire_weapon(&pool, origin, dir, w.fire_rate, w.damage, w.bullet_speed,
                                         w.spread_angle, w.projectile_count, w.bullet_lifetime,
                                         w.bullet_color);
    TEST_ASSERT_EQUAL(1, fired1);

    /* Second shot immediately should be rate-limited */
    int fired2 = bullet_pool_fire_weapon(&pool, origin, dir, w.fire_rate, w.damage, w.bullet_speed,
                                         w.spread_angle, w.projectile_count, w.bullet_lifetime,
                                         w.bullet_color);
    TEST_ASSERT_EQUAL(0, fired2);
}

void test_fire_weapon_shotgun_spread_varies_direction(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    Weapon w = weapon_get_preset(WEAPON_SHOTGUN);
    Vector2 origin = {100.0f, 100.0f};
    Vector2 dir = {1.0f, 0.0f};

    bullet_pool_fire_weapon(&pool, origin, dir, w.fire_rate, w.damage, w.bullet_speed,
                            w.spread_angle, w.projectile_count, w.bullet_lifetime, w.bullet_color);

    /* With 5 pellets and 30 degree spread, not all should have identical velocities */
    bool all_same = true;
    for (int i = 1; i < w.projectile_count; i++) {
        if (pool.bullets[i].velocity.y != pool.bullets[0].velocity.y) {
            all_same = false;
            break;
        }
    }
    TEST_ASSERT_FALSE(all_same);
}

void test_fire_weapon_sets_bullet_color(void) {
    BulletPool pool;
    bullet_pool_init(&pool);

    Weapon w = weapon_get_preset(WEAPON_PLASMA);
    Vector2 origin = {100.0f, 100.0f};
    Vector2 dir = {1.0f, 0.0f};

    bullet_pool_fire_weapon(&pool, origin, dir, w.fire_rate, w.damage, w.bullet_speed,
                            w.spread_angle, w.projectile_count, w.bullet_lifetime, w.bullet_color);

    TEST_ASSERT_EQUAL(w.bullet_color.r, pool.bullets[0].color.r);
    TEST_ASSERT_EQUAL(w.bullet_color.g, pool.bullets[0].color.g);
    TEST_ASSERT_EQUAL(w.bullet_color.b, pool.bullets[0].color.b);
}

/* ── Runner ────────────────────────────────────────────────────────────────── */

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_weapon_get_default_returns_pistol);
    RUN_TEST(test_weapon_get_preset_pistol);
    RUN_TEST(test_weapon_get_preset_smg);
    RUN_TEST(test_weapon_get_preset_shotgun);
    RUN_TEST(test_weapon_get_preset_plasma);
    RUN_TEST(test_weapon_get_preset_invalid_returns_pistol);
    RUN_TEST(test_weapon_count_is_four);
    RUN_TEST(test_fire_weapon_single_bullet);
    RUN_TEST(test_fire_weapon_shotgun_multiple_bullets);
    RUN_TEST(test_fire_weapon_respects_cooldown);
    RUN_TEST(test_fire_weapon_shotgun_spread_varies_direction);
    RUN_TEST(test_fire_weapon_sets_bullet_color);
    return UNITY_END();
}
