/*
 * particle.h -- Particle system for visual effects
 *
 * Pool-based particle emitter for muzzle flash, hit sparks, death explosions,
 * dash trails, and ambient effects. Each particle has position, velocity,
 * lifetime, color, and size that evolve over time.
 */

#pragma once

#include "raylib.h"
#include <stdbool.h>

/* ── Constants ─────────────────────────────────────────────────────────────── */

#define MAX_PARTICLES 512

/* ── Types ─────────────────────────────────────────────────────────────────── */

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float lifetime;     /* remaining lifetime in seconds */
    float max_lifetime; /* initial lifetime (for ratio calculations) */
    float size;         /* current radius */
    float start_size;   /* initial radius */
    Color color;        /* base color (alpha fades with lifetime) */
    bool active;
} Particle;

typedef struct {
    Particle particles[MAX_PARTICLES];
} ParticlePool;

/* ── Public API ────────────────────────────────────────────────────────────── */

/* Initialize all particles to inactive. */
void particle_pool_init(ParticlePool *pool);

/* Update all active particles: move, age, fade, shrink. Deactivate expired. */
void particle_pool_update(ParticlePool *pool, float dt);

/* Draw all active particles as filled circles with alpha fade. */
void particle_pool_draw(const ParticlePool *pool);

/* Count the number of currently active particles. */
int particle_pool_active_count(const ParticlePool *pool);

/*
 * particle_emit -- Spawn a single particle.
 *
 * Parameters:
 *   pool     -- Particle pool to emit into.
 *   pos      -- World-space spawn position.
 *   vel      -- Initial velocity (pixels/sec).
 *   lifetime -- How long the particle lives (seconds).
 *   size     -- Initial radius.
 *   color    -- Base color (alpha will fade over lifetime).
 *
 * Silently drops if the pool is full.
 */
void particle_emit(ParticlePool *pool, Vector2 pos, Vector2 vel, float lifetime, float size,
                   Color color);

/*
 * particle_burst -- Emit multiple particles in a radial pattern.
 *
 * Spawns `count` particles at `pos` with random velocities between
 * `speed_min` and `speed_max`, random lifetimes between `life_min`
 * and `life_max`, and the given size and color.
 */
void particle_burst(ParticlePool *pool, Vector2 pos, int count, float speed_min, float speed_max,
                    float life_min, float life_max, float size, Color color);
