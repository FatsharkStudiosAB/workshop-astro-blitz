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
#include "upgrade.h"
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

/* ── Upgrade pickups ───────────────────────────────────────────────────────── */

#define MAX_UPGRADE_PICKUPS 8
#define UPGRADE_PICKUP_RADIUS 8.0f
#define UPGRADE_PICKUP_LIFETIME 12.0f
#define ELITE_UPGRADE_DROP_CHANCE 50 /* percent chance elite drops an upgrade */

typedef struct {
    Vector2 position;
    UpgradeType type;
    float lifetime;
    bool active;
} UpgradePickup;

typedef struct {
    UpgradePickup pickups[MAX_UPGRADE_PICKUPS];
} UpgradePickupPool;

/* ── Combo system ──────────────────────────────────────────────────────────── */

#define COMBO_TIMEOUT 2.0f          /* seconds before combo resets */
#define COMBO_DISPLAY_DURATION 1.5f /* how long combo text stays visible */

typedef struct {
    int count;           /* current combo count (0 = no combo) */
    float timer;         /* time remaining before combo resets */
    float display_timer; /* time remaining for combo text display */
    int best;            /* best combo this run */
} ComboState;

/* ── Juice constants ───────────────────────────────────────────────────────── */

#define HITSTOP_HIT (3.0f / 60.0f)    /* 3 frames at 60fps on enemy hit */
#define HITSTOP_KILL (5.0f / 60.0f)   /* 5 frames on enemy kill */
#define HITSTOP_MELEE (4.0f / 60.0f)  /* 4 frames on melee hit */
#define HITSTOP_PLAYER (4.0f / 60.0f) /* 4 frames when player takes damage */
#define SLOWMO_DURATION 0.12f         /* seconds of slow-motion after a kill */
#define SLOWMO_SCALE 0.25f            /* time scale during slow-motion */
#define CAMERA_KICK_STRENGTH 4.0f     /* pixels of camera kick per shot */
#define CAMERA_KICK_DECAY 12.0f       /* kick decay speed (per second) */
#define KNOCKBACK_BULLET 120.0f       /* knockback force from bullet hits */

/* ── Floor progression ─────────────────────────────────────────────────────── */

#define WAVES_PER_FLOOR 5          /* waves before exit spawns */
#define EXIT_RADIUS 16.0f          /* exit tile collision radius */
#define FLOOR_ENEMY_HP_SCALE 0.15f /* +15% enemy HP per floor */

/* Run statistics shown on the game-over screen */
typedef struct {
    int kills;           /* total enemies killed this run */
    float survival_time; /* seconds survived */
    int waves_spawned;   /* number of enemy waves spawned */
} GameStats;

/* ── Enemy corpses (permanence -- dead enemies fade slowly) ────────────────── */

#define MAX_CORPSES 32
#define CORPSE_LIFETIME 3.0f /* seconds to fully fade out */

typedef struct {
    Vector2 position;
    float radius;
    float lifetime; /* remaining lifetime (fades alpha as it decreases) */
    Color color;    /* base color of the dead enemy */
    bool active;
} Corpse;

typedef struct {
    Corpse corpses[MAX_CORPSES];
} CorpsePool;

/* ── Ambient particles (cosmetic dust motes) ───────────────────────────────── */

#define MAX_AMBIENT_PARTICLES 64
#define AMBIENT_PARTICLE_RADIUS 1.5f

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float alpha;       /* current opacity (0-1) */
    float alpha_speed; /* fade in/out rate */
    bool active;
} AmbientParticle;

typedef struct {
    Player player;
    BulletPool bullets;
    EnemyPool enemies;
    EnemyBulletPool enemy_bullets;
    ParticlePool particles;
    CorpsePool corpses;
    DamageNumberPool damage_numbers;
    WeaponPickupPool weapon_pickups;
    UpgradePickupPool upgrade_pickups;
    UpgradeState upgrades;
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
    ComboState combo;
    int floor;                                      /* current floor number (1-based) */
    int floor_waves;                                /* waves spawned on current floor */
    Vector2 exit_position;                          /* position of exit tile */
    bool exit_active;                               /* true when exit tile is spawned */
    AmbientParticle ambient[MAX_AMBIENT_PARTICLES]; /* cosmetic floating dust */

    /* ── Juice state ───────────────────────────────────────────────────── */
    float hitstop_timer; /* frames remaining of hitstop freeze (in seconds) */
    float slowmo_timer;  /* seconds remaining of slow-motion after kills */
    float slowmo_scale;  /* time scale during slowmo (e.g. 0.3) */
    Vector2 camera_kick; /* directional camera offset from shooting, decays */

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

/* Draw the world layer (tilemap, entities, particles, effects).
 * Call between postfx_begin and lightmap_render. */
void game_draw_world(const GameState *gs);

/* Draw the UI layer (HUD, menus, overlays).
 * Call after lightmap_render so UI is not darkened by lighting. */
void game_draw_ui(const GameState *gs);

/* Transition to PHASE_GAME_OVER if the player is dead. Separated from
 * game_update so it can be unit-tested without a Raylib window context. */
void game_check_death(GameState *gs);

/* Return true if the game should exit (user chose Quit from menu) */
bool game_should_quit(const GameState *gs);
