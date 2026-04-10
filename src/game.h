/*
 * game.h -- Top-level game state and interface
 */

#pragma once

#include "audio.h"
#include "bullet.h"
#include "enemy.h"
#include "particle.h"
#include "player.h"
#include "raylib.h"
#include "screenshake.h"
#include "settings.h"
#include "tilemap.h"
#include "weapon.h"

/* ── Constants ─────────────────────────────────────────────────────────────── */

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

/* ── Types ─────────────────────────────────────────────────────────────────── */

/* Game phase -- controls which branch of update/draw runs */
typedef enum {
    PHASE_FIRST_RUN, /* first-launch movement picker */
    PHASE_MAIN_MENU,
    PHASE_PLAYING,
    PHASE_PAUSED,
    PHASE_SETTINGS,
    PHASE_GAME_OVER
} GamePhase;

/* Maximum number of entries in a menu list */
#define MAX_MENU_ITEMS 5

/* ── Damage numbers ───────────────────────────────────────────────────────── */

#define MAX_DAMAGE_NUMBERS 32
#define DAMAGE_NUMBER_LIFETIME 0.8f
#define DAMAGE_NUMBER_RISE_SPEED 60.0f

typedef struct {
    Vector2 position;
    float lifetime;
    float max_lifetime;
    int value;
    Color color;
    bool active;
} DamageNumber;

typedef struct {
    DamageNumber numbers[MAX_DAMAGE_NUMBERS];
} DamageNumberPool;

/* ── Weapon pickups ────────────────────────────────────────────────────────── */

#define MAX_WEAPON_PICKUPS 16
#define WEAPON_PICKUP_RADIUS 10.0f
#define WEAPON_PICKUP_LIFETIME 15.0f /* seconds before despawn */
#define WEAPON_DROP_CHANCE 30        /* percent chance to drop on kill (non-swarmer) */

typedef struct {
    Vector2 position;
    Weapon weapon;
    float lifetime;
    bool active;
} WeaponPickup;

typedef struct {
    WeaponPickup pickups[MAX_WEAPON_PICKUPS];
} WeaponPickupPool;

/* Run statistics shown on the game-over screen */
typedef struct {
    int kills;           /* total enemies killed this run */
    float survival_time; /* seconds survived */
    int waves_spawned;   /* number of enemy waves spawned */
} GameStats;

typedef struct {
    Player player;
    BulletPool bullets;
    EnemyPool enemies;
    EnemyBulletPool enemy_bullets;
    ParticlePool particles;
    DamageNumberPool damage_numbers;
    WeaponPickupPool weapon_pickups;
    Tilemap tilemap;
    GameAudio audio;
    Camera2D camera;
    ScreenShake shake;
    Rectangle arena;   /* world bounds (derived from tilemap) */
    float spawn_timer; /* time until next enemy wave */
    GamePhase phase;
    GamePhase settings_return_phase; /* phase to return to after settings */
    int menu_cursor;                 /* currently highlighted menu item */
    GameStats stats;
    Settings settings;
    bool should_quit; /* set by menu "Quit" action */
} GameState;

/* ── Public API ────────────────────────────────────────────────────────────── */

/*
 * game_init -- Reset all gameplay state for a new run.
 *
 * Regenerates the world, resets the player, bullets, enemies, stats, and
 * camera. Preserves user settings and audio state across restarts.
 * Sets phase to PHASE_PLAYING (caller may override afterward).
 */
void game_init(GameState *gs);

/* Advance one frame: dispatch to the current phase's update logic. */
void game_update(GameState *gs);

/* Draw the current frame: dispatch to the current phase's draw logic. */
void game_draw(const GameState *gs);

/* Transition to PHASE_GAME_OVER if the player is dead. Separated from
 * game_update so it can be unit-tested without a Raylib window context. */
void game_check_death(GameState *gs);

/* Return true if the game should exit (user chose Quit from menu) */
bool game_should_quit(const GameState *gs);
