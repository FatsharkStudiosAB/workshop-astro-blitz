/*
 * Tests for the vec2 2D vector math module.
 */

#include "unity.h"
#include "vec2.h"
#include <math.h>

/* Tolerance for floating-point comparisons. */
#define FLOAT_EPSILON 1e-5f

void setUp(void) {
    /* Called before each test. */
}

void tearDown(void) {
    /* Called after each test. */
}

/* --- vec2_add --- */

void test_vec2_add_basic(void) {
    Vec2 a = {1.0f, 2.0f};
    Vec2 b = {3.0f, 4.0f};
    Vec2 result = vec2_add(a, b);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 4.0f, result.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 6.0f, result.y);
}

void test_vec2_add_negative(void) {
    Vec2 a = {-1.0f, -2.0f};
    Vec2 b = {3.0f, -4.0f};
    Vec2 result = vec2_add(a, b);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 2.0f, result.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, -6.0f, result.y);
}

void test_vec2_add_zero(void) {
    Vec2 a = {5.0f, 7.0f};
    Vec2 zero = {0.0f, 0.0f};
    Vec2 result = vec2_add(a, zero);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 5.0f, result.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 7.0f, result.y);
}

/* --- vec2_sub --- */

void test_vec2_sub_basic(void) {
    Vec2 a = {5.0f, 7.0f};
    Vec2 b = {2.0f, 3.0f};
    Vec2 result = vec2_sub(a, b);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 3.0f, result.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 4.0f, result.y);
}

void test_vec2_sub_same_vector(void) {
    Vec2 a = {3.0f, 4.0f};
    Vec2 result = vec2_sub(a, a);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.0f, result.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.0f, result.y);
}

/* --- vec2_scale --- */

void test_vec2_scale_basic(void) {
    Vec2 a = {3.0f, 4.0f};
    Vec2 result = vec2_scale(a, 2.0f);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 6.0f, result.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 8.0f, result.y);
}

void test_vec2_scale_zero(void) {
    Vec2 a = {3.0f, 4.0f};
    Vec2 result = vec2_scale(a, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.0f, result.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.0f, result.y);
}

void test_vec2_scale_negative(void) {
    Vec2 a = {3.0f, 4.0f};
    Vec2 result = vec2_scale(a, -1.0f);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, -3.0f, result.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, -4.0f, result.y);
}

void test_vec2_scale_fractional(void) {
    Vec2 a = {10.0f, 20.0f};
    Vec2 result = vec2_scale(a, 0.5f);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 5.0f, result.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 10.0f, result.y);
}

/* --- vec2_length --- */

void test_vec2_length_3_4_5_triangle(void) {
    Vec2 a = {3.0f, 4.0f};
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 5.0f, vec2_length(a));
}

void test_vec2_length_zero(void) {
    Vec2 zero = {0.0f, 0.0f};
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.0f, vec2_length(zero));
}

void test_vec2_length_unit_x(void) {
    Vec2 a = {1.0f, 0.0f};
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 1.0f, vec2_length(a));
}

void test_vec2_length_unit_y(void) {
    Vec2 a = {0.0f, 1.0f};
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 1.0f, vec2_length(a));
}

void test_vec2_length_negative_components(void) {
    Vec2 a = {-3.0f, -4.0f};
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 5.0f, vec2_length(a));
}

/* --- vec2_normalize --- */

void test_vec2_normalize_basic(void) {
    Vec2 a = {3.0f, 4.0f};
    Vec2 result = vec2_normalize(a);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.6f, result.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.8f, result.y);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 1.0f, vec2_length(result));
}

void test_vec2_normalize_already_unit(void) {
    Vec2 a = {1.0f, 0.0f};
    Vec2 result = vec2_normalize(a);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 1.0f, result.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.0f, result.y);
}

void test_vec2_normalize_zero_returns_zero(void) {
    Vec2 zero = {0.0f, 0.0f};
    Vec2 result = vec2_normalize(zero);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.0f, result.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.0f, result.y);
}

void test_vec2_normalize_large_vector(void) {
    Vec2 a = {1000.0f, 0.0f};
    Vec2 result = vec2_normalize(a);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 1.0f, result.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.0f, result.y);
}

/* --- vec2_distance --- */

void test_vec2_distance_basic(void) {
    Vec2 a = {0.0f, 0.0f};
    Vec2 b = {3.0f, 4.0f};
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 5.0f, vec2_distance(a, b));
}

void test_vec2_distance_same_point(void) {
    Vec2 a = {5.0f, 5.0f};
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.0f, vec2_distance(a, a));
}

void test_vec2_distance_is_symmetric(void) {
    Vec2 a = {1.0f, 2.0f};
    Vec2 b = {4.0f, 6.0f};
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, vec2_distance(a, b), vec2_distance(b, a));
}

/* --- vec2_dot --- */

void test_vec2_dot_perpendicular(void) {
    Vec2 a = {1.0f, 0.0f};
    Vec2 b = {0.0f, 1.0f};
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.0f, vec2_dot(a, b));
}

void test_vec2_dot_parallel(void) {
    Vec2 a = {2.0f, 3.0f};
    Vec2 b = {2.0f, 3.0f};
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 13.0f, vec2_dot(a, b));
}

void test_vec2_dot_opposite(void) {
    Vec2 a = {1.0f, 0.0f};
    Vec2 b = {-1.0f, 0.0f};
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, -1.0f, vec2_dot(a, b));
}

void test_vec2_dot_with_zero(void) {
    Vec2 a = {5.0f, 7.0f};
    Vec2 zero = {0.0f, 0.0f};
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.0f, vec2_dot(a, zero));
}

/* --- vec2_lerp --- */

void test_vec2_lerp_at_zero(void) {
    Vec2 a = {0.0f, 0.0f};
    Vec2 b = {10.0f, 20.0f};
    Vec2 result = vec2_lerp(a, b, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.0f, result.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.0f, result.y);
}

void test_vec2_lerp_at_one(void) {
    Vec2 a = {0.0f, 0.0f};
    Vec2 b = {10.0f, 20.0f};
    Vec2 result = vec2_lerp(a, b, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 10.0f, result.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 20.0f, result.y);
}

void test_vec2_lerp_at_half(void) {
    Vec2 a = {0.0f, 0.0f};
    Vec2 b = {10.0f, 20.0f};
    Vec2 result = vec2_lerp(a, b, 0.5f);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 5.0f, result.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 10.0f, result.y);
}

int main(void) {
    UNITY_BEGIN();

    /* vec2_add */
    RUN_TEST(test_vec2_add_basic);
    RUN_TEST(test_vec2_add_negative);
    RUN_TEST(test_vec2_add_zero);

    /* vec2_sub */
    RUN_TEST(test_vec2_sub_basic);
    RUN_TEST(test_vec2_sub_same_vector);

    /* vec2_scale */
    RUN_TEST(test_vec2_scale_basic);
    RUN_TEST(test_vec2_scale_zero);
    RUN_TEST(test_vec2_scale_negative);
    RUN_TEST(test_vec2_scale_fractional);

    /* vec2_length */
    RUN_TEST(test_vec2_length_3_4_5_triangle);
    RUN_TEST(test_vec2_length_zero);
    RUN_TEST(test_vec2_length_unit_x);
    RUN_TEST(test_vec2_length_unit_y);
    RUN_TEST(test_vec2_length_negative_components);

    /* vec2_normalize */
    RUN_TEST(test_vec2_normalize_basic);
    RUN_TEST(test_vec2_normalize_already_unit);
    RUN_TEST(test_vec2_normalize_zero_returns_zero);
    RUN_TEST(test_vec2_normalize_large_vector);

    /* vec2_distance */
    RUN_TEST(test_vec2_distance_basic);
    RUN_TEST(test_vec2_distance_same_point);
    RUN_TEST(test_vec2_distance_is_symmetric);

    /* vec2_dot */
    RUN_TEST(test_vec2_dot_perpendicular);
    RUN_TEST(test_vec2_dot_parallel);
    RUN_TEST(test_vec2_dot_opposite);
    RUN_TEST(test_vec2_dot_with_zero);

    /* vec2_lerp */
    RUN_TEST(test_vec2_lerp_at_zero);
    RUN_TEST(test_vec2_lerp_at_one);
    RUN_TEST(test_vec2_lerp_at_half);

    return UNITY_END();
}
