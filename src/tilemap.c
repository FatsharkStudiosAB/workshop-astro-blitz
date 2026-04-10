/*
 * tilemap.c -- World grid, procedural generation, and tile collision
 */

#include "tilemap.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ── Helpers ───────────────────────────────────────────────────────────────── */

/* Simple seeded random: uses C stdlib srand/rand. Not cryptographic --
 * fine for procedural level generation. */
static void seed_rng(unsigned int seed) {
    if (seed == 0) {
        /* Use a time-based or Raylib random seed */
        seed = (unsigned int)GetRandomValue(1, 2147483647);
    }
    srand(seed);
}

static int rand_range(int min, int max) {
    if (min >= max) {
        return min;
    }
    return min + (rand() % (max - min + 1));
}

/* ── Public ────────────────────────────────────────────────────────────────── */

void tilemap_init(Tilemap *tm) {
    memset(tm->tiles, 0, sizeof(tm->tiles)); /* All TILE_EMPTY (0) */
    tm->cols = WORLD_COLS;
    tm->rows = WORLD_ROWS;
    tm->tile_size = TILE_SIZE;
}

void tilemap_generate(Tilemap *tm, int spawn_tile_x, int spawn_tile_y, unsigned int seed) {
    tilemap_init(tm);
    seed_rng(seed);

    /* ── Border walls ─────────────────────────────────────────────────── */
    for (int x = 0; x < tm->cols; x++) {
        tm->tiles[0][x] = TILE_WALL;
        tm->tiles[tm->rows - 1][x] = TILE_WALL;
    }
    for (int y = 0; y < tm->rows; y++) {
        tm->tiles[y][0] = TILE_WALL;
        tm->tiles[y][tm->cols - 1] = TILE_WALL;
    }

    /* ── Scatter random obstacles in the interior ─────────────────────── */
    for (int y = 2; y < tm->rows - 2; y++) {
        for (int x = 2; x < tm->cols - 2; x++) {
            if (rand_range(0, 99) < OBSTACLE_DENSITY) {
                tm->tiles[y][x] = TILE_WALL;
            }
        }
    }

    /* ── Place obstacle clusters (small rectangular patches) ──────────── */
    int num_clusters = rand_range(15, 30);
    for (int i = 0; i < num_clusters; i++) {
        int cx = rand_range(3, tm->cols - 4);
        int cy = rand_range(3, tm->rows - 4);
        int cw = rand_range(2, 5);
        int ch = rand_range(2, 4);

        for (int dy = 0; dy < ch; dy++) {
            for (int dx = 0; dx < cw; dx++) {
                int tx = cx + dx;
                int ty = cy + dy;
                if (tx > 0 && tx < tm->cols - 1 && ty > 0 && ty < tm->rows - 1) {
                    tm->tiles[ty][tx] = TILE_WALL;
                }
            }
        }
    }

    /* ── Clear spawn area (circular) ──────────────────────────────────── */
    int r2 = SPAWN_CLEAR_RADIUS * SPAWN_CLEAR_RADIUS;
    for (int dy = -SPAWN_CLEAR_RADIUS; dy <= SPAWN_CLEAR_RADIUS; dy++) {
        for (int dx = -SPAWN_CLEAR_RADIUS; dx <= SPAWN_CLEAR_RADIUS; dx++) {
            if (dx * dx + dy * dy <= r2) {
                int tx = spawn_tile_x + dx;
                int ty = spawn_tile_y + dy;
                if (tx >= 0 && tx < tm->cols && ty >= 0 && ty < tm->rows) {
                    tm->tiles[ty][tx] = TILE_EMPTY;
                }
            }
        }
    }
}

bool tilemap_is_solid(const Tilemap *tm, float world_x, float world_y) {
    int tx, ty;
    tilemap_world_to_tile(tm, world_x, world_y, &tx, &ty);
    return tilemap_is_tile_solid(tm, tx, ty);
}

bool tilemap_is_tile_solid(const Tilemap *tm, int tile_x, int tile_y) {
    if (tile_x < 0 || tile_x >= tm->cols || tile_y < 0 || tile_y >= tm->rows) {
        return true; /* Out of bounds counts as solid */
    }
    return tm->tiles[tile_y][tile_x] == TILE_WALL;
}

Rectangle tilemap_get_world_bounds(const Tilemap *tm) {
    return (Rectangle){0.0f, 0.0f, (float)(tm->cols * tm->tile_size),
                       (float)(tm->rows * tm->tile_size)};
}

void tilemap_world_to_tile(const Tilemap *tm, float world_x, float world_y, int *tile_x,
                           int *tile_y) {
    *tile_x = (int)floorf(world_x / (float)tm->tile_size);
    *tile_y = (int)floorf(world_y / (float)tm->tile_size);
}

Vector2 tilemap_tile_to_world(const Tilemap *tm, int tile_x, int tile_y) {
    return (Vector2){(float)(tile_x * tm->tile_size), (float)(tile_y * tm->tile_size)};
}

void tilemap_draw(const Tilemap *tm, Camera2D camera) {
    /* Determine visible tile range from camera viewport */
    float screen_w = (float)GetScreenWidth();
    float screen_h = (float)GetScreenHeight();

    /* Top-left world coordinate visible on screen */
    float view_x = camera.target.x - camera.offset.x / camera.zoom;
    float view_y = camera.target.y - camera.offset.y / camera.zoom;
    float view_w = screen_w / camera.zoom;
    float view_h = screen_h / camera.zoom;

    int start_x, start_y, end_x, end_y;
    tilemap_world_to_tile(tm, view_x, view_y, &start_x, &start_y);

    /* Add 1 tile margin to avoid pop-in at edges */
    start_x -= 1;
    start_y -= 1;
    tilemap_world_to_tile(tm, view_x + view_w, view_y + view_h, &end_x, &end_y);
    end_x += 2;
    end_y += 2;

    /* Clamp to grid bounds */
    if (start_x < 0) {
        start_x = 0;
    }
    if (start_y < 0) {
        start_y = 0;
    }
    if (end_x > tm->cols) {
        end_x = tm->cols;
    }
    if (end_y > tm->rows) {
        end_y = tm->rows;
    }

    int ts = tm->tile_size;

    for (int y = start_y; y < end_y; y++) {
        for (int x = start_x; x < end_x; x++) {
            int px = x * ts;
            int py = y * ts;

            if (tm->tiles[y][x] == TILE_WALL) {
                /* Walls: dark steel with subtle edge highlight */
                Color wall_fill = (Color){40, 45, 55, 255};
                DrawRectangle(px, py, ts, ts, wall_fill);

                /* Subtle inner border for depth */
                Color wall_edge = (Color){60, 70, 85, 255};
                DrawRectangleLines(px, py, ts, ts, wall_edge);
            } else {
                /* Floor: very dark with subtle grid lines */
                Color floor_fill = (Color){12, 14, 18, 255};
                DrawRectangle(px, py, ts, ts, floor_fill);

                /* Subtle grid dot at tile corner for spatial reference */
                Color grid_dot = (Color){25, 30, 40, 255};
                DrawPixel(px, py, grid_dot);
            }
        }
    }
}
