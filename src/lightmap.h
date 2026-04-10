/*
 * lightmap.h -- 2D multiplicative lighting (Children of Morta technique)
 *
 * Renders a dark "light map" texture with additive soft circles for each
 * light source, then composites it over the scene with multiplicative
 * blending.  Where the light map is bright, the scene appears normally;
 * where it is dark, the scene is shadowed.  Colored lights tint the scene.
 */

#pragma once

#include "raylib.h"
#include <stdbool.h>

/* ── Constants ─────────────────────────────────────────────────────────────── */

#define MAX_LIGHTS 64

/* ── Types ─────────────────────────────────────────────────────────────────── */

typedef struct {
    Vector2 position; /* world-space position */
    Color color;      /* light color (additive) */
    float radius;     /* light radius in world pixels */
    bool active;
} Light;

typedef struct {
    RenderTexture2D target;   /* light map texture (screen size) */
    Texture2D gradient;       /* pre-generated radial gradient (white->transparent) */
    Light lights[MAX_LIGHTS]; /* per-frame light list */
    int light_count;
    Color ambient; /* base darkness level */
} LightMap;

/* ── Public API ────────────────────────────────────────────────────────────── */

/* Initialize the light map system.  Call after InitWindow(). */
void lightmap_init(LightMap *lm, int width, int height);

/* Release GPU resources.  Call before CloseWindow(). */
void lightmap_cleanup(LightMap *lm);

/* Clear the light list for this frame. */
void lightmap_clear(LightMap *lm);

/* Add a light source for this frame. */
void lightmap_add(LightMap *lm, Vector2 world_pos, Color color, float radius);

/* Render the light map and composite it over the scene.
 * Call AFTER EndMode2D() but BEFORE EndDrawing() / postfx_end().
 * The camera is needed to convert world positions to screen positions. */
void lightmap_render(LightMap *lm, Camera2D camera);
