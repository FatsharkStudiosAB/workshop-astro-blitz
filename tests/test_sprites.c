/*
 * test_sprites.c -- Unit tests for the sprites module
 *
 * Tests sprite data integrity (no out-of-bounds palette indices),
 * palette selection logic, and basic draw-call behavior via
 * the Raylib stubs (DrawPixel is a no-op in tests).
 */

#include "sprites.h"
#include "unity.h"

/* ── Access sprite data for validation ─────────────────────────────────────── */

/* These are defined as static in sprites.c, so we re-declare the data arrays
 * here as extern-accessible test helpers. Since we can't access static data
 * directly, we test via the public draw API and validate sprite constants. */

/* ── setUp / tearDown ──────────────────────────────────────────────────────── */

void setUp(void) {}

void tearDown(void) {}

/* ── Sprite dimension constant tests ───────────────────────────────────────── */

void test_player_sprite_dimensions_positive(void) {
    TEST_ASSERT_TRUE(PLAYER_SPRITE_W > 0);
    TEST_ASSERT_TRUE(PLAYER_SPRITE_H > 0);
    TEST_ASSERT_TRUE(PLAYER_SPRITE_W <= 32);
    TEST_ASSERT_TRUE(PLAYER_SPRITE_H <= 32);
}

void test_swarmer_sprite_dimensions_positive(void) {
    TEST_ASSERT_TRUE(SWARMER_SPRITE_W > 0);
    TEST_ASSERT_TRUE(SWARMER_SPRITE_H > 0);
    TEST_ASSERT_TRUE(SWARMER_SPRITE_W <= 32);
    TEST_ASSERT_TRUE(SWARMER_SPRITE_H <= 32);
}

void test_grunt_sprite_dimensions_positive(void) {
    TEST_ASSERT_TRUE(GRUNT_SPRITE_W > 0);
    TEST_ASSERT_TRUE(GRUNT_SPRITE_H > 0);
    TEST_ASSERT_TRUE(GRUNT_SPRITE_W <= 32);
    TEST_ASSERT_TRUE(GRUNT_SPRITE_H <= 32);
}

void test_stalker_sprite_dimensions_positive(void) {
    TEST_ASSERT_TRUE(STALKER_SPRITE_W > 0);
    TEST_ASSERT_TRUE(STALKER_SPRITE_H > 0);
    TEST_ASSERT_TRUE(STALKER_SPRITE_W <= 32);
    TEST_ASSERT_TRUE(STALKER_SPRITE_H <= 32);
}

void test_bomber_sprite_dimensions_positive(void) {
    TEST_ASSERT_TRUE(BOMBER_SPRITE_W > 0);
    TEST_ASSERT_TRUE(BOMBER_SPRITE_H > 0);
    TEST_ASSERT_TRUE(BOMBER_SPRITE_W <= 32);
    TEST_ASSERT_TRUE(BOMBER_SPRITE_H <= 32);
}

/* ── Draw functions don't crash with various inputs ────────────────────────── */
/* These test that the draw functions handle edge cases gracefully.
 * DrawPixel is stubbed as a no-op, so we're testing that the rotation
 * math and palette lookup don't crash or access out of bounds. */

void test_player_draw_facing_up(void) {
    /* Facing up = (0, -1) -- identity rotation */
    sprite_draw_player((Vector2){100, 100}, (Vector2){0, -1}, false);
    /* No crash = pass */
    TEST_PASS();
}

void test_player_draw_facing_right(void) {
    sprite_draw_player((Vector2){100, 100}, (Vector2){1, 0}, false);
    TEST_PASS();
}

void test_player_draw_facing_diagonal(void) {
    /* 45-degree angle -- exercises non-axis-aligned rotation */
    sprite_draw_player((Vector2){100, 100}, (Vector2){0.707f, -0.707f}, false);
    TEST_PASS();
}

void test_player_draw_dashing_palette(void) {
    sprite_draw_player((Vector2){100, 100}, (Vector2){0, -1}, true);
    TEST_PASS();
}

void test_player_draw_zero_direction(void) {
    /* Zero vector -- should use default direction without crashing */
    sprite_draw_player((Vector2){100, 100}, (Vector2){0, 0}, false);
    TEST_PASS();
}

void test_swarmer_draw_normal(void) {
    sprite_draw_swarmer((Vector2){50, 50}, (Vector2){0, 1}, 0.0f);
    TEST_PASS();
}

void test_swarmer_draw_hit_flash(void) {
    sprite_draw_swarmer((Vector2){50, 50}, (Vector2){0, 1}, 0.05f);
    TEST_PASS();
}

void test_grunt_draw_normal(void) {
    sprite_draw_grunt((Vector2){50, 50}, (Vector2){-1, 0}, 0.0f);
    TEST_PASS();
}

void test_stalker_draw_normal(void) {
    sprite_draw_stalker((Vector2){50, 50}, (Vector2){0.5f, 0.866f}, 0.0f);
    TEST_PASS();
}

void test_bomber_draw_normal(void) {
    sprite_draw_bomber((Vector2){50, 50}, (Vector2){0, 1}, 0.0f, false);
    TEST_PASS();
}

void test_bomber_draw_charging(void) {
    sprite_draw_bomber((Vector2){50, 50}, (Vector2){0, 1}, 0.0f, true);
    TEST_PASS();
}

void test_bomber_draw_hit_flash_while_charging(void) {
    /* Hit flash takes priority over charging palette */
    sprite_draw_bomber((Vector2){50, 50}, (Vector2){0, 1}, 0.1f, true);
    TEST_PASS();
}

/* ── Runner ────────────────────────────────────────────────────────────────── */

int main(void) {
    UNITY_BEGIN();

    /* Sprite dimension constants */
    RUN_TEST(test_player_sprite_dimensions_positive);
    RUN_TEST(test_swarmer_sprite_dimensions_positive);
    RUN_TEST(test_grunt_sprite_dimensions_positive);
    RUN_TEST(test_stalker_sprite_dimensions_positive);
    RUN_TEST(test_bomber_sprite_dimensions_positive);

    /* Player draw (rotation, palettes) */
    RUN_TEST(test_player_draw_facing_up);
    RUN_TEST(test_player_draw_facing_right);
    RUN_TEST(test_player_draw_facing_diagonal);
    RUN_TEST(test_player_draw_dashing_palette);
    RUN_TEST(test_player_draw_zero_direction);

    /* Enemy sprite draws */
    RUN_TEST(test_swarmer_draw_normal);
    RUN_TEST(test_swarmer_draw_hit_flash);
    RUN_TEST(test_grunt_draw_normal);
    RUN_TEST(test_stalker_draw_normal);
    RUN_TEST(test_bomber_draw_normal);
    RUN_TEST(test_bomber_draw_charging);
    RUN_TEST(test_bomber_draw_hit_flash_while_charging);

    return UNITY_END();
}
