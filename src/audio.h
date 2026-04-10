/*
 * audio.h -- Audio system for sound effects and music
 *
 * Manages procedurally generated sounds (no external asset files).
 * Call audio_init after InitAudioDevice and audio_cleanup before
 * CloseAudioDevice.
 */

#pragma once

#include "raylib.h"
#include <stdbool.h>

/* ── Constants ─────────────────────────────────────────────────────────────── */

#define SHOOT_SFX_FREQ 880.0f       /* Hz -- A5, punchy high tone */
#define SHOOT_SFX_DURATION 0.08f    /* seconds */
#define SHOOT_SFX_SAMPLE_RATE 44100 /* samples per second */

#define HIT_SFX_FREQ 120.0f       /* Hz -- low impact thud */
#define HIT_SFX_DURATION 0.15f    /* seconds */
#define HIT_SFX_SAMPLE_RATE 44100 /* samples per second */

#define ENEMY_HIT_SFX_FREQ 1200.0f   /* Hz -- bright metallic ping */
#define ENEMY_HIT_SFX_DURATION 0.10f /* seconds */
#define ENEMY_HIT_SFX_SAMPLE_RATE 44100

#define DEATH_MUSIC_SAMPLE_RATE 44100
#define DEATH_MUSIC_DURATION 8.0f /* seconds per loop */

#define AUDIO_MASTER_VOLUME 0.5f

/* ── Types ─────────────────────────────────────────────────────────────────── */

typedef struct {
    Sound shoot_sfx;          /* short burst played on bullet fire */
    Sound hit_sfx;            /* impact sound when enemy collides with player */
    Sound enemy_hit_sfx;      /* metallic ping when bullet hits enemy */
    Sound death_music;        /* looping synth melody for game-over screen */
    bool death_music_playing; /* true while death music is active */
    bool initialized;         /* true after successful audio_init */
} GameAudio;

/* ── Public API ────────────────────────────────────────────────────────────── */

/* Generate procedural sounds and load them into Raylib.
 * Requires InitAudioDevice() to have been called first. */
void audio_init(GameAudio *audio);

/* Unload all sounds. Call before CloseAudioDevice(). */
void audio_cleanup(GameAudio *audio);

/* Per-frame update -- restarts death music for seamless looping. */
void audio_update(GameAudio *audio);

/* Play the bullet-fire sound effect. */
void audio_play_shoot(GameAudio *audio);

/* Play the enemy-hit-player impact sound. */
void audio_play_hit(GameAudio *audio);

/* Play the bullet-hit-enemy sound. */
void audio_play_enemy_hit(GameAudio *audio);

/* Start looping the death-screen music. */
void audio_play_death_music(GameAudio *audio);

/* Stop the death-screen music. */
void audio_stop_death_music(GameAudio *audio);
