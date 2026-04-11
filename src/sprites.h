/*
 * sprites.h -- Hardcoded pixel art sprite data and drawing
 *
 * Sprites are defined as 2D arrays of palette indices. Each sprite type has
 * a base image; directional sprites are rotated/flipped at draw time.
 * Palette index 0 is always transparent.
 */

#pragma once

#include "raylib.h"

/* ── Constants ─────────────────────────────────────────────────────────────── */

/* Player sprite: 16x16 */
#define PLAYER_SPRITE_W 16
#define PLAYER_SPRITE_H 16

/* Swarmer sprite: 10x10 */
#define SWARMER_SPRITE_W 10
#define SWARMER_SPRITE_H 10

/* Grunt sprite: 12x12 */
#define GRUNT_SPRITE_W 12
#define GRUNT_SPRITE_H 12

/* Stalker sprite: 10x10 */
#define STALKER_SPRITE_W 10
#define STALKER_SPRITE_H 10

/* Bomber sprite: 14x14 */
#define BOMBER_SPRITE_W 14
#define BOMBER_SPRITE_H 14

/* ── Public API ────────────────────────────────────────────────────────────── */

/*
 * sprite_draw_player -- Draw the player sprite centered at `pos`.
 * `aim` is the normalized aim direction for rotation.
 * `is_dashing` changes the color palette.
 */
void sprite_draw_player(Vector2 pos, Vector2 aim, bool is_dashing);

/*
 * sprite_draw_swarmer -- Draw a swarmer enemy at `pos`.
 * `move_dir` is the normalized movement direction.
 * `hit_flash` > 0 draws the sprite white for damage feedback.
 */
void sprite_draw_swarmer(Vector2 pos, Vector2 move_dir, float hit_flash);

/* sprite_draw_grunt -- Draw a grunt enemy at `pos`. */
void sprite_draw_grunt(Vector2 pos, Vector2 move_dir, float hit_flash);

/* sprite_draw_stalker -- Draw a stalker enemy at `pos`. */
void sprite_draw_stalker(Vector2 pos, Vector2 move_dir, float hit_flash);

/* sprite_draw_bomber -- Draw a bomber enemy at `pos`. `is_charging` uses a brighter palette. */
void sprite_draw_bomber(Vector2 pos, Vector2 move_dir, float hit_flash, bool is_charging);
