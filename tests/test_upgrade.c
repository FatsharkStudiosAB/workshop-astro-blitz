/*
 * test_upgrade.c -- Unit tests for the passive upgrade system
 */

#include "unity.h"
#include "upgrade.h"

void setUp(void) {}
void tearDown(void) {}

void test_upgrade_init_all_zero(void) {
    UpgradeState us;
    upgrade_state_init(&us);
    for (int i = 0; i < UPGRADE_COUNT; i++) {
        TEST_ASSERT_EQUAL_INT(0, us.stacks[i]);
    }
}

void test_upgrade_add_increments(void) {
    UpgradeState us;
    upgrade_state_init(&us);
    TEST_ASSERT_TRUE(upgrade_add(&us, UPGRADE_SPEED));
    TEST_ASSERT_EQUAL_INT(1, us.stacks[UPGRADE_SPEED]);
    TEST_ASSERT_TRUE(upgrade_add(&us, UPGRADE_SPEED));
    TEST_ASSERT_EQUAL_INT(2, us.stacks[UPGRADE_SPEED]);
}

void test_upgrade_add_caps_at_max(void) {
    UpgradeState us;
    upgrade_state_init(&us);
    for (int i = 0; i < MAX_UPGRADE_STACKS; i++) {
        TEST_ASSERT_TRUE(upgrade_add(&us, UPGRADE_DAMAGE));
    }
    /* Should fail at max */
    TEST_ASSERT_FALSE(upgrade_add(&us, UPGRADE_DAMAGE));
    TEST_ASSERT_EQUAL_INT(MAX_UPGRADE_STACKS, us.stacks[UPGRADE_DAMAGE]);
}

void test_upgrade_speed_bonus(void) {
    UpgradeState us;
    upgrade_state_init(&us);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, upgrade_get_speed_bonus(&us));
    upgrade_add(&us, UPGRADE_SPEED);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, UPGRADE_SPEED_BONUS, upgrade_get_speed_bonus(&us));
    upgrade_add(&us, UPGRADE_SPEED);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.0f * UPGRADE_SPEED_BONUS, upgrade_get_speed_bonus(&us));
}

void test_upgrade_damage_bonus(void) {
    UpgradeState us;
    upgrade_state_init(&us);
    upgrade_add(&us, UPGRADE_DAMAGE);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, UPGRADE_DAMAGE_BONUS, upgrade_get_damage_bonus(&us));
}

void test_upgrade_fire_rate_mult(void) {
    UpgradeState us;
    upgrade_state_init(&us);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, upgrade_get_fire_rate_mult(&us));
    upgrade_add(&us, UPGRADE_FIRE_RATE);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, UPGRADE_FIRE_RATE_BONUS, upgrade_get_fire_rate_mult(&us));
}

void test_upgrade_max_hp_bonus(void) {
    UpgradeState us;
    upgrade_state_init(&us);
    upgrade_add(&us, UPGRADE_MAX_HP);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, UPGRADE_MAX_HP_BONUS, upgrade_get_max_hp_bonus(&us));
}

void test_upgrade_bullet_speed_mult(void) {
    UpgradeState us;
    upgrade_state_init(&us);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, upgrade_get_bullet_speed_mult(&us));
    upgrade_add(&us, UPGRADE_BULLET_SPEED);
    float expected = 1.0f + UPGRADE_BULLET_SPEED_BONUS;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, expected, upgrade_get_bullet_speed_mult(&us));
}

void test_upgrade_dash_cd_mult(void) {
    UpgradeState us;
    upgrade_state_init(&us);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, upgrade_get_dash_cd_mult(&us));
    upgrade_add(&us, UPGRADE_DASH_CD);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, UPGRADE_DASH_CD_BONUS, upgrade_get_dash_cd_mult(&us));
}

void test_upgrade_names_not_null(void) {
    for (int i = 0; i < UPGRADE_COUNT; i++) {
        const char *name = upgrade_get_name((UpgradeType)i);
        TEST_ASSERT_NOT_NULL(name);
        TEST_ASSERT_TRUE(name[0] != '\0');
    }
}

void test_upgrade_add_invalid_type_returns_false(void) {
    UpgradeState us;
    upgrade_state_init(&us);
    TEST_ASSERT_FALSE(upgrade_add(&us, (UpgradeType)99));
    TEST_ASSERT_FALSE(upgrade_add(&us, (UpgradeType)-1));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_upgrade_init_all_zero);
    RUN_TEST(test_upgrade_add_increments);
    RUN_TEST(test_upgrade_add_caps_at_max);
    RUN_TEST(test_upgrade_speed_bonus);
    RUN_TEST(test_upgrade_damage_bonus);
    RUN_TEST(test_upgrade_fire_rate_mult);
    RUN_TEST(test_upgrade_max_hp_bonus);
    RUN_TEST(test_upgrade_bullet_speed_mult);
    RUN_TEST(test_upgrade_dash_cd_mult);
    RUN_TEST(test_upgrade_names_not_null);
    RUN_TEST(test_upgrade_add_invalid_type_returns_false);
    return UNITY_END();
}
