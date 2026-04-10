#include "unity.h"

void setUp(void) {
    /* Called before each test. */
}

void tearDown(void) {
    /* Called after each test. */
}

/* --- Sample tests to verify the framework works --- */

void test_true_is_true(void) {
    TEST_ASSERT_TRUE(1);
}

void test_integer_equality(void) {
    TEST_ASSERT_EQUAL_INT(42, 42);
}

void test_string_equality(void) {
    TEST_ASSERT_EQUAL_STRING("astro", "astro");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_true_is_true);
    RUN_TEST(test_integer_equality);
    RUN_TEST(test_string_equality);
    return UNITY_END();
}
