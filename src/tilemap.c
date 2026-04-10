/*
 * tilemap.c -- World grid, procedural generation, and tile collision
 */

#include "tilemap.h"
#include <math.h>
#include <string.h>

/* ── Helpers ───────────────────────────────────────────────────────────────── */

/* Tilemap-local PRNG state.
 * This avoids mutating the global C RNG state used by srand()/rand(),
 * which may also be used by Raylib on some platforms. */
static unsigned int tilemap_rng_state = 2463534242u;

/* Simple seeded random for procedural level generation.
 * Not cryptographic; deterministic for a given seed. */
static void seed_rng(unsigned int seed) {
    if (seed == 0) {
        /* Use a Raylib-generated seed, but do not reseed the global C RNG. */
        seed = (unsigned int)GetRandomValue(1, 2147483647);
    }

    /* xorshift32 must not be seeded with 0. */
    if (seed == 0) {
        seed = 2463534242u;
    }

    tilemap_rng_state = seed;
}

static unsigned int next_random_u32(void) {
    unsigned int x = tilemap_rng_state;

    /* xorshift32 */
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    tilemap_rng_state = x;
    return x;
}

static int rand_range(int min, int max) {
    unsigned int span;

    if (min >= max) {
        return min;
    }

    span = (unsigned int)(max - min + 1);
    return min + (int)(next_random_u32() % span);
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

/* ── Flow field (BFS) ─────────────────────────────────────────────────────── */

/* BFS queue entry: tile coordinates packed into a single int for compactness */
typedef struct {
    short x;
    short y;
} TileCoord;

/* 4-directional BFS neighbors */
static const int dx4[4] = {1, -1, 0, 0};
static const int dy4[4] = {0, 0, 1, -1};

void tilemap_compute_flow_field(Tilemap *tm, float target_x, float target_y) {
    /* Convert target world position to tile coordinates */
    int goal_tx, goal_ty;
    tilemap_world_to_tile(tm, target_x, target_y, &goal_tx, &goal_ty);

    /* Clamp goal to valid range */
    if (goal_tx < 0) {
        goal_tx = 0;
    }
    if (goal_tx >= tm->cols) {
        goal_tx = tm->cols - 1;
    }
    if (goal_ty < 0) {
        goal_ty = 0;
    }
    if (goal_ty >= tm->rows) {
        goal_ty = tm->rows - 1;
    }

    /* Initialize all distances to unreachable, all flow to zero */
    for (int y = 0; y < tm->rows; y++) {
        for (int x = 0; x < tm->cols; x++) {
            tm->flow_dist[y][x] = FLOW_UNREACHABLE;
            tm->flow[y][x] = (Vector2){0.0f, 0.0f};
        }
    }

    /* BFS queue (static to avoid stack overflow -- 128*96 * 4 bytes = ~48 KB) */
    static TileCoord queue[WORLD_ROWS * WORLD_COLS];
    int head = 0;
    int tail = 0;

    /* Seed BFS from the goal tile (even if it's solid -- enemies near the
     * player's wall tile should still try to approach) */
    tm->flow_dist[goal_ty][goal_tx] = 0;
    queue[tail++] = (TileCoord){(short)goal_tx, (short)goal_ty};

    /* BFS expansion */
    while (head < tail) {
        TileCoord cur = queue[head++];
        int cur_dist = tm->flow_dist[cur.y][cur.x];

        for (int d = 0; d < 4; d++) {
            int nx = cur.x + dx4[d];
            int ny = cur.y + dy4[d];

            /* Skip out-of-bounds */
            if (nx < 0 || nx >= tm->cols || ny < 0 || ny >= tm->rows) {
                continue;
            }

            /* Skip already visited */
            if (tm->flow_dist[ny][nx] != FLOW_UNREACHABLE) {
                continue;
            }

            /* Skip walls */
            if (tm->tiles[ny][nx] == TILE_WALL) {
                continue;
            }

            tm->flow_dist[ny][nx] = cur_dist + 1;
            queue[tail++] = (TileCoord){(short)nx, (short)ny};
        }
    }

    /* Compute flow directions: each tile points toward its lowest-distance
     * walkable neighbor (i.e., one step closer to the goal). */
    for (int y = 0; y < tm->rows; y++) {
        for (int x = 0; x < tm->cols; x++) {
            if (tm->flow_dist[y][x] == FLOW_UNREACHABLE || tm->flow_dist[y][x] == 0) {
                /* Walls/unreachable tiles and the goal tile get zero flow */
                continue;
            }

            int best_dist = tm->flow_dist[y][x];
            int best_dx = 0;
            int best_dy = 0;

            for (int d = 0; d < 4; d++) {
                int nx = x + dx4[d];
                int ny = y + dy4[d];

                if (nx < 0 || nx >= tm->cols || ny < 0 || ny >= tm->rows) {
                    continue;
                }

                int nd = tm->flow_dist[ny][nx];
                if (nd != FLOW_UNREACHABLE && nd < best_dist) {
                    best_dist = nd;
                    best_dx = dx4[d];
                    best_dy = dy4[d];
                }
            }

            /* Store unit direction toward the best neighbor */
            if (best_dx != 0 || best_dy != 0) {
                tm->flow[y][x] = (Vector2){(float)best_dx, (float)best_dy};
            }
        }
    }
}

void tilemap_draw(const Tilemap *tm, Camera2D camera, float time) {
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

    /* Animated pulse for grid lines (slow wave across floor) */
    float wave_speed = 0.8f;
    float wave_scale = 0.03f; /* spatial frequency */

    for (int y = start_y; y < end_y; y++) {
        for (int x = start_x; x < end_x; x++) {
            int px = x * ts;
            int py = y * ts;

            if (tm->tiles[y][x] == TILE_WALL) {
                /* ── Walls: layered steel look ────────────────────── */
                Color wall_fill = (Color){35, 40, 52, 255};
                DrawRectangle(px, py, ts, ts, wall_fill);

                /* Top/left highlight (light source from top-left) */
                DrawLineEx((Vector2){(float)px, (float)py}, (Vector2){(float)(px + ts), (float)py},
                           1.0f, (Color){70, 80, 100, 255});
                DrawLineEx((Vector2){(float)px, (float)py}, (Vector2){(float)px, (float)(py + ts)},
                           1.0f, (Color){65, 75, 95, 255});

                /* Bottom/right shadow */
                DrawLineEx((Vector2){(float)px, (float)(py + ts - 1)},
                           (Vector2){(float)(px + ts), (float)(py + ts - 1)}, 1.0f,
                           (Color){20, 22, 30, 255});
                DrawLineEx((Vector2){(float)(px + ts - 1), (float)py},
                           (Vector2){(float)(px + ts - 1), (float)(py + ts)}, 1.0f,
                           (Color){22, 25, 32, 255});

                /* Center rivet dot for detail */
                DrawPixel(px + ts / 2, py + ts / 2, (Color){55, 62, 78, 255});
            } else {
                /* ── Floor: dark with animated grid lines ─────────── */
                Color floor_fill = (Color){10, 12, 16, 255};
                DrawRectangle(px, py, ts, ts, floor_fill);

                /* Animated wave: brightness pulses across the floor */
                float wave_val = sinf((float)x * wave_scale * 40.0f +
                                      (float)y * wave_scale * 40.0f + time * wave_speed);
                /* Map -1..1 to 0..1, then use as alpha boost */
                float pulse = 0.5f + 0.5f * wave_val;
                unsigned char grid_alpha = (unsigned char)(20.0f + 25.0f * pulse);

                /* Grid lines on tile edges (thin horizontal + vertical) */
                Color grid_h = (Color){0, 180, 160, grid_alpha};
                Color grid_v = (Color){0, 160, 180, grid_alpha};

                /* Bottom edge of tile */
                DrawLineEx((Vector2){(float)px, (float)(py + ts)},
                           (Vector2){(float)(px + ts), (float)(py + ts)}, 1.0f, grid_h);
                /* Right edge of tile */
                DrawLineEx((Vector2){(float)(px + ts), (float)py},
                           (Vector2){(float)(px + ts), (float)(py + ts)}, 1.0f, grid_v);

                /* Brighter intersection dot at tile corners */
                unsigned char dot_alpha = (unsigned char)(30.0f + 30.0f * pulse);
                DrawPixel(px + ts, py + ts, (Color){0, 220, 200, dot_alpha});
            }
        }
    }
}
