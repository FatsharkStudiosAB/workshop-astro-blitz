/*
 * weapon.c -- Weapon preset definitions
 */

#include "weapon.h"

/* ── Preset table ──────────────────────────────────────────────────────────── */

static const Weapon weapon_presets[WEAPON_COUNT] = {
    [WEAPON_PISTOL] =
        {
            .type = WEAPON_PISTOL,
            .fire_rate = 0.25f,
            .damage = 1.0f,
            .bullet_speed = 500.0f,
            .spread_angle = 0.0f,
            .projectile_count = 1,
            .bullet_lifetime = 2.0f,
            .bullet_color = {255, 165, 0, 255}, /* ORANGE */
            .name = "Pistol",
        },
    [WEAPON_SMG] =
        {
            .type = WEAPON_SMG,
            .fire_rate = 0.08f,
            .damage = 0.5f,
            .bullet_speed = 550.0f,
            .spread_angle = 12.0f,
            .projectile_count = 1,
            .bullet_lifetime = 1.5f,
            .bullet_color = {255, 255, 100, 255}, /* bright yellow */
            .name = "SMG",
        },
    [WEAPON_SHOTGUN] =
        {
            .type = WEAPON_SHOTGUN,
            .fire_rate = 0.6f,
            .damage = 1.0f,
            .bullet_speed = 400.0f,
            .spread_angle = 30.0f,
            .projectile_count = 5,
            .bullet_lifetime = 0.8f,
            .bullet_color = {255, 100, 50, 255}, /* reddish orange */
            .name = "Shotgun",
        },
    [WEAPON_PLASMA] =
        {
            .type = WEAPON_PLASMA,
            .fire_rate = 0.4f,
            .damage = 3.0f,
            .bullet_speed = 350.0f,
            .spread_angle = 0.0f,
            .projectile_count = 1,
            .bullet_lifetime = 3.0f,
            .bullet_color = {100, 200, 255, 255}, /* cyan-ish */
            .name = "Plasma",
        },
};

/* ── Public ────────────────────────────────────────────────────────────────── */

Weapon weapon_get_preset(WeaponType type) {
    if (type < 0 || type >= WEAPON_COUNT) {
        return weapon_presets[WEAPON_PISTOL];
    }
    return weapon_presets[type];
}

Weapon weapon_get_default(void) {
    return weapon_presets[WEAPON_PISTOL];
}

const char *weapon_get_description(WeaponType type) {
    switch (type) {
    case WEAPON_PISTOL:
        return "Reliable sidearm. Balanced damage and accuracy.";
    case WEAPON_SMG:
        return "Rapid-fire spray. Low damage, high volume.";
    case WEAPON_SHOTGUN:
        return "5-pellet burst. Devastating at close range.";
    case WEAPON_PLASMA:
        return "Slow, heavy bolts. Maximum damage per shot.";
    default:
        return "Unknown weapon.";
    }
}
