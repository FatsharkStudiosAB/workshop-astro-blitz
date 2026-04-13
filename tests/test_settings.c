/*
 * test_settings.c -- Unit tests for the settings module
 *
 * Tests default initialization, save/load round-trips, and handling of
 * missing or malformed files. Uses temporary files to avoid touching the
 * real settings file.
 */

#include "settings.h"
#include "unity.h"
#include <stdio.h>
#include <string.h>

/* ── Test helpers ──────────────────────────────────────────────────────────── */

static const char *TEST_FILE = "test_settings_tmp.ini";

/* Remove the temporary file if it exists */
static void cleanup_test_file(void) {
    remove(TEST_FILE);
}

/* ── setUp / tearDown ──────────────────────────────────────────────────────── */

void setUp(void) {
    cleanup_test_file();
}

void tearDown(void) {
    cleanup_test_file();
}

/* ── Default initialization tests ──────────────────────────────────────────── */

void test_default_movement_layout_is_8dir(void) {
    /* Verify MOVEMENT_8DIR is the zero value (first enum entry = default) */
    Settings s;
    memset(&s, 0, sizeof(s));
    TEST_ASSERT_EQUAL_INT(MOVEMENT_8DIR, s.movement_layout);
}

void test_load_missing_file_returns_false_and_preserves_value(void) {
    /* TEST_FILE is cleaned up by setUp, so it's guaranteed absent here.
     * settings_load_from returns false and doesn't clobber the caller's
     * value -- same logic settings_init relies on. */
    Settings s = {.movement_layout = MOVEMENT_TANK};
    bool loaded = settings_load_from(&s, TEST_FILE);
    TEST_ASSERT_FALSE(loaded);
    TEST_ASSERT_EQUAL_INT(MOVEMENT_TANK, s.movement_layout);
}

void test_load_from_returns_true_when_file_exists(void) {
    /* Round-trip via the test file to verify load returns true */
    Settings s = {.movement_layout = MOVEMENT_TANK};
    TEST_ASSERT_TRUE(settings_save_to(&s, TEST_FILE));

    Settings s2 = {.movement_layout = MOVEMENT_8DIR};
    bool loaded = settings_load_from(&s2, TEST_FILE);
    TEST_ASSERT_TRUE(loaded);
    TEST_ASSERT_EQUAL_INT(MOVEMENT_TANK, s2.movement_layout);
}

void test_movement_layout_enum_values(void) {
    /* Verify enum values are distinct */
    TEST_ASSERT_NOT_EQUAL(MOVEMENT_TANK, MOVEMENT_8DIR);
}

/* ── Save and load round-trip tests ────────────────────────────────────────── */

void test_save_and_load_tank(void) {
    Settings s_save = {.movement_layout = MOVEMENT_TANK};
    TEST_ASSERT_TRUE(settings_save_to(&s_save, TEST_FILE));

    Settings s_load = {.movement_layout = MOVEMENT_8DIR};
    TEST_ASSERT_TRUE(settings_load_from(&s_load, TEST_FILE));
    TEST_ASSERT_EQUAL_INT(MOVEMENT_TANK, s_load.movement_layout);
}

void test_save_and_load_8dir(void) {
    Settings s_save = {.movement_layout = MOVEMENT_8DIR};
    TEST_ASSERT_TRUE(settings_save_to(&s_save, TEST_FILE));

    Settings s_load = {.movement_layout = MOVEMENT_TANK};
    TEST_ASSERT_TRUE(settings_load_from(&s_load, TEST_FILE));
    TEST_ASSERT_EQUAL_INT(MOVEMENT_8DIR, s_load.movement_layout);
}

void test_save_overwrites_previous(void) {
    Settings s1 = {.movement_layout = MOVEMENT_TANK};
    settings_save_to(&s1, TEST_FILE);

    Settings s2 = {.movement_layout = MOVEMENT_8DIR};
    settings_save_to(&s2, TEST_FILE);

    Settings s_load = {.movement_layout = MOVEMENT_TANK};
    settings_load_from(&s_load, TEST_FILE);
    TEST_ASSERT_EQUAL_INT(MOVEMENT_8DIR, s_load.movement_layout);
}

/* ── Missing file tests ────────────────────────────────────────────────────── */

void test_load_missing_file_returns_false(void) {
    /* TEST_FILE is cleaned up by setUp, so it's guaranteed absent here */
    Settings s = {.movement_layout = MOVEMENT_TANK};
    TEST_ASSERT_FALSE(settings_load_from(&s, TEST_FILE));
}

void test_load_missing_file_preserves_defaults(void) {
    Settings s = {.movement_layout = MOVEMENT_8DIR};
    settings_load_from(&s, TEST_FILE);
    /* Value should be unchanged */
    TEST_ASSERT_EQUAL_INT(MOVEMENT_8DIR, s.movement_layout);
}

/* ── Malformed file tests ──────────────────────────────────────────────────── */

void test_load_empty_file_preserves_defaults(void) {
    /* Create an empty file */
    FILE *f = fopen(TEST_FILE, "w");
    TEST_ASSERT_NOT_NULL(f);
    fclose(f);

    Settings s = {.movement_layout = MOVEMENT_8DIR};
    TEST_ASSERT_TRUE(settings_load_from(&s, TEST_FILE));
    TEST_ASSERT_EQUAL_INT(MOVEMENT_8DIR, s.movement_layout);
}

void test_load_unknown_key_ignored(void) {
    FILE *f = fopen(TEST_FILE, "w");
    TEST_ASSERT_NOT_NULL(f);
    fprintf(f, "unknown_key=some_value\nmovement_layout=8dir\n");
    fclose(f);

    Settings s = {.movement_layout = MOVEMENT_TANK};
    TEST_ASSERT_TRUE(settings_load_from(&s, TEST_FILE));
    TEST_ASSERT_EQUAL_INT(MOVEMENT_8DIR, s.movement_layout);
}

void test_load_invalid_value_preserves_default(void) {
    FILE *f = fopen(TEST_FILE, "w");
    TEST_ASSERT_NOT_NULL(f);
    fprintf(f, "movement_layout=garbage\n");
    fclose(f);

    Settings s = {.movement_layout = MOVEMENT_TANK};
    TEST_ASSERT_TRUE(settings_load_from(&s, TEST_FILE));
    /* Invalid value should keep the existing value (MOVEMENT_TANK) */
    TEST_ASSERT_EQUAL_INT(MOVEMENT_TANK, s.movement_layout);
}

void test_load_comment_lines_ignored(void) {
    FILE *f = fopen(TEST_FILE, "w");
    TEST_ASSERT_NOT_NULL(f);
    fprintf(f, "# This is a comment\n; This is also a comment\nmovement_layout=8dir\n");
    fclose(f);

    Settings s = {.movement_layout = MOVEMENT_TANK};
    TEST_ASSERT_TRUE(settings_load_from(&s, TEST_FILE));
    TEST_ASSERT_EQUAL_INT(MOVEMENT_8DIR, s.movement_layout);
}

void test_load_no_equals_sign_ignored(void) {
    FILE *f = fopen(TEST_FILE, "w");
    TEST_ASSERT_NOT_NULL(f);
    fprintf(f, "this line has no equals sign\nmovement_layout=tank\n");
    fclose(f);

    Settings s = {.movement_layout = MOVEMENT_8DIR};
    TEST_ASSERT_TRUE(settings_load_from(&s, TEST_FILE));
    TEST_ASSERT_EQUAL_INT(MOVEMENT_TANK, s.movement_layout);
}

/* ── Save return value tests ───────────────────────────────────────────────── */

void test_save_returns_true_on_success(void) {
    Settings s = {.movement_layout = MOVEMENT_TANK};
    TEST_ASSERT_TRUE(settings_save_to(&s, TEST_FILE));
}

/* ── File content tests ────────────────────────────────────────────────────── */

void test_save_writes_readable_format(void) {
    Settings s = {.movement_layout = MOVEMENT_8DIR};
    settings_save_to(&s, TEST_FILE);

    /* Read raw file and verify content */
    FILE *f = fopen(TEST_FILE, "r");
    TEST_ASSERT_NOT_NULL(f);

    char buf[256];
    char *result = fgets(buf, sizeof(buf), f);
    fclose(f);

    TEST_ASSERT_NOT_NULL(result);
    /* Should contain the key=value pair */
    TEST_ASSERT_NOT_NULL(strstr(buf, "movement_layout=8dir"));
}

/* ── Visual effect settings tests ───────────────────────────────────────────── */

void test_init_sets_visual_defaults_to_one(void) {
    Settings s;
    memset(&s, 0, sizeof(s));
    settings_init(&s);

    /* All visual effect defaults should be 1.0 */
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, s.screen_shake);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, s.hitstop);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, s.bloom);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, s.scanlines);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, s.chromatic_aberration);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, s.vignette);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, s.lighting);
}

void test_save_and_load_visual_effects(void) {
    Settings s_save;
    settings_init(&s_save);
    s_save.screen_shake = 0.5f;
    s_save.hitstop = 0.0f;
    s_save.bloom = 0.8f;
    s_save.scanlines = 0.3f;
    s_save.chromatic_aberration = 0.1f;
    s_save.vignette = 0.7f;
    s_save.lighting = 0.6f;

    TEST_ASSERT_TRUE(settings_save_to(&s_save, TEST_FILE));

    Settings s_load;
    settings_init(&s_load);
    TEST_ASSERT_TRUE(settings_load_from(&s_load, TEST_FILE));

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, s_load.screen_shake);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, s_load.hitstop);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.8f, s_load.bloom);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.3f, s_load.scanlines);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.1f, s_load.chromatic_aberration);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.7f, s_load.vignette);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.6f, s_load.lighting);
}

void test_load_visual_effects_clamps_out_of_range(void) {
    FILE *f = fopen(TEST_FILE, "w");
    TEST_ASSERT_NOT_NULL(f);
    fprintf(f, "bloom=2.5\nlighting=-0.5\nscreen_shake=1.0\n");
    fclose(f);

    Settings s;
    settings_init(&s);
    TEST_ASSERT_TRUE(settings_load_from(&s, TEST_FILE));

    /* Out-of-range values should be clamped to [0, 1] */
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, s.bloom);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, s.lighting);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, s.screen_shake);
}

void test_load_missing_visual_effects_preserves_defaults(void) {
    /* File only has movement_layout -- visual effects keep defaults */
    FILE *f = fopen(TEST_FILE, "w");
    TEST_ASSERT_NOT_NULL(f);
    fprintf(f, "movement_layout=8dir\n");
    fclose(f);

    Settings s;
    settings_init(&s);
    TEST_ASSERT_TRUE(settings_load_from(&s, TEST_FILE));

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, s.bloom);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, s.lighting);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, s.hitstop);
}

/* ── Runner ────────────────────────────────────────────────────────────────── */

int main(void) {
    UNITY_BEGIN();

    /* Defaults */
    RUN_TEST(test_default_movement_layout_is_8dir);
    RUN_TEST(test_load_missing_file_returns_false_and_preserves_value);
    RUN_TEST(test_load_from_returns_true_when_file_exists);
    RUN_TEST(test_movement_layout_enum_values);

    /* Round-trip */
    RUN_TEST(test_save_and_load_tank);
    RUN_TEST(test_save_and_load_8dir);
    RUN_TEST(test_save_overwrites_previous);

    /* Missing file */
    RUN_TEST(test_load_missing_file_returns_false);
    RUN_TEST(test_load_missing_file_preserves_defaults);

    /* Malformed file */
    RUN_TEST(test_load_empty_file_preserves_defaults);
    RUN_TEST(test_load_unknown_key_ignored);
    RUN_TEST(test_load_invalid_value_preserves_default);
    RUN_TEST(test_load_comment_lines_ignored);
    RUN_TEST(test_load_no_equals_sign_ignored);

    /* Save */
    RUN_TEST(test_save_returns_true_on_success);
    RUN_TEST(test_save_writes_readable_format);

    /* Visual effect settings */
    RUN_TEST(test_init_sets_visual_defaults_to_one);
    RUN_TEST(test_save_and_load_visual_effects);
    RUN_TEST(test_load_visual_effects_clamps_out_of_range);
    RUN_TEST(test_load_missing_visual_effects_preserves_defaults);

    return UNITY_END();
}
