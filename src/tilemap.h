/*
 * tilemap.h -- World grid, procedural generation, and tile collision
 *
 * The tilemap represents the game world as a 2D grid of tiles. Each tile is
 * either empty floor or a solid wall (obstacle). The world is larger than the
 * screen viewport -- a Camera2D follows the player to show the visible portion.
 *
 * Generation produces a bordered arena with randomly scattered obstacle
 * clusters. A clear zone around the spawn point ensures the player never
 * starts inside a wall.
 */

#pragma once

#include "raylib.h"
#include <stdbool.h>

/* ── Constants ─────────────────────────────────────────────────────────────── */

#define TILE_SIZE 32

/* World dimensions in tiles */
#define WORLD_COLS 128
#define WORLD_ROWS 96

/* World dimensions in pixels (derived) */
#define WORLD_WIDTH (TILE_SIZE * WORLD_COLS)
#define WORLD_HEIGHT (TILE_SIZE * WORLD_ROWS)

/* Radius (in tiles) around spawn point kept clear of obstacles */
#define SPAWN_CLEAR_RADIUS 5

/* Obstacle density: probability (0-100) that an interior tile becomes a wall */
#define OBSTACLE_DENSITY 8

/* Flow field: distance value meaning "unreachable" (no path to target) */
#define FLOW_UNREACHABLE (-1)

/* ── Types ─────────────────────────────────────────────────────────────────── */

typedef enum {
    TILE_EMPTY, /* walkable floor */
    TILE_WALL   /* solid obstacle */
} TileType;

typedef struct Tilemap {
    TileType tiles[WORLD_ROWS][WORLD_COLS];
    Vector2 flow[WORLD_ROWS][WORLD_COLS];  /* BFS flow field: direction toward target */
    int flow_dist[WORLD_ROWS][WORLD_COLS]; /* BFS distance (-1 = unreachable/wall) */
    int cols;
    int rows;
    int tile_size;
} Tilemap;

/* ── Public API ────────────────────────────────────────────────────────────── */

/*
 * tilemap_init -- Initialize the tilemap with default dimensions.
 * All tiles are set to TILE_EMPTY.
 */
void tilemap_init(Tilemap *tm);

/*
 * tilemap_generate -- Procedurally generate terrain.
 *
 * Fills the tilemap with:
 *   - Solid border walls on all edges.
 *   - Random obstacle clusters in the interior (density controlled by
 *     OBSTACLE_DENSITY).
 *   - A guaranteed clear circular area around spawn_tile_x/spawn_tile_y
 *     with radius SPAWN_CLEAR_RADIUS tiles.
 *
 * Uses the provided seed for reproducibility. Pass 0 for a random seed.
 */
void tilemap_generate(Tilemap *tm, int spawn_tile_x, int spawn_tile_y, unsigned int seed);

/*
 * tilemap_is_solid -- Check whether a world-pixel coordinate is blocked.
 *
 * Returns true if the tile at the given world position is TILE_WALL,
 * or if the position is outside the world bounds.
 */
bool tilemap_is_solid(const Tilemap *tm, float world_x, float world_y);

/*
 * tilemap_is_tile_solid -- Check whether a tile coordinate is blocked.
 *
 * Returns true if the tile is TILE_WALL or if the coordinates are out
 * of bounds.
 */
bool tilemap_is_tile_solid(const Tilemap *tm, int tile_x, int tile_y);

/*
 * tilemap_get_world_bounds -- Return a Rectangle covering the full world.
 */
Rectangle tilemap_get_world_bounds(const Tilemap *tm);

/*
 * tilemap_draw -- Draw visible tiles within the camera viewport.
 *
 * Only tiles overlapping the camera's visible area are drawn (frustum culled).
 * Call this between BeginMode2D and EndMode2D.
 */
void tilemap_draw(const Tilemap *tm, Camera2D camera);

/*
 * tilemap_world_to_tile -- Convert a world-pixel coordinate to tile indices.
 *
 * Useful for collision queries. Does NOT clamp -- caller must check bounds.
 */
void tilemap_world_to_tile(const Tilemap *tm, float world_x, float world_y, int *tile_x,
                           int *tile_y);

/*
 * tilemap_tile_to_world -- Convert tile indices to world-pixel coordinates.
 *
 * Returns the top-left corner of the tile in world space.
 */
Vector2 tilemap_tile_to_world(const Tilemap *tm, int tile_x, int tile_y);

/*
 * tilemap_compute_flow_field -- Compute a BFS flow field toward a target.
 *
 * Runs a breadth-first search from the tile containing (target_x, target_y)
 * outward through all walkable tiles. After this call:
 *   - tm->flow[y][x] contains a unit direction vector pointing toward the
 *     next tile on the shortest path to the target.
 *   - tm->flow_dist[y][x] contains the BFS distance (in tiles) from that
 *     tile to the target, or FLOW_UNREACHABLE for walls/unreachable tiles.
 *
 * Call once per frame (before enemy update) with the player's world position.
 */
void tilemap_compute_flow_field(Tilemap *tm, float target_x, float target_y);
