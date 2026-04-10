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

/* Forward declaration to avoid circular include */
typedef struct Tilemap Tilemap;

/* ── Constants ─────────────────────────────────────────────────────────────── */

#define MAX_ENEMIES 64
#define MAX_ENEMY_BULLETS 64

/* Swarmer constants */
#define SWARMER_SPEED 150.0f
#define SWARMER_HP 1.0f
#define SWARMER_RADIUS 8.0f
#define SWARMER_DAMAGE 10.0f /* damage dealt to player on contact */

/* Grunt constants */
#define GRUNT_SPEED 80.0f
#define GRUNT_HP 3.0f
#define GRUNT_RADIUS 10.0f
#define GRUNT_DAMAGE 5.0f /* contact damage */
#define GRUNT_SHOOT_COOLDOWN 2.0f
#define GRUNT_BULLET_SPEED 250.0f
#define GRUNT_BULLET_DAMAGE 8.0f
#define GRUNT_PREFERRED_RANGE 200.0f /* tries to stay at this distance */

/* Stalker constants */
#define STALKER_SPEED 200.0f
#define STALKER_HP 2.0f
#define STALKER_RADIUS 7.0f
#define STALKER_DAMAGE 15.0f
#define STALKER_CIRCLE_RANGE 120.0f /* distance to circle at */
#define STALKER_DASH_SPEED 500.0f
#define STALKER_DASH_INTERVAL 3.0f /* seconds between dashes */
#define STALKER_DASH_DURATION 0.2f

/* Bomber constants */
#define BOMBER_SPEED 100.0f
#define BOMBER_HP 4.0f
#define BOMBER_RADIUS 12.0f
#define BOMBER_DAMAGE 20.0f /* contact damage */
#define BOMBER_CHARGE_RANGE 150.0f
#define BOMBER_CHARGE_SPEED 350.0f
#define BOMBER_EXPLOSION_RADIUS 60.0f
#define BOMBER_EXPLOSION_DAMAGE 25.0f

/* Elite modifiers */
#define ELITE_CHANCE 20           /* percent chance per enemy after threshold */
#define ELITE_WAVE_THRESHOLD 5    /* waves before elites can appear */
#define ARMORED_HP_MULT 2.5f      /* HP multiplier */
#define ARMORED_SPEED_MULT 0.7f   /* speed multiplier (slower) */
#define SWIFT_SPEED_MULT 1.6f     /* speed multiplier (faster) */
#define SWIFT_RADIUS_MULT 0.75f   /* radius multiplier (smaller) */
#define BURNING_DAMAGE_BONUS 5.0f /* extra contact damage */

/* Spawning */
#define SPAWN_INTERVAL 3.0f /* seconds between spawn waves */
#define SPAWN_MIN_GROUP 2   /* minimum swarmers per wave */
#define SPAWN_MAX_GROUP 5   /* maximum swarmers per wave */

/* ── Types ─────────────────────────────────────────────────────────────────── */

typedef enum {
    ENEMY_SWARMER,
    ENEMY_GRUNT,
    ENEMY_STALKER,
    ENEMY_BOMBER,
    ENEMY_TYPE_COUNT /* sentinel */
} EnemyType;

typedef enum { ELITE_NONE = 0, ELITE_ARMORED, ELITE_SWIFT, ELITE_BURNING } EliteModifier;

/* Enemy bullet (fired by Grunts) */
typedef struct {
    Vector2 position;
    Vector2 velocity;
    float lifetime;
    float damage;
    bool active;
} EnemyBullet;

typedef struct {
    EnemyBullet bullets[MAX_ENEMY_BULLETS];
} EnemyBulletPool;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float hp;
    float max_hp; /* HP at spawn (for HP bar display) */
    float radius;
    float speed;
    float damage;
    float hit_flash;      /* seconds remaining of white flash (0 = no flash) */
    float shoot_cooldown; /* for Grunt: time until next shot */
    float ai_timer;       /* general-purpose AI timer (dash interval, charge, etc.) */
    bool is_charging;     /* Bomber: currently charging toward player */
    EnemyType type;
    EliteModifier elite; /* ELITE_NONE for regular enemies */
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

/* Spawn with an elite modifier applied. */
void enemy_pool_spawn_elite(EnemyPool *pool, EnemyType type, Vector2 position, EliteModifier elite);

/*
 * Update all active enemies: move toward target, clamp to arena.
 * Pass NULL for tm to skip wall collision checks (e.g. in tests).
 * Pass NULL for ebp to skip enemy shooting (e.g. in tests without bullet pool).
 */
void enemy_pool_update(EnemyPool *pool, float dt, Vector2 target, Rectangle arena,
                       const Tilemap *tm, EnemyBulletPool *ebp);

/* Draw all active enemies. */
void enemy_pool_draw(const EnemyPool *pool);

/* Count the number of currently active enemies. */
int enemy_pool_active_count(const EnemyPool *pool);

/* ── Enemy bullet pool API ─────────────────────────────────────────────────── */

/* Initialize all enemy bullets to inactive. */
void enemy_bullet_pool_init(EnemyBulletPool *pool);

/* Fire an enemy bullet from origin in direction. */
void enemy_bullet_pool_fire(EnemyBulletPool *pool, Vector2 origin, Vector2 direction, float speed,
                            float damage);

/* Update all active enemy bullets (movement + lifetime). */
void enemy_bullet_pool_update(EnemyBulletPool *pool, float dt, Rectangle arena);

/* Draw all active enemy bullets. */
void enemy_bullet_pool_draw(const EnemyBulletPool *pool);

/* ── Collision helpers ─────────────────────────────────────────────────────── */

/* Check if two circles overlap (generic circle-circle collision). */
bool check_circle_collision(Vector2 pos_a, float radius_a, Vector2 pos_b, float radius_b);
