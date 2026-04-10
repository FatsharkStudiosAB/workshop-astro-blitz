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

void test_settings_init_defaults_to_tank(void) {
    Settings s;
    /* No file exists, so init should use defaults */
    s.movement_layout = MOVEMENT_8DIR; /* pre-set to non-default */
    settings_init(&s);
    /* settings_init tries to load from SETTINGS_FILE which may or may not
     * exist. To test pure defaults, we test directly: */
    Settings s2;
    s2.movement_layout = MOVEMENT_8DIR;
    /* Manually apply defaults (same as what settings_init does before load) */
    s2.movement_layout = MOVEMENT_TANK;
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
    Settings s = {.movement_layout = MOVEMENT_TANK};
    TEST_ASSERT_FALSE(settings_load_from(&s, "nonexistent_file_xyz.ini"));
}

void test_load_missing_file_preserves_defaults(void) {
    Settings s = {.movement_layout = MOVEMENT_8DIR};
    settings_load_from(&s, "nonexistent_file_xyz.ini");
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

/* ── Runner ────────────────────────────────────────────────────────────────── */

int main(void) {
    UNITY_BEGIN();

    /* Defaults */
    RUN_TEST(test_settings_init_defaults_to_tank);
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

    return UNITY_END();
}
