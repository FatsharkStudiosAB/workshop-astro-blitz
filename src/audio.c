/*
 * audio.c -- Procedural audio generation and playback
 *
 * Generates all sounds at runtime using Raylib's wave synthesis functions.
 * No external audio files are needed.
 */

#include "audio.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ── Internal helpers ──────────────────────────────────────────────────────── */

/* Generate a short shoot sound: a descending square wave burst with fast
 * attack and exponential decay -- punchy arcade feel. */
static Wave generate_shoot_wave(void) {
    int sample_count = (int)(SHOOT_SFX_DURATION * SHOOT_SFX_SAMPLE_RATE);
    float *samples = (float *)malloc((size_t)sample_count * sizeof(float));

    for (int i = 0; i < sample_count; i++) {
        float t = (float)i / (float)SHOOT_SFX_SAMPLE_RATE;
        float progress = t / SHOOT_SFX_DURATION;

        /* Descending frequency: start at 880 Hz, sweep down to 440 Hz */
        float freq = SHOOT_SFX_FREQ * (1.0f - 0.5f * progress);

        /* Square wave (sign of sine) */
        float phase = 2.0f * (float)M_PI * freq * t;
        float wave = sinf(phase) >= 0.0f ? 1.0f : -1.0f;

        /* Exponential decay envelope */
        float envelope = expf(-8.0f * progress);

        samples[i] = wave * envelope * 0.4f;
    }

    Wave wav = {0};
    wav.frameCount = (unsigned int)sample_count;
    wav.sampleRate = SHOOT_SFX_SAMPLE_RATE;
    wav.sampleSize = 32; /* 32-bit float */
    wav.channels = 1;
    wav.data = samples;

    return wav;
}

/* Generate a simple synth melody for the death screen.
 * Sci-fi style: detuned dual oscillators, low drone layer, dissonant intervals
 * (tritones, minor 2nds), slow LFO wobble, and filtered saw-like timbre. */
static Wave generate_death_music_wave(void) {
    int sample_count = (int)(DEATH_MUSIC_DURATION * DEATH_MUSIC_SAMPLE_RATE);
    float *samples = (float *)malloc((size_t)sample_count * sizeof(float));

    /* Melody notes (Hz): eerie sci-fi sequence using tritones and minor intervals.
     * Bb2, E3, Bb3, B3 (cluster), F3, Bb2, E3, C3 */
    float notes[] = {116.54f, 164.81f, 233.08f, 246.94f,
                     174.61f, 116.54f, 164.81f, 130.81f};
    int note_count = 8;
    float note_duration = DEATH_MUSIC_DURATION / (float)note_count;

    /* Low drone frequency -- sits underneath everything */
    float drone_freq = 55.0f; /* A1 */

    for (int i = 0; i < sample_count; i++) {
        float t = (float)i / (float)DEATH_MUSIC_SAMPLE_RATE;

        /* ── Drone layer: detuned dual saws ────────────────────────────── */
        float drone_phase1 = 2.0f * (float)M_PI * drone_freq * t;
        float drone_phase2 = 2.0f * (float)M_PI * (drone_freq * 1.005f) * t;
        /* Approximate saw wave from harmonics */
        float drone = 0.0f;
        for (int h = 1; h <= 6; h++) {
            float hf = (float)h;
            drone += sinf(drone_phase1 * hf) / hf;
            drone += sinf(drone_phase2 * hf) / hf;
        }
        drone *= 0.15f;

        /* ── Melody layer ──────────────────────────────────────────────── */
        int note_index = (int)(t / note_duration);
        if (note_index >= note_count) {
            note_index = note_count - 1;
        }
        float freq = notes[note_index];

        float note_t = t - (float)note_index * note_duration;
        float note_progress = note_t / note_duration;

        /* Detuned dual oscillators for thick synth sound */
        float phase_a = 2.0f * (float)M_PI * freq * t;
        float phase_b = 2.0f * (float)M_PI * (freq * 1.008f) * t;

        /* Saw-like timbre from first 8 harmonics */
        float melody = 0.0f;
        for (int h = 1; h <= 8; h++) {
            float hf = (float)h;
            /* Simple low-pass: attenuate higher harmonics */
            float lp = 1.0f / (1.0f + hf * 0.3f);
            melody += sinf(phase_a * hf) * lp / hf;
            melody += sinf(phase_b * hf) * lp / hf;
        }
        melody *= 0.2f;

        /* Per-note envelope: slow attack, sustain, fade out */
        float attack = note_progress < 0.1f ? note_progress / 0.1f : 1.0f;
        float release = note_progress > 0.7f ? (1.0f - note_progress) / 0.3f : 1.0f;
        float envelope = attack * release;

        /* Slow LFO wobble on amplitude */
        float lfo = 1.0f + 0.15f * sinf(2.0f * (float)M_PI * 0.8f * t);

        melody *= envelope * lfo;

        /* ── Mix ───────────────────────────────────────────────────────── */
        /* Global slow fade-in over the first second for atmosphere */
        float global_fade = t < 1.0f ? t : 1.0f;

        samples[i] = (drone + melody) * global_fade * 0.35f;
    }

    Wave wav = {0};
    wav.frameCount = (unsigned int)sample_count;
    wav.sampleRate = DEATH_MUSIC_SAMPLE_RATE;
    wav.sampleSize = 32; /* 32-bit float */
    wav.channels = 1;
    wav.data = samples;

    return wav;
}

/* Generate an impact/crunch sound for enemy-player collisions.
 * Low-frequency thud with noise burst and rapid decay. */
static Wave generate_hit_wave(void) {
    int sample_count = (int)(HIT_SFX_DURATION * HIT_SFX_SAMPLE_RATE);
    float *samples = (float *)malloc((size_t)sample_count * sizeof(float));

    for (int i = 0; i < sample_count; i++) {
        float t = (float)i / (float)HIT_SFX_SAMPLE_RATE;
        float progress = t / HIT_SFX_DURATION;

        /* Descending low thud: 120 Hz down to 40 Hz */
        float freq = HIT_SFX_FREQ * (1.0f - 0.67f * progress);
        float phase = 2.0f * (float)M_PI * freq * t;

        /* Mix sine body with distorted square for crunch */
        float body = sinf(phase) * 0.6f;
        float crunch = (sinf(phase * 3.0f) >= 0.0f ? 1.0f : -1.0f) * 0.25f;

        /* Simple noise component (deterministic pseudo-random via large prime) */
        float noise = sinf((float)i * 12345.6789f) * 0.2f;

        /* Fast exponential decay */
        float envelope = expf(-12.0f * progress);

        samples[i] = (body + crunch + noise) * envelope * 0.5f;
    }

    Wave wav = {0};
    wav.frameCount = (unsigned int)sample_count;
    wav.sampleRate = HIT_SFX_SAMPLE_RATE;
    wav.sampleSize = 32;
    wav.channels = 1;
    wav.data = samples;

    return wav;
}

/* Generate a short metallic ping for when a bullet hits an enemy.
 * High-frequency sine with a sharp attack and ringing decay. */
static Wave generate_enemy_hit_wave(void) {
    int sample_count = (int)(ENEMY_HIT_SFX_DURATION * ENEMY_HIT_SFX_SAMPLE_RATE);
    float *samples = (float *)malloc((size_t)sample_count * sizeof(float));

    for (int i = 0; i < sample_count; i++) {
        float t = (float)i / (float)ENEMY_HIT_SFX_SAMPLE_RATE;
        float progress = t / ENEMY_HIT_SFX_DURATION;

        /* Slightly descending frequency for a "splat" feel */
        float freq = ENEMY_HIT_SFX_FREQ * (1.0f - 0.3f * progress);
        float phase = 2.0f * (float)M_PI * freq * t;

        /* Metallic ring: fundamental + inharmonic overtone */
        float ring = sinf(phase) * 0.5f + sinf(phase * 2.7f) * 0.3f;

        /* Sharp attack, ringing decay */
        float envelope = expf(-15.0f * progress);

        samples[i] = ring * envelope * 0.35f;
    }

    Wave wav = {0};
    wav.frameCount = (unsigned int)sample_count;
    wav.sampleRate = ENEMY_HIT_SFX_SAMPLE_RATE;
    wav.sampleSize = 32;
    wav.channels = 1;
    wav.data = samples;

    return wav;
}

/* ── Public ────────────────────────────────────────────────────────────────── */

void audio_init(GameAudio *audio) {
    memset(audio, 0, sizeof(*audio));

    /* Generate and load shoot SFX */
    Wave shoot_wav = generate_shoot_wave();
    audio->shoot_sfx = LoadSoundFromWave(shoot_wav);
    UnloadWave(shoot_wav);

    /* Generate and load hit SFX */
    Wave hit_wav = generate_hit_wave();
    audio->hit_sfx = LoadSoundFromWave(hit_wav);
    UnloadWave(hit_wav);

    /* Generate and load enemy-hit SFX */
    Wave enemy_hit_wav = generate_enemy_hit_wave();
    audio->enemy_hit_sfx = LoadSoundFromWave(enemy_hit_wav);
    UnloadWave(enemy_hit_wav);

    /* Generate and load death music as a Sound (replayed manually for looping) */
    Wave death_wav = generate_death_music_wave();
    audio->death_music = LoadSoundFromWave(death_wav);
    UnloadWave(death_wav);

    SetMasterVolume(AUDIO_MASTER_VOLUME);

    audio->death_music_playing = false;
    audio->initialized = true;
}

void audio_cleanup(GameAudio *audio) {
    if (!audio->initialized) {
        return;
    }

    UnloadSound(audio->shoot_sfx);
    UnloadSound(audio->hit_sfx);
    UnloadSound(audio->enemy_hit_sfx);
    UnloadSound(audio->death_music);

    audio->initialized = false;
    audio->death_music_playing = false;
}

void audio_update(GameAudio *audio) {
    if (!audio->initialized) {
        return;
    }

    /* Manual looping: restart the death music sound when it finishes */
    if (audio->death_music_playing && !IsSoundPlaying(audio->death_music)) {
        PlaySound(audio->death_music);
    }
}

void audio_play_shoot(GameAudio *audio) {
    if (!audio->initialized) {
        return;
    }

    PlaySound(audio->shoot_sfx);
}

void audio_play_hit(GameAudio *audio) {
    if (!audio->initialized) {
        return;
    }

    PlaySound(audio->hit_sfx);
}

void audio_play_enemy_hit(GameAudio *audio) {
    if (!audio->initialized) {
        return;
    }

    PlaySound(audio->enemy_hit_sfx);
}

void audio_play_death_music(GameAudio *audio) {
    if (!audio->initialized || audio->death_music_playing) {
        return;
    }

    PlaySound(audio->death_music);
    audio->death_music_playing = true;
}

void audio_stop_death_music(GameAudio *audio) {
    if (!audio->initialized || !audio->death_music_playing) {
        return;
    }

    StopSound(audio->death_music);
    audio->death_music_playing = false;
}
