/*
 * weapon.h -- Weapon definitions and presets
 *
 * Each weapon type defines fire rate, damage, bullet speed, spread, and
 * projectile count. The player holds a current_weapon that drives bullet
 * firing behavior.
 */

#pragma once

#include "raylib.h"

/* ── Types ─────────────────────────────────────────────────────────────────── */

typedef enum {
    WEAPON_PISTOL,
    WEAPON_SMG,
    WEAPON_SHOTGUN,
    WEAPON_PLASMA,
    WEAPON_COUNT /* sentinel -- total number of weapon types */
} WeaponType;

typedef struct {
    WeaponType type;
    float fire_rate;       /* seconds between shots */
    float damage;          /* damage per bullet */
    float bullet_speed;    /* pixels per second */
    float spread_angle;    /* total cone spread in degrees (0 = perfectly accurate) */
    int projectile_count;  /* bullets per shot */
    float bullet_lifetime; /* seconds before bullet despawns */
    Color bullet_color;    /* visual color for bullets */
    const char *name;      /* display name for HUD */
} Weapon;

/* ── Public API ────────────────────────────────────────────────────────────── */

/* Get the preset configuration for a weapon type. */
Weapon weapon_get_preset(WeaponType type);

/* Get the default starting weapon (Pistol). */
Weapon weapon_get_default(void);
