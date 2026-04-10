/*
 * test_audio.c -- Unit tests for the audio module
 *
 * Tests state management and guard behavior.  Actual sound playback requires
 * InitAudioDevice() which needs a real audio backend, so these tests focus on
 * the logical state machine: initialized flag, death_music_playing flag,
 * and safe no-op behavior when not initialized.
 *
 * NOTE: Tests that call audio_init/audio_cleanup are excluded here because
 * they require InitAudioDevice.  The GameAudio struct's state flags are
 * tested directly.
 */

#include "audio.h"
#include "unity.h"

#include <string.h>

/* ── setUp / tearDown ──────────────────────────────────────────────────────── */

void setUp(void) {}
void tearDown(void) {}

/* ── Zeroed struct tests ───────────────────────────────────────────────────── */

void test_zeroed_audio_not_initialized(void) {
    GameAudio audio;
    memset(&audio, 0, sizeof(audio));

    TEST_ASSERT_FALSE(audio.initialized);
}

void test_zeroed_audio_death_music_not_playing(void) {
    GameAudio audio;
    memset(&audio, 0, sizeof(audio));

    TEST_ASSERT_FALSE(audio.death_music_playing);
}

/* ── Guard: functions are no-ops when not initialized ──────────────────────── */

void test_update_safe_when_not_initialized(void) {
    GameAudio audio;
    memset(&audio, 0, sizeof(audio));

    /* Should not crash */
    audio_update(&audio);
    TEST_ASSERT_FALSE(audio.initialized);
}

void test_play_shoot_safe_when_not_initialized(void) {
    GameAudio audio;
    memset(&audio, 0, sizeof(audio));

    /* Should not crash */
    audio_play_shoot(&audio);
    TEST_ASSERT_FALSE(audio.initialized);
}

void test_play_hit_safe_when_not_initialized(void) {
    GameAudio audio;
    memset(&audio, 0, sizeof(audio));

    /* Should not crash */
    audio_play_hit(&audio);
    TEST_ASSERT_FALSE(audio.initialized);
}

void test_play_enemy_hit_safe_when_not_initialized(void) {
    GameAudio audio;
    memset(&audio, 0, sizeof(audio));

    /* Should not crash */
    audio_play_enemy_hit(&audio);
    TEST_ASSERT_FALSE(audio.initialized);
}

void test_play_death_music_safe_when_not_initialized(void) {
    GameAudio audio;
    memset(&audio, 0, sizeof(audio));

    /* Should not crash and should not set the flag */
    audio_play_death_music(&audio);
    TEST_ASSERT_FALSE(audio.death_music_playing);
}

void test_stop_death_music_safe_when_not_initialized(void) {
    GameAudio audio;
    memset(&audio, 0, sizeof(audio));

    /* Should not crash */
    audio_stop_death_music(&audio);
    TEST_ASSERT_FALSE(audio.death_music_playing);
}

void test_cleanup_safe_when_not_initialized(void) {
    GameAudio audio;
    memset(&audio, 0, sizeof(audio));

    /* Should not crash */
    audio_cleanup(&audio);
    TEST_ASSERT_FALSE(audio.initialized);
}

/* ── Guard: death music double-play/double-stop ────────────────────────────── */

void test_stop_death_music_noop_when_not_playing(void) {
    GameAudio audio;
    memset(&audio, 0, sizeof(audio));
    audio.initialized = true; /* pretend initialized for flag logic */

    /* death_music_playing is false; stop should be a no-op */
    audio_stop_death_music(&audio);
    TEST_ASSERT_FALSE(audio.death_music_playing);
}

/* ── Constants sanity checks ───────────────────────────────────────────────── */

void test_shoot_sfx_freq_positive(void) {
    TEST_ASSERT_TRUE(SHOOT_SFX_FREQ > 0.0f);
}

void test_shoot_sfx_duration_positive(void) {
    TEST_ASSERT_TRUE(SHOOT_SFX_DURATION > 0.0f);
}

void test_hit_sfx_freq_positive(void) {
    TEST_ASSERT_TRUE(HIT_SFX_FREQ > 0.0f);
}

void test_hit_sfx_duration_positive(void) {
    TEST_ASSERT_TRUE(HIT_SFX_DURATION > 0.0f);
}

void test_enemy_hit_sfx_freq_positive(void) {
    TEST_ASSERT_TRUE(ENEMY_HIT_SFX_FREQ > 0.0f);
}

void test_enemy_hit_sfx_duration_positive(void) {
    TEST_ASSERT_TRUE(ENEMY_HIT_SFX_DURATION > 0.0f);
}

void test_death_music_duration_positive(void) {
    TEST_ASSERT_TRUE(DEATH_MUSIC_DURATION > 0.0f);
}

void test_master_volume_in_range(void) {
    TEST_ASSERT_TRUE(AUDIO_MASTER_VOLUME > 0.0f);
    TEST_ASSERT_TRUE(AUDIO_MASTER_VOLUME <= 1.0f);
}

void test_sample_rates_standard(void) {
    TEST_ASSERT_EQUAL_INT(44100, SHOOT_SFX_SAMPLE_RATE);
    TEST_ASSERT_EQUAL_INT(44100, HIT_SFX_SAMPLE_RATE);
    TEST_ASSERT_EQUAL_INT(44100, ENEMY_HIT_SFX_SAMPLE_RATE);
    TEST_ASSERT_EQUAL_INT(44100, DEATH_MUSIC_SAMPLE_RATE);
}

/* ── Runner ────────────────────────────────────────────────────────────────── */

int main(void) {
    UNITY_BEGIN();

    /* Zeroed struct */
    RUN_TEST(test_zeroed_audio_not_initialized);
    RUN_TEST(test_zeroed_audio_death_music_not_playing);

    /* Guards (not initialized) */
    RUN_TEST(test_update_safe_when_not_initialized);
    RUN_TEST(test_play_shoot_safe_when_not_initialized);
    RUN_TEST(test_play_hit_safe_when_not_initialized);
    RUN_TEST(test_play_enemy_hit_safe_when_not_initialized);
    RUN_TEST(test_play_death_music_safe_when_not_initialized);
    RUN_TEST(test_stop_death_music_safe_when_not_initialized);
    RUN_TEST(test_cleanup_safe_when_not_initialized);

    /* Guards (double stop) */
    RUN_TEST(test_stop_death_music_noop_when_not_playing);

    /* Constants */
    RUN_TEST(test_shoot_sfx_freq_positive);
    RUN_TEST(test_shoot_sfx_duration_positive);
    RUN_TEST(test_hit_sfx_freq_positive);
    RUN_TEST(test_hit_sfx_duration_positive);
    RUN_TEST(test_enemy_hit_sfx_freq_positive);
    RUN_TEST(test_enemy_hit_sfx_duration_positive);
    RUN_TEST(test_death_music_duration_positive);
    RUN_TEST(test_master_volume_in_range);
    RUN_TEST(test_sample_rates_standard);

    return UNITY_END();
}
