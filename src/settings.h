/*
 * settings.h -- Persistent game settings
 *
 * Settings are stored in a text-based INI-style file (key=value pairs).
 * The file is loaded on startup and saved whenever a setting changes.
 */

#pragma once

#include <stdbool.h>

/* ── Constants ─────────────────────────────────────────────────────────────── */

#define SETTINGS_FILE "settings.ini"

/* ── Types ─────────────────────────────────────────────────────────────────── */

/* Movement layout controls how WASD maps to world-space directions */
typedef enum {
    MOVEMENT_8DIR, /* W = up, S = down, A = left, D = right (screen-relative, default) */
    MOVEMENT_TANK  /* W/S = forward/back relative to aim, A/D = strafe */
} MovementLayout;

/* All persistent game settings */
typedef struct {
    MovementLayout movement_layout;
} Settings;

/* ── Public API ────────────────────────────────────────────────────────────── */

/*
 * settings_init -- Initialize settings with defaults, then load from file.
 *
 * If the settings file does not exist or is malformed, defaults are used.
 * Returns true if an existing settings file was loaded, false if this is
 * the first run (no file found).
 */
bool settings_init(Settings *s);

/*
 * settings_save -- Write current settings to the settings file.
 *
 * Returns true on success, false on I/O error.
 */
bool settings_save(const Settings *s);

/*
 * settings_load -- Load settings from the settings file.
 *
 * Returns true if the file was loaded successfully. Missing or malformed
 * entries keep their existing values (caller should call settings_init first).
 */
bool settings_load(Settings *s);

/*
 * settings_load_from -- Load settings from a specific file path.
 *
 * Same behavior as settings_load but reads from the given path.
 * Useful for testing without touching the default settings file.
 */
bool settings_load_from(Settings *s, const char *path);

/*
 * settings_save_to -- Write current settings to a specific file path.
 *
 * Same behavior as settings_save but writes to the given path.
 * Useful for testing without touching the default settings file.
 */
bool settings_save_to(const Settings *s, const char *path);
