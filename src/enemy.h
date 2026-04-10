/*
 * enemy.h -- Enemy types, pool, and interface
 *
 * Enemies are managed via a fixed-size object pool (same pattern as bullets).
 * Each enemy has a type that determines its behavior, speed, and HP.
 */

#pragma once

#include "raylib.h"
#include "raymath.h"
#include <stdbool.h>

/* ── Constants ─────────────────────────────────────────────────────────────── */

#define MAX_ENEMIES 64

/* Swarmer constants */
#define SWARMER_SPEED 150.0f
#define SWARMER_HP 1.0f
#define SWARMER_RADIUS 8.0f
#define SWARMER_DAMAGE 10.0f /* damage dealt to player on contact */

/* Spawning */
#define SPAWN_INTERVAL 3.0f /* seconds between spawn waves */
#define SPAWN_MIN_GROUP 4   /* minimum swarmers per wave */
#define SPAWN_MAX_GROUP 8   /* maximum swarmers per wave */

/* ── Types ─────────────────────────────────────────────────────────────────── */

typedef enum { ENEMY_SWARMER } EnemyType;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float hp;
    float radius;
    float speed;
    float damage;
    EnemyType type;
    bool active;
} Enemy;

typedef struct {
    Enemy enemies[MAX_ENEMIES];
} EnemyPool;

/* ── Public API ────────────────────────────────────────────────────────────── */

/* Initialize all enemies to inactive. */
void enemy_pool_init(EnemyPool *pool);

/* Spawn a single enemy of the given type at the given position. */
void enemy_pool_spawn(EnemyPool *pool, EnemyType type, Vector2 position);

/* Update all active enemies: move toward target, clamp to arena. */
void enemy_pool_update(EnemyPool *pool, float dt, Vector2 target, Rectangle arena);

/* Draw all active enemies. */
void enemy_pool_draw(const EnemyPool *pool);

/* Count the number of currently active enemies. */
int enemy_pool_active_count(const EnemyPool *pool);

/* ── Collision helpers ─────────────────────────────────────────────────────── */

/* Check if two circles overlap (generic circle-circle collision). */
bool check_circle_collision(Vector2 pos_a, float radius_a, Vector2 pos_b, float radius_b);
