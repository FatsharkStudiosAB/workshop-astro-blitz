/*
 * test_tilemap.c -- Unit tests for the tilemap module
 *
 * Tests initialization, procedural generation (borders, spawn clearing,
 * obstacle placement), collision queries, coordinate conversion, and
 * world bounds.
 */

#include "tilemap.h"
#include "unity.h"
#include <math.h>

/* ── Test helpers ──────────────────────────────────────────────────────────── */

static const float FLOAT_TOLERANCE = 0.001f;

/* ── setUp / tearDown ──────────────────────────────────────────────────────── */

void setUp(void) {}
void tearDown(void) {}

/* ── tilemap_init tests ────────────────────────────────────────────────────── */

void test_init_sets_dimensions(void) {
    Tilemap tm;
    tilemap_init(&tm);

    TEST_ASSERT_EQUAL_INT(WORLD_COLS, tm.cols);
    TEST_ASSERT_EQUAL_INT(WORLD_ROWS, tm.rows);
    TEST_ASSERT_EQUAL_INT(TILE_SIZE, tm.tile_size);
}

void test_init_all_tiles_empty(void) {
    Tilemap tm;
    tilemap_init(&tm);

    for (int y = 0; y < tm.rows; y++) {
        for (int x = 0; x < tm.cols; x++) {
            TEST_ASSERT_EQUAL_INT(TILE_EMPTY, tm.tiles[y][x]);
        }
    }
}

/* ── tilemap_generate tests ────────────────────────────────────────────────── */

void test_generate_border_walls_top(void) {
    Tilemap tm;
    tilemap_generate(&tm, WORLD_COLS / 2, WORLD_ROWS / 2, 42);

    for (int x = 0; x < tm.cols; x++) {
        TEST_ASSERT_EQUAL_INT(TILE_WALL, tm.tiles[0][x]);
    }
}

void test_generate_border_walls_bottom(void) {
    Tilemap tm;
    tilemap_generate(&tm, WORLD_COLS / 2, WORLD_ROWS / 2, 42);

    for (int x = 0; x < tm.cols; x++) {
        TEST_ASSERT_EQUAL_INT(TILE_WALL, tm.tiles[tm.rows - 1][x]);
    }
}

void test_generate_border_walls_left(void) {
    Tilemap tm;
    tilemap_generate(&tm, WORLD_COLS / 2, WORLD_ROWS / 2, 42);

    for (int y = 0; y < tm.rows; y++) {
        TEST_ASSERT_EQUAL_INT(TILE_WALL, tm.tiles[y][0]);
    }
}

void test_generate_border_walls_right(void) {
    Tilemap tm;
    tilemap_generate(&tm, WORLD_COLS / 2, WORLD_ROWS / 2, 42);

    for (int y = 0; y < tm.rows; y++) {
        TEST_ASSERT_EQUAL_INT(TILE_WALL, tm.tiles[y][tm.cols - 1]);
    }
}

void test_generate_spawn_area_clear(void) {
    int sx = WORLD_COLS / 2;
    int sy = WORLD_ROWS / 2;
    Tilemap tm;
    tilemap_generate(&tm, sx, sy, 42);

    /* Every tile within SPAWN_CLEAR_RADIUS of spawn must be empty */
    int r2 = SPAWN_CLEAR_RADIUS * SPAWN_CLEAR_RADIUS;
    for (int dy = -SPAWN_CLEAR_RADIUS; dy <= SPAWN_CLEAR_RADIUS; dy++) {
        for (int dx = -SPAWN_CLEAR_RADIUS; dx <= SPAWN_CLEAR_RADIUS; dx++) {
            if (dx * dx + dy * dy <= r2) {
                int tx = sx + dx;
                int ty = sy + dy;
                TEST_ASSERT_EQUAL_INT_MESSAGE(TILE_EMPTY, tm.tiles[ty][tx],
                                              "Tile in spawn zone should be empty");
            }
        }
    }
}

void test_generate_has_some_obstacles(void) {
    Tilemap tm;
    tilemap_generate(&tm, WORLD_COLS / 2, WORLD_ROWS / 2, 42);

    /* Count interior walls (exclude border) */
    int wall_count = 0;
    for (int y = 1; y < tm.rows - 1; y++) {
        for (int x = 1; x < tm.cols - 1; x++) {
            if (tm.tiles[y][x] == TILE_WALL) {
                wall_count++;
            }
        }
    }

    /* With OBSTACLE_DENSITY ~8% plus clusters, should have at least some walls */
    TEST_ASSERT_TRUE(wall_count > 50);
}

void test_generate_deterministic_with_same_seed(void) {
    Tilemap tm1, tm2;
    int sx = WORLD_COLS / 2;
    int sy = WORLD_ROWS / 2;

    tilemap_generate(&tm1, sx, sy, 12345);
    tilemap_generate(&tm2, sx, sy, 12345);

    for (int y = 0; y < tm1.rows; y++) {
        for (int x = 0; x < tm1.cols; x++) {
            TEST_ASSERT_EQUAL_INT(tm1.tiles[y][x], tm2.tiles[y][x]);
        }
    }
}

void test_generate_different_seeds_produce_different_maps(void) {
    Tilemap tm1, tm2;
    int sx = WORLD_COLS / 2;
    int sy = WORLD_ROWS / 2;

    tilemap_generate(&tm1, sx, sy, 111);
    tilemap_generate(&tm2, sx, sy, 222);

    /* Count differences in interior (borders are identical) */
    int diff_count = 0;
    for (int y = 2; y < tm1.rows - 2; y++) {
        for (int x = 2; x < tm1.cols - 2; x++) {
            if (tm1.tiles[y][x] != tm2.tiles[y][x]) {
                diff_count++;
            }
        }
    }

    TEST_ASSERT_TRUE(diff_count > 0);
}

/* ── tilemap_is_solid / tilemap_is_tile_solid tests ───────────────────────── */

void test_is_tile_solid_wall(void) {
    Tilemap tm;
    tilemap_init(&tm);
    tm.tiles[5][5] = TILE_WALL;

    TEST_ASSERT_TRUE(tilemap_is_tile_solid(&tm, 5, 5));
}

void test_is_tile_solid_empty(void) {
    Tilemap tm;
    tilemap_init(&tm);

    TEST_ASSERT_FALSE(tilemap_is_tile_solid(&tm, 5, 5));
}

void test_is_tile_solid_out_of_bounds_negative(void) {
    Tilemap tm;
    tilemap_init(&tm);

    TEST_ASSERT_TRUE(tilemap_is_tile_solid(&tm, -1, 0));
    TEST_ASSERT_TRUE(tilemap_is_tile_solid(&tm, 0, -1));
}

void test_is_tile_solid_out_of_bounds_large(void) {
    Tilemap tm;
    tilemap_init(&tm);

    TEST_ASSERT_TRUE(tilemap_is_tile_solid(&tm, WORLD_COLS, 0));
    TEST_ASSERT_TRUE(tilemap_is_tile_solid(&tm, 0, WORLD_ROWS));
}

void test_is_solid_world_coords(void) {
    Tilemap tm;
    tilemap_init(&tm);
    tm.tiles[3][4] = TILE_WALL;

    /* Pixel in the middle of tile (4, 3) */
    float wx = 4.0f * TILE_SIZE + TILE_SIZE / 2.0f;
    float wy = 3.0f * TILE_SIZE + TILE_SIZE / 2.0f;

    TEST_ASSERT_TRUE(tilemap_is_solid(&tm, wx, wy));
}

void test_is_solid_world_coords_empty(void) {
    Tilemap tm;
    tilemap_init(&tm);

    /* Tile (5, 5) is empty by default */
    float wx = 5.0f * TILE_SIZE + TILE_SIZE / 2.0f;
    float wy = 5.0f * TILE_SIZE + TILE_SIZE / 2.0f;

    TEST_ASSERT_FALSE(tilemap_is_solid(&tm, wx, wy));
}

void test_is_solid_negative_coords(void) {
    Tilemap tm;
    tilemap_init(&tm);

    TEST_ASSERT_TRUE(tilemap_is_solid(&tm, -10.0f, -10.0f));
}

/* ── Coordinate conversion tests ──────────────────────────────────────────── */

void test_world_to_tile_origin(void) {
    Tilemap tm;
    tilemap_init(&tm);

    int tx, ty;
    tilemap_world_to_tile(&tm, 0.0f, 0.0f, &tx, &ty);

    TEST_ASSERT_EQUAL_INT(0, tx);
    TEST_ASSERT_EQUAL_INT(0, ty);
}

void test_world_to_tile_middle(void) {
    Tilemap tm;
    tilemap_init(&tm);

    int tx, ty;
    float wx = 4.5f * TILE_SIZE;
    float wy = 3.5f * TILE_SIZE;
    tilemap_world_to_tile(&tm, wx, wy, &tx, &ty);

    TEST_ASSERT_EQUAL_INT(4, tx);
    TEST_ASSERT_EQUAL_INT(3, ty);
}

void test_world_to_tile_edge(void) {
    Tilemap tm;
    tilemap_init(&tm);

    int tx, ty;
    /* Just past tile boundary */
    float wx = 2.0f * TILE_SIZE + 1.0f;
    float wy = 5.0f * TILE_SIZE + 1.0f;
    tilemap_world_to_tile(&tm, wx, wy, &tx, &ty);

    TEST_ASSERT_EQUAL_INT(2, tx);
    TEST_ASSERT_EQUAL_INT(5, ty);
}

void test_tile_to_world(void) {
    Tilemap tm;
    tilemap_init(&tm);

    Vector2 pos = tilemap_tile_to_world(&tm, 3, 7);

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 3.0f * TILE_SIZE, pos.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 7.0f * TILE_SIZE, pos.y);
}

void test_tile_to_world_origin(void) {
    Tilemap tm;
    tilemap_init(&tm);

    Vector2 pos = tilemap_tile_to_world(&tm, 0, 0);

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 0.0f, pos.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 0.0f, pos.y);
}

/* ── World bounds tests ───────────────────────────────────────────────────── */

void test_world_bounds(void) {
    Tilemap tm;
    tilemap_init(&tm);

    Rectangle bounds = tilemap_get_world_bounds(&tm);

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 0.0f, bounds.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 0.0f, bounds.y);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, (float)WORLD_WIDTH, bounds.width);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, (float)WORLD_HEIGHT, bounds.height);
}

/* ── Constants sanity checks ───────────────────────────────────────────────── */

void test_tile_size_positive(void) {
    TEST_ASSERT_TRUE(TILE_SIZE > 0);
}

void test_world_dimensions_positive(void) {
    TEST_ASSERT_TRUE(WORLD_COLS > 0);
    TEST_ASSERT_TRUE(WORLD_ROWS > 0);
    TEST_ASSERT_TRUE(WORLD_WIDTH > 0);
    TEST_ASSERT_TRUE(WORLD_HEIGHT > 0);
}

void test_spawn_clear_radius_positive(void) {
    TEST_ASSERT_TRUE(SPAWN_CLEAR_RADIUS > 0);
}

void test_obstacle_density_in_range(void) {
    TEST_ASSERT_TRUE(OBSTACLE_DENSITY >= 0);
    TEST_ASSERT_TRUE(OBSTACLE_DENSITY <= 100);
}

void test_world_larger_than_screen(void) {
    /* The whole point: world must be bigger than the viewport */
    TEST_ASSERT_TRUE(WORLD_WIDTH > 800);
    TEST_ASSERT_TRUE(WORLD_HEIGHT > 600);
}

/* ── Runner ────────────────────────────────────────────────────────────────── */

int main(void) {
    UNITY_BEGIN();

    /* Init */
    RUN_TEST(test_init_sets_dimensions);
    RUN_TEST(test_init_all_tiles_empty);

    /* Generate */
    RUN_TEST(test_generate_border_walls_top);
    RUN_TEST(test_generate_border_walls_bottom);
    RUN_TEST(test_generate_border_walls_left);
    RUN_TEST(test_generate_border_walls_right);
    RUN_TEST(test_generate_spawn_area_clear);
    RUN_TEST(test_generate_has_some_obstacles);
    RUN_TEST(test_generate_deterministic_with_same_seed);
    RUN_TEST(test_generate_different_seeds_produce_different_maps);

    /* Tile queries */
    RUN_TEST(test_is_tile_solid_wall);
    RUN_TEST(test_is_tile_solid_empty);
    RUN_TEST(test_is_tile_solid_out_of_bounds_negative);
    RUN_TEST(test_is_tile_solid_out_of_bounds_large);
    RUN_TEST(test_is_solid_world_coords);
    RUN_TEST(test_is_solid_world_coords_empty);
    RUN_TEST(test_is_solid_negative_coords);

    /* Coordinate conversion */
    RUN_TEST(test_world_to_tile_origin);
    RUN_TEST(test_world_to_tile_middle);
    RUN_TEST(test_world_to_tile_edge);
    RUN_TEST(test_tile_to_world);
    RUN_TEST(test_tile_to_world_origin);

    /* World bounds */
    RUN_TEST(test_world_bounds);

    /* Constants */
    RUN_TEST(test_tile_size_positive);
    RUN_TEST(test_world_dimensions_positive);
    RUN_TEST(test_spawn_clear_radius_positive);
    RUN_TEST(test_obstacle_density_in_range);
    RUN_TEST(test_world_larger_than_screen);

    return UNITY_END();
}
