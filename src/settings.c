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

bool settings_init(Settings *s) {
    /* Defaults */
    s->movement_layout = MOVEMENT_8DIR;

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
