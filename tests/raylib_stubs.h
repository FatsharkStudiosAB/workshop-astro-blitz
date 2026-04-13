/*
 * raylib_stubs.h -- Controllable Raylib stubs for headless integration testing
 *
 * Provides fake implementations of Raylib runtime functions so that game
 * logic (game_update, player_update, etc.) can run without a window or
 * audio device.  Test code controls return values via the stub_* API.
 *
 * Usage:
 *   1. Call stub_reset() in setUp() to clear all state.
 *   2. Use stub_set_* helpers to configure inputs before each game_update.
 *   3. Call game_update / player_update / etc. normally.
 *   4. Assert on game state.
 */

#pragma once

#include <stdbool.h>

/* ── Input stubs ──────────────────────────────────────────────────────────── */

/* Set a key as currently held (IsKeyDown returns true). */
void stub_set_key_down(int key, bool down);

/* Set a key as just pressed this frame (IsKeyPressed returns true once). */
void stub_set_key_pressed(int key, bool pressed);

/* Set the mouse button state (IsMouseButtonDown). */
void stub_set_mouse_button_down(int button, bool down);

/* Set a mouse button as just pressed this frame (one-shot, like keys). */
void stub_set_mouse_button_pressed(int button, bool pressed);

/* Set the mouse position returned by GetMousePosition. */
void stub_set_mouse_position(float x, float y);

/* ── Time / utility stubs ─────────────────────────────────────────────────── */

/* Set the value returned by GetFrameTime. */
void stub_set_frame_time(float dt);

/* Set the next value returned by GetRandomValue (repeats until changed). */
void stub_set_random_value(int value);

/* Set the screen dimensions returned by GetScreenWidth/Height. */
void stub_set_screen_size(int width, int height);

/* ── Lifecycle ────────────────────────────────────────────────────────────── */

/* Reset all stub state to defaults.  Call in setUp(). */
void stub_reset(void);
