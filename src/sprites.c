/*
 * sprites.c -- Hardcoded pixel art sprite data and drawing
 *
 * Each sprite is a 2D array of palette indices. Index 0 = transparent.
 * Sprites are drawn pixel-by-pixel, rotated to face the aim/movement direction.
 * At 400x300 internal resolution, these sprites look appropriately chunky.
 */

#include "sprites.h"
#include <math.h>
#include <stdbool.h>

/* ── Palette helper ────────────────────────────────────────────────────────── */

/* Draw a single rotated sprite given pixel data, palette, and direction.
 * The sprite faces "up" (negative Y) by default; `dir` rotates it. */
static void draw_rotated_sprite(const unsigned char *data, int w, int h, const Color *palette,
                                int palette_size, Vector2 pos, Vector2 dir) {
    /* Rotation: dir is the facing direction; compute sin/cos for rotation */
    /* Default sprite orientation: facing "up" = (0, -1). We rotate from (0,-1) to dir. */
    float angle = atan2f(dir.y, dir.x) + 1.5707963f; /* +PI/2 to convert from "right" to "up" */
    float ca = cosf(angle);
    float sa = sinf(angle);

    float cx = (float)w * 0.5f;
    float cy = (float)h * 0.5f;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            unsigned char idx = data[y * w + x];
            if (idx == 0 || idx >= palette_size) {
                continue; /* transparent */
            }
            /* Offset from center */
            float ox = (float)x - cx + 0.5f;
            float oy = (float)y - cy + 0.5f;
            /* Rotate */
            float rx = ox * ca - oy * sa;
            float ry = ox * sa + oy * ca;
            /* Final position */
            int px = (int)(pos.x + rx);
            int py = (int)(pos.y + ry);
            DrawPixel(px, py, palette[idx]);
        }
    }
}

/* ── Player sprite data (16x16, facing up) ─────────────────────────────────── */
/* Palette: 0=transparent, 1=dark body, 2=mid armor, 3=neon outline,
 *          4=visor, 5=reactor core, 6=weapon barrel */
/* clang-format off */
static const unsigned char player_data[PLAYER_SPRITE_H][PLAYER_SPRITE_W] = {
    {0,0,0,0,0,0,0,3,3,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,3,2,2,3,0,0,0,0,0,0},
    {0,0,0,0,0,3,2,2,2,2,3,0,0,0,0,0},
    {0,0,0,0,0,3,2,4,4,2,3,0,0,0,0,0},
    {0,0,0,0,3,2,2,2,2,2,2,3,0,0,0,0},
    {0,0,0,3,2,2,2,2,2,2,2,2,3,0,0,0},
    {0,0,3,1,2,2,2,5,5,2,2,2,1,3,0,0},
    {0,3,3,1,1,2,2,5,5,2,2,1,1,3,3,0},
    {0,3,0,1,1,1,2,2,2,2,1,1,1,0,3,0},
    {0,3,0,1,1,1,1,2,2,1,1,1,1,0,3,0},
    {0,0,0,3,1,1,1,1,1,1,1,1,3,0,0,0},
    {0,0,0,3,1,1,1,1,1,1,1,1,3,0,0,0},
    {0,0,0,0,3,1,1,1,1,1,1,3,0,0,0,0},
    {0,0,0,0,3,1,1,0,0,1,1,3,0,0,0,0},
    {0,0,0,0,0,3,3,0,0,3,3,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};
/* clang-format on */

/* ── Swarmer sprite data (10x10, facing up) ────────────────────────────────── */
/* Palette: 0=transparent, 1=dark body, 2=red-orange, 3=bright eyes */
/* clang-format off */
static const unsigned char swarmer_data[SWARMER_SPRITE_H][SWARMER_SPRITE_W] = {
    {0,0,0,0,2,2,0,0,0,0},
    {0,0,0,2,1,1,2,0,0,0},
    {0,0,2,1,3,3,1,2,0,0},
    {0,2,1,1,1,1,1,1,2,0},
    {2,1,1,1,1,1,1,1,1,2},
    {2,1,1,1,1,1,1,1,1,2},
    {0,2,1,1,1,1,1,1,2,0},
    {0,0,2,1,1,1,1,2,0,0},
    {0,0,0,2,1,1,2,0,0,0},
    {0,0,0,0,2,2,0,0,0,0},
};
/* clang-format on */

/* ── Grunt sprite data (12x12, facing up) ──────────────────────────────────── */
/* Palette: 0=transparent, 1=dark blue, 2=blue, 3=bright blue, 4=barrel */
/* clang-format off */
static const unsigned char grunt_data[GRUNT_SPRITE_H][GRUNT_SPRITE_W] = {
    {0,0,0,0,0,4,4,0,0,0,0,0},
    {0,0,0,0,4,4,4,4,0,0,0,0},
    {0,0,0,3,2,2,2,2,3,0,0,0},
    {0,0,3,2,3,3,3,3,2,3,0,0},
    {0,3,1,2,2,2,2,2,2,1,3,0},
    {0,3,1,1,2,2,2,2,1,1,3,0},
    {3,1,1,1,1,2,2,1,1,1,1,3},
    {3,1,1,1,1,1,1,1,1,1,1,3},
    {0,3,1,1,1,1,1,1,1,1,3,0},
    {0,0,3,1,1,1,1,1,1,3,0,0},
    {0,0,0,3,1,0,0,1,3,0,0,0},
    {0,0,0,0,3,0,0,3,0,0,0,0},
};
/* clang-format on */

/* ── Stalker sprite data (10x10, facing up) ────────────────────────────────── */
/* Palette: 0=transparent, 1=dark green, 2=green, 3=bright green */
/* clang-format off */
static const unsigned char stalker_data[STALKER_SPRITE_H][STALKER_SPRITE_W] = {
    {0,0,0,0,3,3,0,0,0,0},
    {0,0,0,3,2,2,3,0,0,0},
    {0,0,3,1,3,3,1,3,0,0},
    {0,0,2,1,1,1,1,2,0,0},
    {0,3,1,1,1,1,1,1,3,0},
    {0,3,1,1,1,1,1,1,3,0},
    {0,0,2,1,1,1,1,2,0,0},
    {0,0,0,2,1,1,2,0,0,0},
    {0,0,3,0,2,2,0,3,0,0},
    {0,3,0,0,0,0,0,0,3,0},
};
/* clang-format on */

/* ── Bomber sprite data (14x14, facing up) ─────────────────────────────────── */
/* Palette: 0=transparent, 1=dark orange, 2=orange, 3=bright yellow, 4=core */
/* clang-format off */
static const unsigned char bomber_data[BOMBER_SPRITE_H][BOMBER_SPRITE_W] = {
    {0,0,0,0,0,2,2,2,2,0,0,0,0,0},
    {0,0,0,0,2,1,1,1,1,2,0,0,0,0},
    {0,0,0,2,1,1,1,1,1,1,2,0,0,0},
    {0,0,2,1,1,1,1,1,1,1,1,2,0,0},
    {0,2,1,1,1,3,3,3,3,1,1,1,2,0},
    {2,1,1,1,3,4,4,4,4,3,1,1,1,2},
    {2,1,1,1,3,4,4,4,4,3,1,1,1,2},
    {2,1,1,1,3,4,4,4,4,3,1,1,1,2},
    {2,1,1,1,3,4,4,4,4,3,1,1,1,2},
    {0,2,1,1,1,3,3,3,3,1,1,1,2,0},
    {0,0,2,1,1,1,1,1,1,1,1,2,0,0},
    {0,0,0,2,1,1,1,1,1,1,2,0,0,0},
    {0,0,0,0,2,1,1,1,1,2,0,0,0,0},
    {0,0,0,0,0,2,2,2,2,0,0,0,0,0},
};
/* clang-format on */

/* ── Palettes ──────────────────────────────────────────────────────────────── */

/* Player normal palette */
static const Color player_palette[] = {
    {0, 0, 0, 0},         /* 0: transparent */
    {6, 35, 50, 255},     /* 1: dark body */
    {10, 60, 80, 255},    /* 2: mid armor */
    {0, 230, 210, 255},   /* 3: neon outline */
    {180, 255, 245, 255}, /* 4: visor */
    {0, 200, 180, 255},   /* 5: reactor core */
    {0, 180, 160, 255},   /* 6: weapon barrel */
};
#define PLAYER_PALETTE_SIZE (sizeof(player_palette) / sizeof(player_palette[0]))

/* Player dash palette */
static const Color player_dash_palette[] = {
    {0, 0, 0, 0},         /* 0: transparent */
    {12, 30, 80, 255},    /* 1: dark body */
    {20, 55, 140, 255},   /* 2: mid armor */
    {80, 180, 255, 255},  /* 3: neon outline */
    {160, 220, 255, 255}, /* 4: visor */
    {100, 200, 255, 255}, /* 5: reactor core */
    {80, 160, 255, 255},  /* 6: weapon barrel */
};

/* Swarmer palette */
static const Color swarmer_palette[] = {
    {0, 0, 0, 0},        /* 0: transparent */
    {80, 20, 10, 255},   /* 1: dark body */
    {255, 60, 30, 255},  /* 2: red-orange outline */
    {255, 200, 50, 255}, /* 3: bright eyes */
};
#define SWARMER_PALETTE_SIZE (sizeof(swarmer_palette) / sizeof(swarmer_palette[0]))

/* Grunt palette */
static const Color grunt_palette[] = {
    {0, 0, 0, 0},         /* 0: transparent */
    {20, 20, 80, 255},    /* 1: dark blue */
    {60, 60, 200, 255},   /* 2: blue */
    {120, 120, 255, 255}, /* 3: bright blue */
    {200, 200, 255, 255}, /* 4: barrel/visor */
};
#define GRUNT_PALETTE_SIZE (sizeof(grunt_palette) / sizeof(grunt_palette[0]))

/* Stalker palette */
static const Color stalker_palette[] = {
    {0, 0, 0, 0},       /* 0: transparent */
    {10, 60, 10, 255},  /* 1: dark green */
    {30, 180, 30, 255}, /* 2: green */
    {80, 255, 80, 255}, /* 3: bright green */
};
#define STALKER_PALETTE_SIZE (sizeof(stalker_palette) / sizeof(stalker_palette[0]))

/* Bomber palette */
static const Color bomber_palette[] = {
    {0, 0, 0, 0},        /* 0: transparent */
    {80, 40, 10, 255},   /* 1: dark orange */
    {255, 150, 30, 255}, /* 2: orange outline */
    {255, 220, 50, 255}, /* 3: bright yellow */
    {255, 100, 20, 255}, /* 4: core glow */
};
#define BOMBER_PALETTE_SIZE (sizeof(bomber_palette) / sizeof(bomber_palette[0]))

/* Bomber charging palette (brighter) */
static const Color bomber_charge_palette[] = {
    {0, 0, 0, 0},         /* 0: transparent */
    {120, 60, 10, 255},   /* 1: dark orange */
    {255, 200, 60, 255},  /* 2: bright orange */
    {255, 255, 150, 255}, /* 3: near-white yellow */
    {255, 60, 10, 255},   /* 4: hot core */
};

/* Hit flash palette (all white) */
static const Color flash_palette[] = {
    {0, 0, 0, 0},         /* 0: transparent */
    {255, 255, 255, 255}, /* 1 */
    {255, 255, 255, 255}, /* 2 */
    {255, 255, 255, 255}, /* 3 */
    {255, 255, 255, 255}, /* 4 */
    {255, 255, 255, 255}, /* 5 */
    {255, 255, 255, 255}, /* 6 */
};
#define FLASH_PALETTE_SIZE (sizeof(flash_palette) / sizeof(flash_palette[0]))

/* ── Public API implementation ─────────────────────────────────────────────── */

void sprite_draw_player(Vector2 pos, Vector2 aim, bool is_dashing) {
    const Color *pal = is_dashing ? player_dash_palette : player_palette;
    draw_rotated_sprite((const unsigned char *)player_data, PLAYER_SPRITE_W, PLAYER_SPRITE_H, pal,
                        (int)PLAYER_PALETTE_SIZE, pos, aim);
}

void sprite_draw_swarmer(Vector2 pos, Vector2 move_dir, float hit_flash) {
    const Color *pal = (hit_flash > 0.0f) ? flash_palette : swarmer_palette;
    int pal_size = (hit_flash > 0.0f) ? (int)FLASH_PALETTE_SIZE : (int)SWARMER_PALETTE_SIZE;
    draw_rotated_sprite((const unsigned char *)swarmer_data, SWARMER_SPRITE_W, SWARMER_SPRITE_H,
                        pal, pal_size, pos, move_dir);
}

void sprite_draw_grunt(Vector2 pos, Vector2 move_dir, float hit_flash) {
    const Color *pal = (hit_flash > 0.0f) ? flash_palette : grunt_palette;
    int pal_size = (hit_flash > 0.0f) ? (int)FLASH_PALETTE_SIZE : (int)GRUNT_PALETTE_SIZE;
    draw_rotated_sprite((const unsigned char *)grunt_data, GRUNT_SPRITE_W, GRUNT_SPRITE_H, pal,
                        pal_size, pos, move_dir);
}

void sprite_draw_stalker(Vector2 pos, Vector2 move_dir, float hit_flash) {
    const Color *pal = (hit_flash > 0.0f) ? flash_palette : stalker_palette;
    int pal_size = (hit_flash > 0.0f) ? (int)FLASH_PALETTE_SIZE : (int)STALKER_PALETTE_SIZE;
    draw_rotated_sprite((const unsigned char *)stalker_data, STALKER_SPRITE_W, STALKER_SPRITE_H,
                        pal, pal_size, pos, move_dir);
}

void sprite_draw_bomber(Vector2 pos, Vector2 move_dir, float hit_flash, bool is_charging) {
    const Color *pal;
    int pal_size;
    if (hit_flash > 0.0f) {
        pal = flash_palette;
        pal_size = (int)FLASH_PALETTE_SIZE;
    } else if (is_charging) {
        pal = bomber_charge_palette;
        pal_size = (int)BOMBER_PALETTE_SIZE;
    } else {
        pal = bomber_palette;
        pal_size = (int)BOMBER_PALETTE_SIZE;
    }
    draw_rotated_sprite((const unsigned char *)bomber_data, BOMBER_SPRITE_W, BOMBER_SPRITE_H, pal,
                        pal_size, pos, move_dir);
}
