/*
 * screenshake.h -- Screen shake effect via camera offset
 *
 * Adds trauma-based screen shake to the camera. Trauma decays over time.
 * Higher trauma = stronger shake. Call screenshake_add_trauma to trigger
 * shake events, and screenshake_apply to offset the camera each frame.
 */

#pragma once

#include "raylib.h"

/* ── Constants ─────────────────────────────────────────────────────────────── */

#define SHAKE_MAX_OFFSET 8.0f  /* maximum pixel offset at trauma = 1.0 */
#define SHAKE_DECAY_RATE 3.0f  /* trauma units lost per second */

/* ── Types ─────────────────────────────────────────────────────────────────── */

typedef struct {
    float trauma; /* current trauma level [0.0 - 1.0] */
} ScreenShake;

/* ── Public API ────────────────────────────────────────────────────────────── */

/* Initialize screen shake state (zero trauma). */
void screenshake_init(ScreenShake *shake);

/* Add trauma (clamped to [0, 1]). Typical values: 0.2 for hit, 0.4 for kill. */
void screenshake_add_trauma(ScreenShake *shake, float amount);

/*
 * screenshake_update -- Decay trauma over time.
 *
 * Call once per frame before screenshake_apply.
 */
void screenshake_update(ScreenShake *shake, float dt);

/*
 * screenshake_apply -- Apply shake offset to a camera.
 *
 * Modifies camera.target by a random offset proportional to trauma^2.
 * Call after update_camera (which sets camera.target to player position).
 * Returns the modified camera.
 */
Camera2D screenshake_apply(const ScreenShake *shake, Camera2D camera);
