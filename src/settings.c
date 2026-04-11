/*
 * settings.c -- Persistent game settings (text-based INI file)
 *
 * File format:
 *   movement_layout=tank
 *   movement_layout=8dir
 *
 * Unknown keys are ignored. Missing keys keep their default values.
 */

#include "settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Helpers ───────────────────────────────────────────────────────────────── */

#define MAX_LINE_LENGTH 256

static const char *movement_layout_to_str(MovementLayout layout) {
    switch (layout) {
    case MOVEMENT_8DIR:
        return "8dir";
    case MOVEMENT_TANK:
        return "tank";
    }
    return "8dir"; /* fallback to default */
}

static MovementLayout str_to_movement_layout(const char *str, MovementLayout fallback) {
    if (strcmp(str, "tank") == 0) {
        return MOVEMENT_TANK;
    }
    if (strcmp(str, "8dir") == 0) {
        return MOVEMENT_8DIR;
    }
    return fallback;
}

/* Strip trailing newline/carriage-return from a string in place */
static void strip_newline(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[--len] = '\0';
    }
}

/* ── Public ────────────────────────────────────────────────────────────────── */

/* Clamp a float to [0, max] */
static float clampf(float v, float lo, float hi) {
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

bool settings_init(Settings *s) {
    /* Defaults */
    s->movement_layout = MOVEMENT_8DIR;
    s->screen_shake = 1.0f;
    s->hitstop = 1.0f;
    s->bloom = 1.0f;
    s->scanlines = 1.0f;
    s->chromatic_aberration = 1.0f;
    s->vignette = 1.0f;
    s->lighting = 1.0f;

    /* Try to load from file; if it fails, defaults remain */
    return settings_load(s);
}

bool settings_save(const Settings *s) {
    return settings_save_to(s, SETTINGS_FILE);
}

bool settings_load(Settings *s) {
    return settings_load_from(s, SETTINGS_FILE);
}

bool settings_save_to(const Settings *s, const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) {
        return false;
    }

    if (fprintf(f, "movement_layout=%s\n", movement_layout_to_str(s->movement_layout)) < 0) {
        fclose(f);
        return false;
    }

    /* Visual effect intensities (0.0-1.0) */
    fprintf(f, "screen_shake=%.2f\n", (double)s->screen_shake);
    fprintf(f, "hitstop=%.2f\n", (double)s->hitstop);
    fprintf(f, "bloom=%.2f\n", (double)s->bloom);
    fprintf(f, "scanlines=%.2f\n", (double)s->scanlines);
    fprintf(f, "chromatic_aberration=%.2f\n", (double)s->chromatic_aberration);
    fprintf(f, "vignette=%.2f\n", (double)s->vignette);
    fprintf(f, "lighting=%.2f\n", (double)s->lighting);

    if (fclose(f) != 0) {
        return false;
    }
    return true;
}

bool settings_load_from(Settings *s, const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        return false;
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), f)) {
        strip_newline(line);

        /* Skip empty lines and comments */
        if (line[0] == '\0' || line[0] == '#' || line[0] == ';') {
            continue;
        }

        /* Split on '=' */
        char *eq = strchr(line, '=');
        if (!eq) {
            continue;
        }

        *eq = '\0';
        const char *key = line;
        const char *value = eq + 1;

        if (strcmp(key, "movement_layout") == 0) {
            s->movement_layout = str_to_movement_layout(value, s->movement_layout);
        } else if (strcmp(key, "screen_shake") == 0) {
            s->screen_shake = clampf((float)atof(value), 0.0f, 1.0f);
        } else if (strcmp(key, "hitstop") == 0) {
            s->hitstop = clampf((float)atof(value), 0.0f, 1.0f);
        } else if (strcmp(key, "bloom") == 0) {
            s->bloom = clampf((float)atof(value), 0.0f, 1.0f);
        } else if (strcmp(key, "scanlines") == 0) {
            s->scanlines = clampf((float)atof(value), 0.0f, 1.0f);
        } else if (strcmp(key, "chromatic_aberration") == 0) {
            s->chromatic_aberration = clampf((float)atof(value), 0.0f, 1.0f);
        } else if (strcmp(key, "vignette") == 0) {
            s->vignette = clampf((float)atof(value), 0.0f, 1.0f);
        } else if (strcmp(key, "lighting") == 0) {
            s->lighting = clampf((float)atof(value), 0.0f, 1.0f);
        }
        /* Unknown keys are silently ignored */
    }

    if (ferror(f)) {
        fclose(f);
        return false;
    }

    if (fclose(f) != 0) {
        return false;
    }
    return true;
}
