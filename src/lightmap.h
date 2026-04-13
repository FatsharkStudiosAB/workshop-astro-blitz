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

/* Build the light map texture (draws lights to the off-screen target).
 * Call this OUTSIDE any BeginTextureMode block to avoid nested targets.
 * The camera is needed to convert world positions to screen positions. */
void lightmap_build(LightMap *lm, Camera2D camera);

/* Build the light map with scaled ambient (0 = no shadow, 1 = full).
 * At intensity 0 the build is skipped entirely. */
void lightmap_build_scaled(LightMap *lm, Camera2D camera, float intensity);

/* Composite the already-built light map over the current render target
 * using multiplicative blending. Safe to call inside a BeginTextureMode block
 * because it only draws a textured quad (no nested texture mode). */
void lightmap_composite(LightMap *lm);
