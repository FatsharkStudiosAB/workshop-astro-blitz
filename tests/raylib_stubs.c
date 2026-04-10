/*
 * raylib_stubs.c -- Fake Raylib implementations for headless integration tests
 *
 * Every Raylib function called by game source files is provided here as a
 * controllable stub or silent no-op.  Link this instead of libraylib to run
 * game_update() and friends without a window or audio device.
 *
 * Rendering functions are no-ops.  Input / time / utility functions return
 * values controlled via the stub_* API declared in raylib_stubs.h.
 */

#include "raylib_stubs.h"
#include "raylib.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* ── Internal state ───────────────────────────────────────────────────────── */

#define MAX_KEYS 512
#define MAX_MOUSE_BUTTONS 8

static bool s_keys_down[MAX_KEYS];
static bool s_keys_pressed[MAX_KEYS];
static bool s_mouse_buttons[MAX_MOUSE_BUTTONS];
static Vector2 s_mouse_position;
static float s_frame_time;
static int s_random_value;
static int s_screen_width;
static int s_screen_height;

/* ── Stub control API (called by test code) ───────────────────────────────── */

void stub_reset(void) {
    memset(s_keys_down, 0, sizeof(s_keys_down));
    memset(s_keys_pressed, 0, sizeof(s_keys_pressed));
    memset(s_mouse_buttons, 0, sizeof(s_mouse_buttons));
    s_mouse_position = (Vector2){0.0f, 0.0f};
    s_frame_time = 1.0f / 60.0f; /* default 60 fps */
    s_random_value = 0;
    s_screen_width = 800;
    s_screen_height = 600;
}

void stub_set_key_down(int key, bool down) {
    if (key >= 0 && key < MAX_KEYS) {
        s_keys_down[key] = down;
    }
}

void stub_set_key_pressed(int key, bool pressed) {
    if (key >= 0 && key < MAX_KEYS) {
        s_keys_pressed[key] = pressed;
    }
}

void stub_set_mouse_button_down(int button, bool down) {
    if (button >= 0 && button < MAX_MOUSE_BUTTONS) {
        s_mouse_buttons[button] = down;
    }
}

void stub_set_mouse_position(float x, float y) {
    s_mouse_position = (Vector2){x, y};
}

void stub_set_frame_time(float dt) {
    s_frame_time = dt;
}

void stub_set_random_value(int value) {
    s_random_value = value;
}

void stub_set_screen_size(int width, int height) {
    s_screen_width = width;
    s_screen_height = height;
}

/* ── Input stubs ──────────────────────────────────────────────────────────── */

bool IsKeyDown(int key) {
    if (key >= 0 && key < MAX_KEYS) {
        return s_keys_down[key];
    }
    return false;
}

bool IsKeyPressed(int key) {
    if (key >= 0 && key < MAX_KEYS) {
        bool val = s_keys_pressed[key];
        s_keys_pressed[key] = false; /* consumed after one read */
        return val;
    }
    return false;
}

bool IsMouseButtonDown(int button) {
    if (button >= 0 && button < MAX_MOUSE_BUTTONS) {
        return s_mouse_buttons[button];
    }
    return false;
}

Vector2 GetMousePosition(void) {
    return s_mouse_position;
}

Vector2 GetScreenToWorld2D(Vector2 position, Camera2D camera) {
    /* Simplified inverse: undo offset and zoom.  Good enough for tests
     * where camera zoom is 1.0 and offset is screen center. */
    float zoom = (camera.zoom != 0.0f) ? camera.zoom : 1.0f;
    Vector2 result;
    result.x = (position.x - camera.offset.x) / zoom + camera.target.x;
    result.y = (position.y - camera.offset.y) / zoom + camera.target.y;
    return result;
}

/* ── Time / utility stubs ─────────────────────────────────────────────────── */

float GetFrameTime(void) {
    return s_frame_time;
}

int GetRandomValue(int min, int max) {
    /* Clamp the stub value into the requested range so callers that
     * depend on valid bounds (e.g. spawn_wave edge selection) still
     * get a usable result. */
    if (s_random_value < min) {
        return min;
    }
    if (s_random_value > max) {
        return max;
    }
    return s_random_value;
}

int GetScreenWidth(void) {
    return s_screen_width;
}

int GetScreenHeight(void) {
    return s_screen_height;
}

/* ── Window / lifecycle stubs (no-ops) ────────────────────────────────────── */

void InitWindow(int width, int height, const char *title) {
    (void)width;
    (void)height;
    (void)title;
}

void CloseWindow(void) {
}

void SetTargetFPS(int fps) {
    (void)fps;
}

bool WindowShouldClose(void) {
    return false;
}

/* ── Audio stubs (no-ops) ─────────────────────────────────────────────────── */

void InitAudioDevice(void) {
}

void CloseAudioDevice(void) {
}

Sound LoadSoundFromWave(Wave wave) {
    (void)wave;
    Sound s = {0};
    return s;
}

void UnloadWave(Wave wave) {
    (void)wave;
}

void UnloadSound(Sound sound) {
    (void)sound;
}

void SetMasterVolume(float volume) {
    (void)volume;
}

void PlaySound(Sound sound) {
    (void)sound;
}

void StopSound(Sound sound) {
    (void)sound;
}

bool IsSoundPlaying(Sound sound) {
    (void)sound;
    return false;
}

/* ── Rendering stubs (no-ops) ─────────────────────────────────────────────── */

void BeginDrawing(void) {
}

void EndDrawing(void) {
}

void ClearBackground(Color color) {
    (void)color;
}

void BeginMode2D(Camera2D camera) {
    (void)camera;
}

void EndMode2D(void) {
}

void DrawPixel(int posX, int posY, Color color) {
    (void)posX;
    (void)posY;
    (void)color;
}

void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color) {
    (void)startPos;
    (void)endPos;
    (void)thick;
    (void)color;
}

void DrawCircleV(Vector2 center, float radius, Color color) {
    (void)center;
    (void)radius;
    (void)color;
}

void DrawCircleLinesV(Vector2 center, float radius, Color color) {
    (void)center;
    (void)radius;
    (void)color;
}

void DrawRectangle(int posX, int posY, int width, int height, Color color) {
    (void)posX;
    (void)posY;
    (void)width;
    (void)height;
    (void)color;
}

void DrawRectangleLines(int posX, int posY, int width, int height, Color color) {
    (void)posX;
    (void)posY;
    (void)width;
    (void)height;
    (void)color;
}

void DrawTriangle(Vector2 v1, Vector2 v2, Vector2 v3, Color color) {
    (void)v1;
    (void)v2;
    (void)v3;
    (void)color;
}

void DrawText(const char *text, int posX, int posY, int fontSize, Color color) {
    (void)text;
    (void)posX;
    (void)posY;
    (void)fontSize;
    (void)color;
}

int MeasureText(const char *text, int fontSize) {
    (void)text;
    (void)fontSize;
    return 0;
}

const char *TextFormat(const char *text, ...) {
    /* Return a static buffer with formatted text -- enough for tests. */
    static char buffer[256];
    va_list args;
    va_start(args, text);
    vsnprintf(buffer, sizeof(buffer), text, args);
    va_end(args);
    return buffer;
}
