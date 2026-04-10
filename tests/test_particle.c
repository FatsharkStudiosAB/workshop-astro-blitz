/*
 * test_particle.c -- Unit tests for the particle system
 */

#include "unity.h"

#include "particle.h"

#define FLOAT_EPSILON 1e-3f

static ParticlePool pool;

void setUp(void) {
    particle_pool_init(&pool);
}

void tearDown(void) {
}

/* ── Init tests ───────────────────────────────────────────────────────────── */

void test_init_all_inactive(void) {
    TEST_ASSERT_EQUAL_INT(0, particle_pool_active_count(&pool));
}

/* ── Emit tests ───────────────────────────────────────────────────────────── */

void test_emit_activates_particle(void) {
    particle_emit(&pool, (Vector2){100, 200}, (Vector2){10, -5}, 1.0f, 4.0f, RED);
    TEST_ASSERT_EQUAL_INT(1, particle_pool_active_count(&pool));
}

void test_emit_sets_fields(void) {
    particle_emit(&pool, (Vector2){50, 60}, (Vector2){1, 2}, 0.5f, 3.0f, GREEN);

    Particle *p = &pool.particles[0];
    TEST_ASSERT_TRUE(p->active);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 50.0f, p->position.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 60.0f, p->position.y);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 1.0f, p->velocity.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 2.0f, p->velocity.y);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.5f, p->lifetime);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 0.5f, p->max_lifetime);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 3.0f, p->size);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 3.0f, p->start_size);
}

void test_emit_multiple(void) {
    for (int i = 0; i < 5; i++) {
        particle_emit(&pool, (Vector2){0, 0}, (Vector2){0, 0}, 1.0f, 1.0f, WHITE);
    }
    TEST_ASSERT_EQUAL_INT(5, particle_pool_active_count(&pool));
}

void test_emit_full_pool_silently_drops(void) {
    /* Fill the pool */
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particle_emit(&pool, (Vector2){0, 0}, (Vector2){0, 0}, 1.0f, 1.0f, WHITE);
    }
    TEST_ASSERT_EQUAL_INT(MAX_PARTICLES, particle_pool_active_count(&pool));

    /* One more should be silently dropped */
    particle_emit(&pool, (Vector2){0, 0}, (Vector2){0, 0}, 1.0f, 1.0f, RED);
    TEST_ASSERT_EQUAL_INT(MAX_PARTICLES, particle_pool_active_count(&pool));
}

/* ── Update tests ─────────────────────────────────────────────────────────── */

void test_update_moves_particle(void) {
    particle_emit(&pool, (Vector2){100, 100}, (Vector2){200, 0}, 1.0f, 2.0f, RED);
    particle_pool_update(&pool, 0.1f);

    /* Position should have moved ~20 px right (with slight drag) */
    Particle *p = &pool.particles[0];
    TEST_ASSERT_GREATER_THAN(110.0f, p->position.x);
}

void test_update_ages_particle(void) {
    particle_emit(&pool, (Vector2){0, 0}, (Vector2){0, 0}, 1.0f, 2.0f, RED);
    particle_pool_update(&pool, 0.3f);

    Particle *p = &pool.particles[0];
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.7f, p->lifetime);
}

void test_update_deactivates_expired(void) {
    particle_emit(&pool, (Vector2){0, 0}, (Vector2){0, 0}, 0.1f, 2.0f, RED);
    TEST_ASSERT_EQUAL_INT(1, particle_pool_active_count(&pool));

    particle_pool_update(&pool, 0.2f);
    TEST_ASSERT_EQUAL_INT(0, particle_pool_active_count(&pool));
}

void test_update_shrinks_particle(void) {
    particle_emit(&pool, (Vector2){0, 0}, (Vector2){0, 0}, 1.0f, 4.0f, RED);
    particle_pool_update(&pool, 0.5f);

    /* Half lifetime gone => size should be ~half of start */
    Particle *p = &pool.particles[0];
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 2.0f, p->size);
}

void test_update_applies_drag(void) {
    particle_emit(&pool, (Vector2){0, 0}, (Vector2){100, 0}, 2.0f, 1.0f, RED);

    /* After several updates the velocity should decrease */
    for (int i = 0; i < 10; i++) {
        particle_pool_update(&pool, 0.1f);
    }

    Particle *p = &pool.particles[0];
    TEST_ASSERT_LESS_THAN(100.0f, p->velocity.x);
}

/* ── Reuse tests ──────────────────────────────────────────────────────────── */

void test_expired_slot_reused(void) {
    particle_emit(&pool, (Vector2){0, 0}, (Vector2){0, 0}, 0.05f, 1.0f, RED);
    particle_pool_update(&pool, 0.1f);
    TEST_ASSERT_EQUAL_INT(0, particle_pool_active_count(&pool));

    /* Emit again -- should reuse the same slot */
    particle_emit(&pool, (Vector2){10, 20}, (Vector2){0, 0}, 1.0f, 1.0f, BLUE);
    TEST_ASSERT_EQUAL_INT(1, particle_pool_active_count(&pool));
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, 10.0f, pool.particles[0].position.x);
}

/* ── Constants tests ──────────────────────────────────────────────────────── */

void test_max_particles_constant(void) {
    TEST_ASSERT_EQUAL_INT(512, MAX_PARTICLES);
}

/* ── Main ─────────────────────────────────────────────────────────────────── */

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_all_inactive);
    RUN_TEST(test_emit_activates_particle);
    RUN_TEST(test_emit_sets_fields);
    RUN_TEST(test_emit_multiple);
    RUN_TEST(test_emit_full_pool_silently_drops);
    RUN_TEST(test_update_moves_particle);
    RUN_TEST(test_update_ages_particle);
    RUN_TEST(test_update_deactivates_expired);
    RUN_TEST(test_update_shrinks_particle);
    RUN_TEST(test_update_applies_drag);
    RUN_TEST(test_expired_slot_reused);
    RUN_TEST(test_max_particles_constant);
    return UNITY_END();
}
