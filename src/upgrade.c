/*
 * upgrade.c -- Passive upgrade system implementation
 */

#include "upgrade.h"
#include <math.h>
#include <string.h>

void upgrade_state_init(UpgradeState *us) {
    memset(us->stacks, 0, sizeof(us->stacks));
}

bool upgrade_add(UpgradeState *us, UpgradeType type) {
    if (type < 0 || type >= UPGRADE_COUNT) {
        return false;
    }
    if (us->stacks[type] >= MAX_UPGRADE_STACKS) {
        return false;
    }
    us->stacks[type]++;
    return true;
}

float upgrade_get_speed_bonus(const UpgradeState *us) {
    return (float)us->stacks[UPGRADE_SPEED] * UPGRADE_SPEED_BONUS;
}

float upgrade_get_damage_bonus(const UpgradeState *us) {
    return (float)us->stacks[UPGRADE_DAMAGE] * UPGRADE_DAMAGE_BONUS;
}

float upgrade_get_fire_rate_mult(const UpgradeState *us) {
    float mult = 1.0f;
    for (int i = 0; i < us->stacks[UPGRADE_FIRE_RATE]; i++) {
        mult *= UPGRADE_FIRE_RATE_BONUS;
    }
    return mult;
}

float upgrade_get_max_hp_bonus(const UpgradeState *us) {
    return (float)us->stacks[UPGRADE_MAX_HP] * UPGRADE_MAX_HP_BONUS;
}

float upgrade_get_bullet_speed_mult(const UpgradeState *us) {
    return 1.0f + (float)us->stacks[UPGRADE_BULLET_SPEED] * UPGRADE_BULLET_SPEED_BONUS;
}

float upgrade_get_dash_cd_mult(const UpgradeState *us) {
    float mult = 1.0f;
    for (int i = 0; i < us->stacks[UPGRADE_DASH_CD]; i++) {
        mult *= UPGRADE_DASH_CD_BONUS;
    }
    return mult;
}

const char *upgrade_get_name(UpgradeType type) {
    switch (type) {
    case UPGRADE_SPEED:
        return "SPD";
    case UPGRADE_DAMAGE:
        return "DMG";
    case UPGRADE_FIRE_RATE:
        return "ROF";
    case UPGRADE_MAX_HP:
        return "HP+";
    case UPGRADE_BULLET_SPEED:
        return "BSP";
    case UPGRADE_DASH_CD:
        return "DSH";
    default:
        return "???";
    }
}

Color upgrade_get_color(UpgradeType type) {
    switch (type) {
    case UPGRADE_SPEED:
        return (Color){100, 255, 100, 255}; /* green */
    case UPGRADE_DAMAGE:
        return (Color){255, 80, 80, 255}; /* red */
    case UPGRADE_FIRE_RATE:
        return (Color){255, 255, 80, 255}; /* yellow */
    case UPGRADE_MAX_HP:
        return (Color){255, 150, 200, 255}; /* pink */
    case UPGRADE_BULLET_SPEED:
        return (Color){100, 200, 255, 255}; /* cyan */
    case UPGRADE_DASH_CD:
        return (Color){150, 100, 255, 255}; /* purple */
    default:
        return (Color){200, 200, 200, 255};
    }
}

const char *upgrade_get_description(UpgradeType type) {
    switch (type) {
    case UPGRADE_SPEED:
        return "+30 movement speed per stack";
    case UPGRADE_DAMAGE:
        return "+0.5 bullet damage per stack";
    case UPGRADE_FIRE_RATE:
        return "Fire 15% faster per stack";
    case UPGRADE_MAX_HP:
        return "+20 max HP per stack (heals too)";
    case UPGRADE_BULLET_SPEED:
        return "+15% bullet speed per stack";
    case UPGRADE_DASH_CD:
        return "Dash recharges 15% faster per stack";
    default:
        return "Unknown upgrade.";
    }
}
