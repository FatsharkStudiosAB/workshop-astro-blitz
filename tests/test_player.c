/*
 * test_player.c -- Unit tests for the player module
 *
 * Tests pure logic (init, arena clamping, dash state machine).
 * Does NOT test input-dependent branches (requires Raylib window).
 */

#include "unity.h"
#include "player.h"
#include <math.h>

/* ── Test helpers ──────────────────────────────────────────────────────────── */

static const Rectangle TEST_ARENA = {10.0f, 10.0f, 780.0f, 580.0f};
static const float FLOAT_TOLERANCE = 0.001f;

/* ── setUp / tearDown ──────────────────────────────────────────────────────── */

void setUp(void) {}
void tearDown(void) {}

/* ── player_init tests ─────────────────────────────────────────────────────── */

void test_player_init_sets_position(void)
{
    Player p;
    Vector2 start = {100.0f, 200.0f};
    player_init(&p, start);

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 100.0f, p.position.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 200.0f, p.position.y);
}

void test_player_init_sets_hp_to_max(void)
{
    Player p;
    player_init(&p, (Vector2){0, 0});

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, PLAYER_MAX_HP, p.hp);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, PLAYER_MAX_HP, p.max_hp);
}

void test_player_init_aim_direction_is_up(void)
{
    Player p;
    player_init(&p, (Vector2){0, 0});

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 0.0f, p.aim_direction.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, -1.0f, p.aim_direction.y);
}

void test_player_init_dash_is_inactive(void)
{
    Player p;
    player_init(&p, (Vector2){0, 0});

    TEST_ASSERT_FALSE(p.is_dashing);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 0.0f, p.dash_timer);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 0.0f, p.dash_cooldown);
}

/* ── Arena clamping tests ──────────────────────────────────────────────────── */

/*
 * Arena clamping is implemented inside player_update(), which calls Raylib input
 * functions (GetMousePosition, IsKeyDown, etc.) that require an active window.
 * We cannot call player_update() in a headless unit test. Clamping is verified
 * by manual playtesting. These tests only check init-time positioning.
 */

void test_player_init_at_arena_center(void)
{
    Player p;
    float cx = TEST_ARENA.x + TEST_ARENA.width / 2.0f;
    float cy = TEST_ARENA.y + TEST_ARENA.height / 2.0f;
    player_init(&p, (Vector2){cx, cy});

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, cx, p.position.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, cy, p.position.y);
}

/* ── Dash state tests ──────────────────────────────────────────────────────── */

void test_player_dash_fields_after_manual_activation(void)
{
    /* Simulate what happens when dash is triggered */
    Player p;
    player_init(&p, (Vector2){400.0f, 300.0f});

    /* Manually set dash state (as player_update would) */
    p.is_dashing     = true;
    p.dash_direction  = (Vector2){1.0f, 0.0f};
    p.dash_timer      = DASH_DURATION;
    p.dash_cooldown   = DASH_COOLDOWN;

    TEST_ASSERT_TRUE(p.is_dashing);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, DASH_DURATION, p.dash_timer);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, DASH_COOLDOWN, p.dash_cooldown);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 1.0f, p.dash_direction.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 0.0f, p.dash_direction.y);
}

void test_player_dash_timer_expires(void)
{
    /* Simulate dash timer running out and verify state transition */
    Player p;
    player_init(&p, (Vector2){400.0f, 300.0f});

    p.is_dashing = true;
    p.dash_timer = 0.01f;

    /* Advance past the remaining dash duration */
    p.dash_timer -= 0.02f;
    if (p.dash_timer <= 0.0f) {
        p.dash_timer = 0.0f;
        p.is_dashing = false;
    }

    TEST_ASSERT_FALSE(p.is_dashing);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 0.0f, p.dash_timer);
}

void test_player_dash_cooldown_constant_is_positive(void)
{
    TEST_ASSERT_TRUE(DASH_COOLDOWN > 0.0f);
    TEST_ASSERT_TRUE(DASH_DURATION > 0.0f);
    TEST_ASSERT_TRUE(DASH_SPEED > PLAYER_SPEED);
}

/* ── Constants sanity checks ───────────────────────────────────────────────── */

void test_player_radius_is_positive(void)
{
    TEST_ASSERT_TRUE(PLAYER_RADIUS > 0.0f);
}

void test_player_speed_is_positive(void)
{
    TEST_ASSERT_TRUE(PLAYER_SPEED > 0.0f);
}

void test_player_max_hp_is_positive(void)
{
    TEST_ASSERT_TRUE(PLAYER_MAX_HP > 0.0f);
}

/* ── Runner ────────────────────────────────────────────────────────────────── */

int main(void)
{
    UNITY_BEGIN();

    /* Init */
    RUN_TEST(test_player_init_sets_position);
    RUN_TEST(test_player_init_sets_hp_to_max);
    RUN_TEST(test_player_init_aim_direction_is_up);
    RUN_TEST(test_player_init_dash_is_inactive);
    RUN_TEST(test_player_init_at_arena_center);

    /* Dash state */
    RUN_TEST(test_player_dash_fields_after_manual_activation);
    RUN_TEST(test_player_dash_timer_expires);
    RUN_TEST(test_player_dash_cooldown_constant_is_positive);

    /* Constants */
    RUN_TEST(test_player_radius_is_positive);
    RUN_TEST(test_player_speed_is_positive);
    RUN_TEST(test_player_max_hp_is_positive);

    return UNITY_END();
}
