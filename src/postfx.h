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
    RenderTexture2D target;     /* scene rendered here first (render resolution) */
    RenderTexture2D output;     /* post-processed result (render resolution) */
    Shader shader;              /* combined post-processing shader */
    int time_loc;               /* uniform: elapsed time */
    int resolution_loc;         /* uniform: screen resolution */
    int bloom_intensity_loc;    /* uniform: bloom strength (0-1) */
    int scanline_intensity_loc; /* uniform: scanline strength (0-1) */
    int aberration_amount_loc;  /* uniform: chromatic aberration (0-1) */
    int vignette_amount_loc;    /* uniform: vignette darkening (0-1) */
    bool enabled;               /* toggle post-processing on/off */
} PostFX;

/* ── Public API ────────────────────────────────────────────────────────────── */

/* Initialize the post-processing pipeline. Call after InitWindow(). */
void postfx_init(PostFX *pfx, int width, int height);

/* Release GPU resources. Call before CloseWindow(). */
void postfx_cleanup(PostFX *pfx);

/* Begin rendering the scene (redirects to off-screen texture). */
void postfx_begin(PostFX *pfx);

/* End scene rendering and apply the shader to the output texture.
 * Does NOT call BeginDrawing/EndDrawing -- caller handles that.
 * After this call, use postfx_get_texture() to access the result. */
void postfx_end(PostFX *pfx, float time);

/* Get the post-processed output texture (Y-flipped RenderTexture).
 * Use this to draw the result to the screen at any scale. */
Texture2D postfx_get_texture(const PostFX *pfx);

/* Toggle post-processing on/off. */
void postfx_toggle(PostFX *pfx);

/* Set effect intensities (all 0.0-1.0). Call before postfx_end each frame. */
void postfx_set_params(PostFX *pfx, float bloom, float scanlines, float aberration, float vignette);
