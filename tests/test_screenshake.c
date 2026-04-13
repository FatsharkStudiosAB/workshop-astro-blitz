/*
 * test_screenshake.c -- Unit tests for screen shake system
 */

#include "unity.h"

#include "screenshake.h"

#define FLOAT_EPSILON 1e-3f

static ScreenShake shake;

void setUp(void) {
    screenshake_init(&shake);
}

void tearDown(void) {}

/* ── Init tests ───────────────────────────────────────────────────────────── */

void test_init_zero_trauma(void) {
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.0f, shake.trauma);
}

/* ── Trauma tests ─────────────────────────────────────────────────────────── */

void test_add_trauma(void) {
    screenshake_add_trauma(&shake, 0.3f);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.3f, shake.trauma);
}

void test_trauma_clamped_to_one(void) {
    screenshake_add_trauma(&shake, 0.8f);
    screenshake_add_trauma(&shake, 0.5f);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 1.0f, shake.trauma);
}

void test_trauma_stacks(void) {
    screenshake_add_trauma(&shake, 0.2f);
    screenshake_add_trauma(&shake, 0.3f);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.5f, shake.trauma);
}

/* ── Decay tests ──────────────────────────────────────────────────────────── */

void test_trauma_decays(void) {
    screenshake_add_trauma(&shake, 1.0f);
    screenshake_update(&shake, 0.1f);
    /* Decay: 1.0 - 3.0 * 0.1 = 0.7 */
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.7f, shake.trauma);
}

void test_trauma_decays_to_zero(void) {
    screenshake_add_trauma(&shake, 0.1f);
    screenshake_update(&shake, 1.0f); /* More than enough to reach 0 */
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.0f, shake.trauma);
}

void test_no_negative_trauma(void) {
    screenshake_update(&shake, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.0f, shake.trauma);
}

/* ── Apply tests ──────────────────────────────────────────────────────────── */

void test_apply_no_trauma_no_change(void) {
    Camera2D cam = {0};
    cam.target = (Vector2){100.0f, 200.0f};
    cam.offset = (Vector2){400.0f, 300.0f};
    cam.zoom = 1.0f;

    Camera2D result = screenshake_apply(&shake, cam);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 100.0f, result.target.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 200.0f, result.target.y);
}

void test_apply_with_trauma_offsets_target(void) {
    screenshake_add_trauma(&shake, 1.0f);

    Camera2D cam = {0};
    cam.target = (Vector2){100.0f, 200.0f};
    cam.offset = (Vector2){400.0f, 300.0f};
    cam.zoom = 1.0f;

    /* Run multiple times to verify offset is within bounds */
    for (int i = 0; i < 20; i++) {
        Camera2D result = screenshake_apply(&shake, cam);
        float dx = result.target.x - cam.target.x;
        float dy = result.target.y - cam.target.y;
        /* Offset should be within SHAKE_MAX_OFFSET */
        TEST_ASSERT_FLOAT_WITHIN(SHAKE_MAX_OFFSET + 0.1f, 0.0f, dx);
        TEST_ASSERT_FLOAT_WITHIN(SHAKE_MAX_OFFSET + 0.1f, 0.0f, dy);
    }
}

/* ── Constants tests ──────────────────────────────────────────────────────── */

void test_shake_max_offset_constant(void) {
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 8.0f, SHAKE_MAX_OFFSET);
}

void test_shake_decay_rate_constant(void) {
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 3.0f, SHAKE_DECAY_RATE);
}

/* ── Main ─────────────────────────────────────────────────────────────────── */

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_zero_trauma);
    RUN_TEST(test_add_trauma);
    RUN_TEST(test_trauma_clamped_to_one);
    RUN_TEST(test_trauma_stacks);
    RUN_TEST(test_trauma_decays);
    RUN_TEST(test_trauma_decays_to_zero);
    RUN_TEST(test_no_negative_trauma);
    RUN_TEST(test_apply_no_trauma_no_change);
    RUN_TEST(test_apply_with_trauma_offsets_target);
    RUN_TEST(test_shake_max_offset_constant);
    RUN_TEST(test_shake_decay_rate_constant);
    return UNITY_END();
}
