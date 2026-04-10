/*
 * test_game.c -- Unit tests for the game module
 *
 * Tests game_init (arena dimensions, player placement, bullet pool state).
 * Does NOT test game_update/game_draw (require Raylib window context).
 */

#include "unity.h"
#include "game.h"

/* ── Test helpers ──────────────────────────────────────────────────────────── */

static const float FLOAT_TOLERANCE = 0.001f;

/* ── setUp / tearDown ──────────────────────────────────────────────────────── */

void setUp(void) {}
void tearDown(void) {}

/* ── game_init tests ───────────────────────────────────────────────────────── */

void test_game_init_arena_position(void)
{
    GameState gs;
    game_init(&gs);

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, ARENA_MARGIN, gs.arena.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, ARENA_MARGIN, gs.arena.y);
}

void test_game_init_arena_dimensions(void)
{
    GameState gs;
    game_init(&gs);

    float expected_w = SCREEN_WIDTH - 2.0f * ARENA_MARGIN;
    float expected_h = SCREEN_HEIGHT - 2.0f * ARENA_MARGIN;

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, expected_w, gs.arena.width);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, expected_h, gs.arena.height);
}

void test_game_init_player_centered_in_arena(void)
{
    GameState gs;
    game_init(&gs);

    float expected_x = gs.arena.x + gs.arena.width / 2.0f;
    float expected_y = gs.arena.y + gs.arena.height / 2.0f;

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, expected_x, gs.player.position.x);
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, expected_y, gs.player.position.y);
}

void test_game_init_player_hp_full(void)
{
    GameState gs;
    game_init(&gs);

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, gs.player.max_hp, gs.player.hp);
}

void test_game_init_player_not_dashing(void)
{
    GameState gs;
    game_init(&gs);

    TEST_ASSERT_FALSE(gs.player.is_dashing);
}

void test_game_init_bullet_pool_empty(void)
{
    GameState gs;
    game_init(&gs);

    for (int i = 0; i < MAX_BULLETS; i++) {
        TEST_ASSERT_FALSE(gs.bullets.bullets[i].active);
    }
}

void test_game_init_bullet_pool_cooldown_zero(void)
{
    GameState gs;
    game_init(&gs);

    TEST_ASSERT_FLOAT_WITHIN(FLOAT_TOLERANCE, 0.0f, gs.bullets.fire_cooldown);
}

/* ── Constants sanity checks ───────────────────────────────────────────────── */

void test_screen_dimensions_positive(void)
{
    TEST_ASSERT_TRUE(SCREEN_WIDTH > 0);
    TEST_ASSERT_TRUE(SCREEN_HEIGHT > 0);
}

void test_arena_margin_positive(void)
{
    TEST_ASSERT_TRUE(ARENA_MARGIN > 0.0f);
}

void test_arena_fits_on_screen(void)
{
    TEST_ASSERT_TRUE(SCREEN_WIDTH - 2.0f * ARENA_MARGIN > 0.0f);
    TEST_ASSERT_TRUE(SCREEN_HEIGHT - 2.0f * ARENA_MARGIN > 0.0f);
}

/* ── Runner ────────────────────────────────────────────────────────────────── */

int main(void)
{
    UNITY_BEGIN();

    /* game_init */
    RUN_TEST(test_game_init_arena_position);
    RUN_TEST(test_game_init_arena_dimensions);
    RUN_TEST(test_game_init_player_centered_in_arena);
    RUN_TEST(test_game_init_player_hp_full);
    RUN_TEST(test_game_init_player_not_dashing);
    RUN_TEST(test_game_init_bullet_pool_empty);
    RUN_TEST(test_game_init_bullet_pool_cooldown_zero);

    /* Constants */
    RUN_TEST(test_screen_dimensions_positive);
    RUN_TEST(test_arena_margin_positive);
    RUN_TEST(test_arena_fits_on_screen);

    return UNITY_END();
}
