/*
 * upgrade.h -- Passive upgrade system
 *
 * Six stackable upgrade types that modify player stats. Each stack level
 * adds a fixed bonus. Upgrades drop from elite enemies and are picked up
 * on contact.
 */

#pragma once

#include "raylib.h"
#include <stdbool.h>

/* ── Constants ─────────────────────────────────────────────────────────────── */

#define MAX_UPGRADE_STACKS 5 /* max stacks per upgrade type */

/* Per-stack bonuses */
#define UPGRADE_SPEED_BONUS 30.0f        /* +30 move speed per stack */
#define UPGRADE_DAMAGE_BONUS 0.5f        /* +0.5 bullet damage per stack */
#define UPGRADE_FIRE_RATE_BONUS 0.85f    /* multiply fire rate by 0.85 per stack (faster) */
#define UPGRADE_MAX_HP_BONUS 20.0f       /* +20 max HP per stack */
#define UPGRADE_BULLET_SPEED_BONUS 0.15f /* +15% bullet speed per stack */
#define UPGRADE_DASH_CD_BONUS 0.85f      /* multiply dash CD by 0.85 per stack (shorter) */

/* ── Types ─────────────────────────────────────────────────────────────────── */

typedef enum {
    UPGRADE_SPEED,
    UPGRADE_DAMAGE,
    UPGRADE_FIRE_RATE,
    UPGRADE_MAX_HP,
    UPGRADE_BULLET_SPEED,
    UPGRADE_DASH_CD,
    UPGRADE_COUNT /* sentinel */
} UpgradeType;

typedef struct {
    int stacks[UPGRADE_COUNT]; /* current stack count per upgrade type */
} UpgradeState;

/* ── Public API ────────────────────────────────────────────────────────────── */

/* Initialize all upgrade stacks to zero. */
void upgrade_state_init(UpgradeState *us);

/* Add one stack of the given upgrade type. Returns false if already at max. */
bool upgrade_add(UpgradeState *us, UpgradeType type);

/* Get the current speed bonus (additive). */
float upgrade_get_speed_bonus(const UpgradeState *us);

/* Get the current damage bonus (additive). */
float upgrade_get_damage_bonus(const UpgradeState *us);

/* Get the fire rate multiplier (< 1 = faster). */
float upgrade_get_fire_rate_mult(const UpgradeState *us);

/* Get the max HP bonus (additive). */
float upgrade_get_max_hp_bonus(const UpgradeState *us);

/* Get the bullet speed multiplier (> 1 = faster). */
float upgrade_get_bullet_speed_mult(const UpgradeState *us);

/* Get the dash cooldown multiplier (< 1 = shorter). */
float upgrade_get_dash_cd_mult(const UpgradeState *us);

/* Get a display name for an upgrade type. */
const char *upgrade_get_name(UpgradeType type);

/* Get a display color for an upgrade type. */
Color upgrade_get_color(UpgradeType type);
