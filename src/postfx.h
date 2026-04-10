/*
 * postfx.h -- Post-processing effects (bloom, CRT, vignette)
 *
 * Renders the scene to an off-screen texture, then draws it to the screen
 * through a combined fragment shader for bloom glow, scanlines, chromatic
 * aberration, and vignette darkening.
 */

#pragma once

#include "raylib.h"
#include <stdbool.h>

/* ── Types ─────────────────────────────────────────────────────────────────── */

typedef struct {
    RenderTexture2D target;  /* scene rendered here first */
    Shader shader;           /* combined post-processing shader */
    int time_loc;            /* uniform: elapsed time */
    int resolution_loc;      /* uniform: screen resolution */
    int bloom_intensity_loc; /* uniform: bloom strength */
    bool enabled;            /* toggle post-processing on/off */
} PostFX;

/* ── Public API ────────────────────────────────────────────────────────────── */

/* Initialize the post-processing pipeline. Call after InitWindow(). */
void postfx_init(PostFX *pfx, int width, int height);

/* Release GPU resources. Call before CloseWindow(). */
void postfx_cleanup(PostFX *pfx);

/* Begin rendering the scene (redirects to off-screen texture). */
void postfx_begin(PostFX *pfx);

/* End scene rendering and draw to screen through the shader. */
void postfx_end(PostFX *pfx, float time);

/* Toggle post-processing on/off. */
void postfx_toggle(PostFX *pfx);
