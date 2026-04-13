/*
 * particle.c -- Particle system implementation
 */

#include "particle.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ── Public ────────────────────────────────────────────────────────────────── */

void particle_pool_init(ParticlePool *pool) {
    memset(pool->particles, 0, sizeof(pool->particles));
}

void particle_pool_update(ParticlePool *pool, float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &pool->particles[i];
        if (!p->active) {
            continue;
        }

        /* Move */
        p->position.x += p->velocity.x * dt;
        p->position.y += p->velocity.y * dt;

        /* Age */
        p->lifetime -= dt;
        if (p->lifetime <= 0.0f) {
            p->active = false;
            continue;
        }

        /* Shrink proportionally to remaining lifetime */
        float ratio = p->lifetime / p->max_lifetime;
        p->size = p->start_size * ratio;

        /* Slow down slightly (drag) */
        p->velocity.x *= (1.0f - 1.5f * dt);
        p->velocity.y *= (1.0f - 1.5f * dt);
    }
}

void particle_pool_draw(const ParticlePool *pool) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        const Particle *p = &pool->particles[i];
        if (!p->active) {
            continue;
        }

        /* Fade alpha based on remaining lifetime */
        float ratio = p->lifetime / p->max_lifetime;
        unsigned char alpha = (unsigned char)(ratio * (float)p->color.a);

        Color c = p->color;
        c.a = alpha;

        DrawCircleV(p->position, p->size, c);
    }
}

int particle_pool_active_count(const ParticlePool *pool) {
    int count = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (pool->particles[i].active) {
            count++;
        }
    }
    return count;
}

void particle_emit(ParticlePool *pool, Vector2 pos, Vector2 vel, float lifetime, float size,
                   Color color) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &pool->particles[i];
        if (!p->active) {
            p->position = pos;
            p->velocity = vel;
            p->lifetime = lifetime;
            p->max_lifetime = lifetime;
            p->size = size;
            p->start_size = size;
            p->color = color;
            p->active = true;
            return;
        }
    }
    /* Pool full -- silently drop */
}

void particle_burst(ParticlePool *pool, Vector2 pos, int count, float speed_min, float speed_max,
                    float life_min, float life_max, float size, Color color) {
    for (int i = 0; i < count; i++) {
        /* Random angle in full circle */
        float angle = ((float)GetRandomValue(0, 3600) / 3600.0f) * 2.0f * (float)M_PI;
        /* Random speed in range */
        float speed =
            speed_min + ((float)GetRandomValue(0, 1000) / 1000.0f) * (speed_max - speed_min);
        /* Random lifetime in range */
        float life = life_min + ((float)GetRandomValue(0, 1000) / 1000.0f) * (life_max - life_min);

        Vector2 vel = {cosf(angle) * speed, sinf(angle) * speed};
        particle_emit(pool, pos, vel, life, size, color);
    }
}
