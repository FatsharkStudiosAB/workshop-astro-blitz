/*
 * game.c -- Top-level game state, update loop, and draw
 */

#include "game.h"
#include <math.h>
#include <string.h>

/* ── Constants ─────────────────────────────────────────────────────────────── */

/* Maximum attempts per enemy to find a walkable spawn position */
#define SPAWN_RETRIES 5

/* Hit flash duration in seconds */
#define HIT_FLASH_DURATION 0.1f

/* ── Damage number helpers ─────────────────────────────────────────────────── */

static void damage_number_pool_init(DamageNumberPool *pool) {
    memset(pool->numbers, 0, sizeof(pool->numbers));
}

static void damage_number_spawn(DamageNumberPool *pool, Vector2 pos, int value, Color color) {
    for (int i = 0; i < MAX_DAMAGE_NUMBERS; i++) {
        DamageNumber *dn = &pool->numbers[i];
        if (!dn->active) {
            dn->position = pos;
            dn->value = value;
            dn->color = color;
            dn->lifetime = DAMAGE_NUMBER_LIFETIME;
            dn->max_lifetime = DAMAGE_NUMBER_LIFETIME;
            dn->active = true;
            return;
        }
    }
}

static void damage_number_pool_update(DamageNumberPool *pool, float dt) {
    for (int i = 0; i < MAX_DAMAGE_NUMBERS; i++) {
        DamageNumber *dn = &pool->numbers[i];
        if (!dn->active) {
            continue;
        }
        dn->position.y -= DAMAGE_NUMBER_RISE_SPEED * dt;
        dn->lifetime -= dt;
        if (dn->lifetime <= 0.0f) {
            dn->active = false;
        }
    }
}

static void damage_number_pool_draw(const DamageNumberPool *pool) {
    for (int i = 0; i < MAX_DAMAGE_NUMBERS; i++) {
        const DamageNumber *dn = &pool->numbers[i];
        if (!dn->active) {
            continue;
        }
        float ratio = dn->lifetime / dn->max_lifetime;
        unsigned char alpha = (unsigned char)(ratio * 255.0f);
        Color c = dn->color;
        c.a = alpha;
        const char *text = TextFormat("%d", dn->value);
        int font_size = 16;
        int w = MeasureText(text, font_size);
        DrawText(text, (int)(dn->position.x - w / 2.0f), (int)dn->position.y, font_size, c);
    }
}

/* ── Weapon pickup helpers ──────────────────────────────────────────────────── */

static void weapon_pickup_pool_init(WeaponPickupPool *pool) {
    memset(pool->pickups, 0, sizeof(pool->pickups));
}

static void weapon_pickup_spawn(WeaponPickupPool *pool, Vector2 pos, Weapon weapon) {
    for (int i = 0; i < MAX_WEAPON_PICKUPS; i++) {
        WeaponPickup *wp = &pool->pickups[i];
        if (!wp->active) {
            wp->position = pos;
            wp->weapon = weapon;
            wp->lifetime = WEAPON_PICKUP_LIFETIME;
            wp->active = true;
            return;
        }
    }
}

static void weapon_pickup_pool_update(WeaponPickupPool *pool, float dt) {
    for (int i = 0; i < MAX_WEAPON_PICKUPS; i++) {
        WeaponPickup *wp = &pool->pickups[i];
        if (!wp->active) {
            continue;
        }
        wp->lifetime -= dt;
        if (wp->lifetime <= 0.0f) {
            wp->active = false;
        }
    }
}

static void weapon_pickup_pool_draw(const WeaponPickupPool *pool) {
    for (int i = 0; i < MAX_WEAPON_PICKUPS; i++) {
        const WeaponPickup *wp = &pool->pickups[i];
        if (!wp->active) {
            continue;
        }

        /* Pulsing glow effect based on lifetime */
        float pulse = 0.5f + 0.5f * sinf(wp->lifetime * 4.0f);
        unsigned char glow_alpha = (unsigned char)(40.0f + 40.0f * pulse);
        Color glow = wp->weapon.bullet_color;
        glow.a = glow_alpha;

        /* Fade out in last 3 seconds */
        float alpha_mult = 1.0f;
        if (wp->lifetime < 3.0f) {
            alpha_mult = wp->lifetime / 3.0f;
        }

        Color outline = wp->weapon.bullet_color;
        outline.a = (unsigned char)(255.0f * alpha_mult);
        Color fill = {outline.r / 4, outline.g / 4, outline.b / 4, outline.a};

        /* Draw pickup: outer glow + diamond shape + weapon initial */
        DrawCircleV(wp->position, WEAPON_PICKUP_RADIUS + 4.0f, glow);
        DrawCircleV(wp->position, WEAPON_PICKUP_RADIUS, fill);
        DrawCircleLinesV(wp->position, WEAPON_PICKUP_RADIUS, outline);

        /* Weapon initial letter */
        const char *initial = "?";
        if (wp->weapon.name && wp->weapon.name[0]) {
            initial = TextFormat("%c", wp->weapon.name[0]);
        }
        int font_size = 12;
        int w = MeasureText(initial, font_size);
        Color text_color = outline;
        DrawText(initial, (int)(wp->position.x - w / 2.0f),
                 (int)(wp->position.y - font_size / 2.0f), font_size, text_color);
    }
}

/* ── Helpers ────────────────────────────────────────────────────────────────── */

/* Pick an enemy type based on wave number. Later waves introduce harder types. */
static EnemyType pick_enemy_type(int waves_spawned) {
    /* Wave 0-2: only swarmers */
    if (waves_spawned < 3) {
        return ENEMY_SWARMER;
    }
    /* Wave 3-5: introduce grunts (30% chance) */
    if (waves_spawned < 6) {
        return (GetRandomValue(1, 10) <= 3) ? ENEMY_GRUNT : ENEMY_SWARMER;
    }
    /* Wave 6-9: introduce stalkers */
    if (waves_spawned < 10) {
        int roll = GetRandomValue(1, 10);
        if (roll <= 2) {
            return ENEMY_STALKER;
        }
        if (roll <= 5) {
            return ENEMY_GRUNT;
        }
        return ENEMY_SWARMER;
    }
    /* Wave 10+: all types including bombers */
    int roll = GetRandomValue(1, 10);
    if (roll <= 1) {
        return ENEMY_BOMBER;
    }
    if (roll <= 3) {
        return ENEMY_STALKER;
    }
    if (roll <= 5) {
        return ENEMY_GRUNT;
    }
    return ENEMY_SWARMER;
}

/* Spawn a wave of enemies just outside the camera viewport */
static void spawn_wave(GameState *gs) {
    int count = GetRandomValue(SPAWN_MIN_GROUP, SPAWN_MAX_GROUP);

    /* Calculate viewport edges in world space from the active camera */
    float screen_w = (float)GetScreenWidth();
    float screen_h = (float)GetScreenHeight();
    float zoom = (gs->camera.zoom != 0.0f) ? gs->camera.zoom : 1.0f;
    float cam_x = gs->camera.target.x;
    float cam_y = gs->camera.target.y;

    /* Spawn margin: a little outside the visible area */
    float margin = 40.0f;
    float left = cam_x - (gs->camera.offset.x / zoom) - margin;
    float right = cam_x + ((screen_w - gs->camera.offset.x) / zoom) + margin;
    float top = cam_y - (gs->camera.offset.y / zoom) - margin;
    float bottom = cam_y + ((screen_h - gs->camera.offset.y) / zoom) + margin;

    /* Clamp to the walkable interior, not the solid border-wall tiles */
    Rectangle wb = gs->arena;
    float spawn_padding = 10.0f;
    float min_spawn_x = wb.x + TILE_SIZE + spawn_padding;
    float max_spawn_x = wb.x + wb.width - TILE_SIZE - spawn_padding;
    float min_spawn_y = wb.y + TILE_SIZE + spawn_padding;
    float max_spawn_y = wb.y + wb.height - TILE_SIZE - spawn_padding;

    if (left < min_spawn_x) {
        left = min_spawn_x;
    }
    if (right > max_spawn_x) {
        right = max_spawn_x;
    }
    if (top < min_spawn_y) {
        top = min_spawn_y;
    }
    if (bottom > max_spawn_y) {
        bottom = max_spawn_y;
    }

    for (int i = 0; i < count; i++) {
        for (int attempt = 0; attempt < SPAWN_RETRIES; attempt++) {
            Vector2 pos;
            int edge = GetRandomValue(0, 3);

            switch (edge) {
            case 0: /* top */
                pos.x = (float)GetRandomValue((int)left, (int)right);
                pos.y = top;
                break;
            case 1: /* bottom */
                pos.x = (float)GetRandomValue((int)left, (int)right);
                pos.y = bottom;
                break;
            case 2: /* left */
                pos.x = left;
                pos.y = (float)GetRandomValue((int)top, (int)bottom);
                break;
            default: /* right */
                pos.x = right;
                pos.y = (float)GetRandomValue((int)top, (int)bottom);
                break;
            }

            /* Spawn if the position is walkable; otherwise retry */
            if (!tilemap_is_solid(&gs->tilemap, pos.x, pos.y)) {
                EnemyType type = pick_enemy_type(gs->stats.waves_spawned);

                /* Roll for elite modifier after threshold waves */
                EliteModifier elite = ELITE_NONE;
                if (gs->stats.waves_spawned >= ELITE_WAVE_THRESHOLD &&
                    GetRandomValue(1, 100) <= ELITE_CHANCE) {
                    elite = (EliteModifier)(1 + GetRandomValue(0, 2));
                    /* ELITE_ARMORED=1, ELITE_SWIFT=2, ELITE_BURNING=3 */
                }

                if (elite != ELITE_NONE) {
                    enemy_pool_spawn_elite(&gs->enemies, type, pos, elite);
                } else {
                    enemy_pool_spawn(&gs->enemies, type, pos);
                }
                break;
            }
        }
    }
}

static void resolve_bullet_enemy_collisions(GameState *gs) {
    BulletPool *bp = &gs->bullets;
    EnemyPool *ep = &gs->enemies;

    for (int b = 0; b < MAX_BULLETS; b++) {
        Bullet *bullet = &bp->bullets[b];
        if (!bullet->active) {
            continue;
        }

        for (int e = 0; e < MAX_ENEMIES; e++) {
            Enemy *enemy = &ep->enemies[e];
            if (!enemy->active) {
                continue;
            }

            if (check_circle_collision(bullet->position, BULLET_RADIUS, enemy->position,
                                       enemy->radius)) {
                int dmg = (int)bullet->damage;
                if (dmg < 1) {
                    dmg = 1;
                }
                bullet->active = false;
                enemy->hp -= bullet->damage;
                enemy->hit_flash = HIT_FLASH_DURATION;
                audio_play_enemy_hit(&gs->audio);

                /* Hit sparks */
                particle_burst(&gs->particles, bullet->position, 6, 40.0f, 120.0f, 0.1f, 0.3f, 2.5f,
                               (Color){255, 200, 50, 255});

                /* Damage number */
                damage_number_spawn(&gs->damage_numbers, enemy->position, dmg,
                                    (Color){255, 255, 100, 255});

                if (enemy->hp <= 0.0f) {
                    enemy->active = false;
                    gs->stats.kills++;

                    /* Combo system */
                    gs->combo.count++;
                    gs->combo.timer = COMBO_TIMEOUT;
                    gs->combo.display_timer = COMBO_DISPLAY_DURATION;
                    if (gs->combo.count > gs->combo.best) {
                        gs->combo.best = gs->combo.count;
                    }

                    /* Escalating screen shake with combo */
                    float shake_amount = 0.15f + 0.02f * (float)gs->combo.count;
                    if (shake_amount > 0.5f) {
                        shake_amount = 0.5f;
                    }
                    screenshake_add_trauma(&gs->shake, shake_amount);

                    /* Death explosion particles */
                    particle_burst(&gs->particles, enemy->position, 15, 30.0f, 150.0f, 0.2f, 0.6f,
                                   3.5f, (Color){255, 60, 30, 255});
                    particle_burst(&gs->particles, enemy->position, 8, 20.0f, 80.0f, 0.3f, 0.8f,
                                   2.0f, (Color){255, 160, 40, 200});

                    /* Bomber AoE explosion on death */
                    if (enemy->type == ENEMY_BOMBER) {
                        screenshake_add_trauma(&gs->shake, 0.4f);
                        particle_burst(&gs->particles, enemy->position, 20, 50.0f, 200.0f, 0.3f,
                                       0.8f, 5.0f, (Color){255, 120, 20, 255});
                        /* Damage player if within blast radius */
                        float dx = gs->player.position.x - enemy->position.x;
                        float dy = gs->player.position.y - enemy->position.y;
                        float dist_sq = dx * dx + dy * dy;
                        if (dist_sq < BOMBER_EXPLOSION_RADIUS * BOMBER_EXPLOSION_RADIUS) {
                            gs->player.hp -= BOMBER_EXPLOSION_DAMAGE;
                            damage_number_spawn(&gs->damage_numbers, gs->player.position,
                                                (int)BOMBER_EXPLOSION_DAMAGE,
                                                (Color){255, 60, 60, 255});
                            if (gs->player.hp < 0.0f) {
                                gs->player.hp = 0.0f;
                            }
                        }
                    }

                    /* Weapon drop (non-swarmers have a chance) */
                    if (enemy->type != ENEMY_SWARMER &&
                        GetRandomValue(1, 100) <= WEAPON_DROP_CHANCE) {
                        /* Drop a random weapon that differs from current */
                        WeaponType drop = (WeaponType)GetRandomValue(0, WEAPON_COUNT - 1);
                        if (drop == gs->player.current_weapon.type) {
                            drop = (WeaponType)((drop + 1) % WEAPON_COUNT);
                        }
                        weapon_pickup_spawn(&gs->weapon_pickups, enemy->position,
                                            weapon_get_preset(drop));
                    }
                }
                break; /* bullet consumed -- stop checking enemies */
            }
        }
    }
}

static void resolve_enemy_player_collisions(GameState *gs) {
    Player *p = &gs->player;
    EnemyPool *ep = &gs->enemies;

    /* Skip damage if player is dashing and invincible */
    if (DASH_INVINCIBLE && p->is_dashing) {
        return;
    }

    for (int e = 0; e < MAX_ENEMIES; e++) {
        Enemy *enemy = &ep->enemies[e];
        if (!enemy->active) {
            continue;
        }

        if (check_circle_collision(p->position, PLAYER_RADIUS, enemy->position, enemy->radius)) {
            p->hp -= enemy->damage;
            enemy->active = false; /* swarmer dies on contact */
            audio_play_hit(&gs->audio);
            screenshake_add_trauma(&gs->shake, 0.35f);

            /* Damage number on player */
            damage_number_spawn(&gs->damage_numbers, p->position, (int)enemy->damage,
                                (Color){255, 60, 60, 255});

            /* Impact particles */
            particle_burst(&gs->particles, enemy->position, 10, 30.0f, 120.0f, 0.15f, 0.4f, 3.0f,
                           (Color){255, 40, 20, 255});

            if (p->hp < 0.0f) {
                p->hp = 0.0f;
            }
        }
    }
}

static void resolve_enemy_bullet_player_collisions(GameState *gs) {
    Player *p = &gs->player;
    EnemyBulletPool *ebp = &gs->enemy_bullets;

    /* Skip if dashing and invincible */
    if (DASH_INVINCIBLE && p->is_dashing) {
        return;
    }

    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        EnemyBullet *b = &ebp->bullets[i];
        if (!b->active) {
            continue;
        }

        if (check_circle_collision(b->position, 4.0f, p->position, PLAYER_RADIUS)) {
            b->active = false;
            p->hp -= b->damage;
            audio_play_hit(&gs->audio);
            screenshake_add_trauma(&gs->shake, 0.2f);

            damage_number_spawn(&gs->damage_numbers, p->position, (int)b->damage,
                                (Color){255, 60, 60, 255});

            particle_burst(&gs->particles, b->position, 6, 30.0f, 100.0f, 0.1f, 0.3f, 2.5f,
                           (Color){255, 80, 80, 255});

            if (p->hp < 0.0f) {
                p->hp = 0.0f;
            }
        }
    }
}

static void resolve_melee_enemy_collisions(GameState *gs) {
    Player *p = &gs->player;
    EnemyPool *ep = &gs->enemies;

    /* Only check during active swing */
    if (p->melee_timer <= 0.0f) {
        return;
    }

    float half_arc = (MELEE_ARC_DEGREES * 0.5f) * DEG2RAD;
    float melee_range = MELEE_RANGE + PLAYER_RADIUS;

    for (int e = 0; e < MAX_ENEMIES; e++) {
        Enemy *enemy = &ep->enemies[e];
        if (!enemy->active) {
            continue;
        }

        /* Distance check */
        Vector2 diff = Vector2Subtract(enemy->position, p->position);
        float dist = Vector2Length(diff);
        if (dist > melee_range + enemy->radius || dist < 0.01f) {
            continue;
        }

        /* Arc angle check */
        Vector2 to_enemy = Vector2Scale(diff, 1.0f / dist);
        float dot = p->melee_direction.x * to_enemy.x + p->melee_direction.y * to_enemy.y;
        float angle = acosf(dot < -1.0f ? -1.0f : (dot > 1.0f ? 1.0f : dot));
        if (angle > half_arc) {
            continue;
        }

        /* Hit! */
        enemy->hp -= MELEE_DAMAGE;
        enemy->hit_flash = 0.1f;
        audio_play_enemy_hit(&gs->audio);

        /* Knockback */
        Vector2 knockback_dir = Vector2Scale(to_enemy, MELEE_KNOCKBACK);
        enemy->velocity = knockback_dir;

        /* Damage number */
        damage_number_spawn(&gs->damage_numbers, enemy->position, (int)MELEE_DAMAGE,
                            (Color){200, 200, 255, 255});

        /* Hit sparks */
        particle_burst(&gs->particles, enemy->position, 8, 40.0f, 120.0f, 0.1f, 0.3f, 2.5f,
                       (Color){200, 200, 255, 255});

        if (enemy->hp <= 0.0f) {
            enemy->active = false;
            gs->stats.kills++;
            gs->combo.count++;
            gs->combo.timer = COMBO_TIMEOUT;
            gs->combo.display_timer = COMBO_DISPLAY_DURATION;
            if (gs->combo.count > gs->combo.best) {
                gs->combo.best = gs->combo.count;
            }
            screenshake_add_trauma(&gs->shake, 0.2f);

            /* Death explosion */
            particle_burst(&gs->particles, enemy->position, 15, 30.0f, 150.0f, 0.2f, 0.6f, 3.5f,
                           (Color){255, 60, 30, 255});
        }

        /* Melee hits all enemies in arc (no single-target limit) */
    }

    /* Melee only hits once per swing -- zero timer after resolving */
    p->melee_timer = 0.0f;
}

static void resolve_weapon_pickup_collisions(GameState *gs) {
    Player *p = &gs->player;
    WeaponPickupPool *wpp = &gs->weapon_pickups;

    for (int i = 0; i < MAX_WEAPON_PICKUPS; i++) {
        WeaponPickup *wp = &wpp->pickups[i];
        if (!wp->active) {
            continue;
        }

        if (check_circle_collision(p->position, PLAYER_RADIUS, wp->position,
                                   WEAPON_PICKUP_RADIUS)) {
            /* Swap weapon */
            p->current_weapon = wp->weapon;
            wp->active = false;

            /* Pickup effect */
            particle_burst(&gs->particles, wp->position, 8, 30.0f, 100.0f, 0.1f, 0.3f, 2.5f,
                           wp->weapon.bullet_color);
        }
    }
}

static void update_camera(GameState *gs) {
    gs->camera.target = gs->player.position;

    /* Clamp camera so it doesn't show outside the world.
     * offset is in screen pixels; convert to world units via zoom. */
    float zoom = (gs->camera.zoom != 0.0f) ? gs->camera.zoom : 1.0f;
    float half_w = gs->camera.offset.x / zoom;
    float half_h = gs->camera.offset.y / zoom;
    Rectangle wb = gs->arena;

    if (gs->camera.target.x - half_w < wb.x) {
        gs->camera.target.x = wb.x + half_w;
    }
    if (gs->camera.target.x + half_w > wb.x + wb.width) {
        gs->camera.target.x = wb.x + wb.width - half_w;
    }
    if (gs->camera.target.y - half_h < wb.y) {
        gs->camera.target.y = wb.y + half_h;
    }
    if (gs->camera.target.y + half_h > wb.y + wb.height) {
        gs->camera.target.y = wb.y + wb.height - half_h;
    }
}

static void draw_hud(const GameState *gs) {
    const Player *p = &gs->player;

    /* ── Health bar (bottom-left) ───────────────────────────────────────── */
    float bar_x = 10.0f;
    float bar_y = SCREEN_HEIGHT - 30.0f;
    float bar_w = 150.0f;
    float bar_h = 16.0f;
    float hp_ratio = p->hp / p->max_hp;
    if (hp_ratio < 0.0f) {
        hp_ratio = 0.0f;
    }

    DrawRectangle((int)bar_x, (int)bar_y, (int)bar_w, (int)bar_h, DARKGRAY);
    DrawRectangle((int)bar_x, (int)bar_y, (int)(bar_w * hp_ratio), (int)bar_h, RED);
    DrawRectangleLines((int)bar_x, (int)bar_y, (int)bar_w, (int)bar_h, RAYWHITE);
    DrawText("HP", (int)bar_x + 4, (int)bar_y + 2, 12, RAYWHITE);

    /* ── Dash cooldown indicator (next to health bar) ─────────────────── */
    float dash_x = bar_x + bar_w + 20.0f;
    float dash_w = 80.0f;
    float cd_ratio = 1.0f;
    if (p->dash_cooldown > 0.0f) {
        cd_ratio = 1.0f - (p->dash_cooldown / DASH_COOLDOWN);
    }

    DrawRectangle((int)dash_x, (int)bar_y, (int)dash_w, (int)bar_h, DARKGRAY);
    DrawRectangle((int)dash_x, (int)bar_y, (int)(dash_w * cd_ratio), (int)bar_h, SKYBLUE);
    DrawRectangleLines((int)dash_x, (int)bar_y, (int)dash_w, (int)bar_h, RAYWHITE);
    DrawText("DASH", (int)dash_x + 4, (int)bar_y + 2, 12, RAYWHITE);

    /* ── Melee cooldown indicator (next to dash) ─────────────────────── */
    float melee_x = dash_x + dash_w + 20.0f;
    float melee_w = 60.0f;
    float melee_ratio = 1.0f;
    if (p->melee_cooldown > 0.0f) {
        melee_ratio = 1.0f - (p->melee_cooldown / MELEE_COOLDOWN);
    }
    DrawRectangle((int)melee_x, (int)bar_y, (int)melee_w, (int)bar_h, DARKGRAY);
    DrawRectangle((int)melee_x, (int)bar_y, (int)(melee_w * melee_ratio), (int)bar_h,
                  (Color){200, 200, 255, 255});
    DrawRectangleLines((int)melee_x, (int)bar_y, (int)melee_w, (int)bar_h, RAYWHITE);
    DrawText("MELEE", (int)melee_x + 2, (int)bar_y + 2, 10, RAYWHITE);

    /* ── Weapon name (top-left) ────────────────────────────────────────── */
    const char *weapon_name = p->current_weapon.name ? p->current_weapon.name : "???";
    DrawText(weapon_name, 10, 10, 20, p->current_weapon.bullet_color);

    /* ── Combo counter (top-center) ────────────────────────────────────── */
    if (gs->combo.display_timer > 0.0f && gs->combo.count >= 2) {
        float fade = gs->combo.display_timer / COMBO_DISPLAY_DURATION;
        unsigned char alpha = (unsigned char)(255.0f * fade);

        /* Scale font size with combo */
        int font_size = 24 + gs->combo.count * 2;
        if (font_size > 48) {
            font_size = 48;
        }

        /* Color escalates from yellow to red */
        unsigned char r = 255;
        unsigned char g_val = (unsigned char)(255 - (gs->combo.count * 20));
        if (g_val < 50) {
            g_val = 50;
        }
        Color combo_color = {r, g_val, 50, alpha};

        const char *combo_text = TextFormat("x%d COMBO!", gs->combo.count);
        int w = MeasureText(combo_text, font_size);
        int x = (SCREEN_WIDTH - w) / 2;
        int y = 40;
        DrawText(combo_text, x, y, font_size, combo_color);
    }

    /* ── Kill counter + floor (top-right) ─────────────────────────────── */
    const char *floor_text = TextFormat("Floor %d", gs->floor);
    int floor_w = MeasureText(floor_text, 16);
    DrawText(floor_text, SCREEN_WIDTH - floor_w - 10, 10, 16, (Color){0, 220, 200, 255});

    const char *kills_text = TextFormat("Kills: %d", gs->stats.kills);
    int kills_w = MeasureText(kills_text, 16);
    DrawText(kills_text, SCREEN_WIDTH - kills_w - 10, 30, 16, RAYWHITE);

    /* Exit indicator */
    if (gs->exit_active) {
        DrawText("EXIT OPEN - Find the portal!",
                 (SCREEN_WIDTH - MeasureText("EXIT OPEN - Find the portal!", 16)) / 2,
                 SCREEN_HEIGHT - 55, 16, (Color){0, 255, 150, 255});
    }
}

static void draw_game_over(const GameState *gs) {
    /* Semi-transparent dark overlay */
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 180});

    /* "GAME OVER" title */
    const char *title = "GAME OVER";
    int title_size = 60;
    int title_w = MeasureText(title, title_size);
    int title_x = (SCREEN_WIDTH - title_w) / 2;
    int title_y = SCREEN_HEIGHT / 2 - 100;
    DrawText(title, title_x, title_y, title_size, RED);

    /* Stats */
    int stat_size = 20;
    int stat_x_center = SCREEN_WIDTH / 2;
    int stat_y = title_y + title_size + 30;
    int line_spacing = 28;

    /* Kills */
    const char *kills_text = TextFormat("Kills: %d", gs->stats.kills);
    int kills_w = MeasureText(kills_text, stat_size);
    DrawText(kills_text, stat_x_center - kills_w / 2, stat_y, stat_size, RAYWHITE);

    /* Survival time (MM:SS) */
    int total_seconds = (int)gs->stats.survival_time;
    int minutes = total_seconds / 60;
    int seconds = total_seconds % 60;
    const char *time_text = TextFormat("Time: %02d:%02d", minutes, seconds);
    int time_w = MeasureText(time_text, stat_size);
    DrawText(time_text, stat_x_center - time_w / 2, stat_y + line_spacing, stat_size, RAYWHITE);

    /* Waves spawned */
    const char *waves_text = TextFormat("Waves: %d", gs->stats.waves_spawned);
    int waves_w = MeasureText(waves_text, stat_size);
    DrawText(waves_text, stat_x_center - waves_w / 2, stat_y + 2 * line_spacing, stat_size,
             RAYWHITE);

    /* Floor reached */
    const char *floor_text = TextFormat("Floor: %d", gs->floor);
    int floor_w = MeasureText(floor_text, stat_size);
    DrawText(floor_text, stat_x_center - floor_w / 2, stat_y + 3 * line_spacing, stat_size,
             (Color){0, 220, 200, 255});

    /* Best combo */
    if (gs->combo.best >= 2) {
        const char *combo_text = TextFormat("Best Combo: x%d", gs->combo.best);
        int combo_w = MeasureText(combo_text, stat_size);
        DrawText(combo_text, stat_x_center - combo_w / 2, stat_y + 4 * line_spacing, stat_size,
                 (Color){255, 200, 50, 255});
    }

    /* Restart / menu prompts */
    const char *prompt = "Press R to restart  |  ESC for menu";
    int prompt_size = 16;
    int prompt_w = MeasureText(prompt, prompt_size);
    DrawText(prompt, (SCREEN_WIDTH - prompt_w) / 2, stat_y + 4 * line_spacing + 20, prompt_size,
             GRAY);
}

/* ── Menu helpers ──────────────────────────────────────────────────────────── */

/* Navigate a menu cursor with W/S/Up/Down. Returns new cursor value. */
static int menu_navigate(int cursor, int item_count) {
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
        cursor--;
        if (cursor < 0) {
            cursor = item_count - 1;
        }
    }
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
        cursor++;
        if (cursor >= item_count) {
            cursor = 0;
        }
    }
    return cursor;
}

/* Returns true if the user confirmed the current menu selection */
static bool menu_confirm(void) {
    return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
}

/* Draw a centered menu list. Selected item is highlighted. */
static void draw_menu_items(const char *items[], int count, int selected, int start_y,
                            int font_size, int spacing) {
    for (int i = 0; i < count; i++) {
        int w = MeasureText(items[i], font_size);
        int x = (SCREEN_WIDTH - w) / 2;
        int y = start_y + i * spacing;
        Color c = (i == selected) ? (Color){0, 220, 200, 255} : GRAY;
        DrawText(items[i], x, y, font_size, c);

        /* Draw selection arrow */
        if (i == selected) {
            DrawText(">", x - 20, y, font_size, c);
        }
    }
}

/* ── Phase-specific update helpers ─────────────────────────────────────────── */

static void update_first_run(GameState *gs) {
    enum { PICK_8DIR, PICK_TANK, PICK_COUNT };

    gs->menu_cursor = menu_navigate(gs->menu_cursor, PICK_COUNT);

    if (menu_confirm()) {
        switch (gs->menu_cursor) {
        case PICK_8DIR:
            gs->settings.movement_layout = MOVEMENT_8DIR;
            break;
        case PICK_TANK:
            gs->settings.movement_layout = MOVEMENT_TANK;
            break;
        }
        settings_save(&gs->settings);
        gs->phase = PHASE_MAIN_MENU;
        gs->menu_cursor = 0;
    }
}

static void update_main_menu(GameState *gs) {
    enum { MENU_PLAY, MENU_SETTINGS, MENU_QUIT, MENU_COUNT };

    gs->menu_cursor = menu_navigate(gs->menu_cursor, MENU_COUNT);

    if (menu_confirm()) {
        switch (gs->menu_cursor) {
        case MENU_PLAY:
            game_init(gs);
            gs->phase = PHASE_PLAYING;
            gs->menu_cursor = 0;
            break;
        case MENU_SETTINGS:
            gs->settings_return_phase = PHASE_MAIN_MENU;
            gs->phase = PHASE_SETTINGS;
            gs->menu_cursor = 0;
            break;
        case MENU_QUIT:
            gs->should_quit = true;
            break;
        }
    }
}

static void update_paused(GameState *gs) {
    bool dead = gs->player.hp <= 0.0f;

    if (dead) {
        /* Dead: menu is Settings / Main Menu / Quit (no Resume) */
        enum { DEAD_SETTINGS, DEAD_MAIN_MENU, DEAD_QUIT, DEAD_COUNT };

        /* ESC returns to game-over screen */
        if (IsKeyPressed(KEY_ESCAPE)) {
            gs->phase = PHASE_GAME_OVER;
            gs->menu_cursor = 0;
            return;
        }

        gs->menu_cursor = menu_navigate(gs->menu_cursor, DEAD_COUNT);

        if (menu_confirm()) {
            switch (gs->menu_cursor) {
            case DEAD_SETTINGS:
                gs->settings_return_phase = PHASE_PAUSED;
                gs->phase = PHASE_SETTINGS;
                gs->menu_cursor = 0;
                break;
            case DEAD_MAIN_MENU:
                audio_stop_death_music(&gs->audio);
                gs->phase = PHASE_MAIN_MENU;
                gs->menu_cursor = 0;
                break;
            case DEAD_QUIT:
                gs->should_quit = true;
                break;
            }
        }
    } else {
        /* Alive: full menu with Resume */
        enum { PAUSE_RESUME, PAUSE_SETTINGS, PAUSE_MAIN_MENU, PAUSE_QUIT, PAUSE_COUNT };

        /* ESC to resume */
        if (IsKeyPressed(KEY_ESCAPE)) {
            gs->phase = PHASE_PLAYING;
            gs->menu_cursor = 0;
            return;
        }

        gs->menu_cursor = menu_navigate(gs->menu_cursor, PAUSE_COUNT);

        if (menu_confirm()) {
            switch (gs->menu_cursor) {
            case PAUSE_RESUME:
                gs->phase = PHASE_PLAYING;
                gs->menu_cursor = 0;
                break;
            case PAUSE_SETTINGS:
                gs->settings_return_phase = PHASE_PAUSED;
                gs->phase = PHASE_SETTINGS;
                gs->menu_cursor = 0;
                break;
            case PAUSE_MAIN_MENU:
                audio_stop_death_music(&gs->audio);
                gs->phase = PHASE_MAIN_MENU;
                gs->menu_cursor = 0;
                break;
            case PAUSE_QUIT:
                gs->should_quit = true;
                break;
            }
        }
    }
}

static void update_settings(GameState *gs) {
    /* ESC to go back */
    if (IsKeyPressed(KEY_ESCAPE)) {
        gs->phase = gs->settings_return_phase;
        gs->menu_cursor = 0;
        return;
    }

    /* Left/Right to toggle movement layout */
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
        if (gs->settings.movement_layout == MOVEMENT_TANK) {
            gs->settings.movement_layout = MOVEMENT_8DIR;
        } else {
            gs->settings.movement_layout = MOVEMENT_TANK;
        }
        settings_save(&gs->settings);
    }
}

static void advance_floor(GameState *gs) {
    gs->floor++;
    gs->floor_waves = 0;
    gs->exit_active = false;

    /* Regenerate world */
    int spawn_tx = WORLD_COLS / 2;
    int spawn_ty = WORLD_ROWS / 2;
    tilemap_generate(&gs->tilemap, spawn_tx, spawn_ty, 0);
    gs->arena = tilemap_get_world_bounds(&gs->tilemap);

    /* Place player at center */
    Vector2 center = tilemap_tile_to_world(&gs->tilemap, spawn_tx, spawn_ty);
    center.x += TILE_SIZE / 2.0f;
    center.y += TILE_SIZE / 2.0f;
    gs->player.position = center;

    /* Reset pools (keep player stats, weapon, HP) */
    bullet_pool_init(&gs->bullets);
    enemy_pool_init(&gs->enemies);
    enemy_bullet_pool_init(&gs->enemy_bullets);
    particle_pool_init(&gs->particles);
    damage_number_pool_init(&gs->damage_numbers);
    weapon_pickup_pool_init(&gs->weapon_pickups);
    gs->spawn_timer = SPAWN_INTERVAL;

    /* Camera */
    gs->camera.target = center;
}

static void update_playing(GameState *gs) {
    /* ESC to pause */
    if (IsKeyPressed(KEY_ESCAPE)) {
        gs->phase = PHASE_PAUSED;
        gs->menu_cursor = 0;
        return;
    }

    float dt = GetFrameTime();
    gs->stats.survival_time += dt;

    player_update(&gs->player, dt, gs->arena, &gs->tilemap, gs->camera,
                  gs->settings.movement_layout);

    /* ── Shooting (weapon-driven) ────────────────────────────────────── */
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 muzzle = Vector2Add(gs->player.position,
                                    Vector2Scale(gs->player.aim_direction, PLAYER_RADIUS + 2.0f));
        Weapon *w = &gs->player.current_weapon;
        int fired =
            bullet_pool_fire_weapon(&gs->bullets, muzzle, gs->player.aim_direction, w->fire_rate,
                                    w->damage, w->bullet_speed, w->spread_angle,
                                    w->projectile_count, w->bullet_lifetime, w->bullet_color);
        if (fired > 0) {
            audio_play_shoot(&gs->audio);

            /* Muzzle flash particles */
            particle_burst(&gs->particles, muzzle, 4, 60.0f, 150.0f, 0.05f, 0.15f, 2.0f,
                           (Color){255, 220, 100, 255});
        }
    }

    /* ── Melee attack (right-click) ──────────────────────────────────── */
    if (gs->player.melee_cooldown > 0.0f) {
        gs->player.melee_cooldown -= dt;
    }
    if (gs->player.melee_timer > 0.0f) {
        gs->player.melee_timer -= dt;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && gs->player.melee_cooldown <= 0.0f) {
        gs->player.melee_timer = MELEE_DURATION;
        gs->player.melee_cooldown = MELEE_COOLDOWN;
        gs->player.melee_direction = gs->player.aim_direction;

        /* Swing particles */
        particle_burst(&gs->particles, gs->player.position, 6, 40.0f, 100.0f, 0.05f, 0.2f, 3.0f,
                       (Color){200, 200, 255, 200});
    }

    /* ── Dash trail ───────────────────────────────────────────────────── */
    if (gs->player.is_dashing) {
        particle_emit(&gs->particles, gs->player.position,
                      (Vector2){(float)GetRandomValue(-30, 30), (float)GetRandomValue(-30, 30)},
                      0.3f, 5.0f, (Color){80, 160, 255, 150});
    }

    bullet_pool_update(&gs->bullets, dt, gs->arena, &gs->tilemap);
    particle_pool_update(&gs->particles, dt);
    damage_number_pool_update(&gs->damage_numbers, dt);

    /* ── Enemy spawning ───────────────────────────────────────────────── */
    gs->spawn_timer -= dt;
    if (gs->spawn_timer <= 0.0f) {
        spawn_wave(gs);
        gs->spawn_timer = SPAWN_INTERVAL;
        gs->stats.waves_spawned++;
        gs->floor_waves++;

        /* Spawn exit after enough waves on this floor */
        if (gs->floor_waves >= WAVES_PER_FLOOR && !gs->exit_active) {
            /* Place exit at a random walkable tile far from player */
            for (int attempt = 0; attempt < 20; attempt++) {
                int tx = GetRandomValue(2, WORLD_COLS - 3);
                int ty = GetRandomValue(2, WORLD_ROWS - 3);
                if (gs->tilemap.tiles[ty][tx] == TILE_EMPTY) {
                    Vector2 epos = tilemap_tile_to_world(&gs->tilemap, tx, ty);
                    epos.x += TILE_SIZE / 2.0f;
                    epos.y += TILE_SIZE / 2.0f;
                    float dx = epos.x - gs->player.position.x;
                    float dy = epos.y - gs->player.position.y;
                    if (dx * dx + dy * dy > 200.0f * 200.0f) {
                        gs->exit_position = epos;
                        gs->exit_active = true;
                        break;
                    }
                }
            }
        }
    }

    /* ── Floor exit check ─────────────────────────────────────────────── */
    if (gs->exit_active) {
        if (check_circle_collision(gs->player.position, PLAYER_RADIUS, gs->exit_position,
                                   EXIT_RADIUS)) {
            advance_floor(gs);
            return; /* skip rest of frame after floor transition */
        }
    }

    /* ── Enemy update ─────────────────────────────────────────────────── */
    tilemap_compute_flow_field(&gs->tilemap, gs->player.position.x, gs->player.position.y);
    enemy_pool_update(&gs->enemies, dt, gs->player.position, gs->arena, &gs->tilemap,
                      &gs->enemy_bullets);
    enemy_bullet_pool_update(&gs->enemy_bullets, dt, gs->arena);

    /* ── Weapon pickups ────────────────────────────────────────────────── */
    weapon_pickup_pool_update(&gs->weapon_pickups, dt);

    /* ── Combo timer ──────────────────────────────────────────────────── */
    if (gs->combo.timer > 0.0f) {
        gs->combo.timer -= dt;
        if (gs->combo.timer <= 0.0f) {
            gs->combo.count = 0;
            gs->combo.timer = 0.0f;
        }
    }
    if (gs->combo.display_timer > 0.0f) {
        gs->combo.display_timer -= dt;
    }

    /* ── Collisions ───────────────────────────────────────────────────── */
    resolve_bullet_enemy_collisions(gs);
    resolve_melee_enemy_collisions(gs);
    resolve_enemy_player_collisions(gs);
    resolve_enemy_bullet_player_collisions(gs);
    resolve_weapon_pickup_collisions(gs);

    /* ── Death check ──────────────────────────────────────────────────── */
    game_check_death(gs);

    /* ── Camera ───────────────────────────────────────────────────────── */
    update_camera(gs);
    screenshake_update(&gs->shake, dt);
}

static void update_game_over(GameState *gs) {
    /* ESC opens pause menu (with main menu / quit options) */
    if (IsKeyPressed(KEY_ESCAPE)) {
        gs->phase = PHASE_PAUSED;
        gs->menu_cursor = 0;
        return;
    }

    /* R to restart */
    if (IsKeyPressed(KEY_R)) {
        audio_stop_death_music(&gs->audio);
        game_init(gs);
        gs->phase = PHASE_PLAYING;
    }
}

/* ── Phase-specific draw helpers ───────────────────────────────────────────── */

static void draw_first_run(const GameState *gs) {
    /* Title */
    const char *title = "ASTRO BLITZ";
    int title_size = 50;
    int title_w = MeasureText(title, title_size);
    DrawText(title, (SCREEN_WIDTH - title_w) / 2, 80, title_size, (Color){0, 220, 200, 255});

    /* Prompt */
    const char *prompt = "Choose your movement style:";
    int prompt_size = 20;
    int prompt_w = MeasureText(prompt, prompt_size);
    DrawText(prompt, (SCREEN_WIDTH - prompt_w) / 2, 170, prompt_size, RAYWHITE);

    /* Options */
    const char *items[] = {"8-Directional", "Tank Controls"};
    draw_menu_items(items, 2, gs->menu_cursor, 240, 24, 50);

    /* Descriptions for the currently highlighted option */
    const char *desc = NULL;
    if (gs->menu_cursor == 0) {
        desc = "W = up, S = down, A = left, D = right";
    } else {
        desc = "W/S = forward/back toward aim, A/D = strafe";
    }
    int desc_size = 16;
    int desc_w = MeasureText(desc, desc_size);
    DrawText(desc, (SCREEN_WIDTH - desc_w) / 2, 360, desc_size, GRAY);

    const char *note = "(You can change this later in Settings)";
    int note_size = 14;
    int note_w = MeasureText(note, note_size);
    DrawText(note, (SCREEN_WIDTH - note_w) / 2, 390, note_size, DARKGRAY);

    /* Hint */
    const char *hint = "W/S to select  |  Enter to confirm";
    int hint_size = 14;
    int hint_w = MeasureText(hint, hint_size);
    DrawText(hint, (SCREEN_WIDTH - hint_w) / 2, SCREEN_HEIGHT - 50, hint_size, DARKGRAY);
}

static void draw_main_menu(const GameState *gs) {
    /* Title */
    const char *title = "ASTRO BLITZ";
    int title_size = 50;
    int title_w = MeasureText(title, title_size);
    DrawText(title, (SCREEN_WIDTH - title_w) / 2, 120, title_size, (Color){0, 220, 200, 255});

    /* Subtitle */
    const char *sub = "Top-down sci-fi roguelike shooter";
    int sub_size = 16;
    int sub_w = MeasureText(sub, sub_size);
    DrawText(sub, (SCREEN_WIDTH - sub_w) / 2, 180, sub_size, GRAY);

    /* Menu items */
    const char *items[] = {"Play", "Settings", "Quit"};
    draw_menu_items(items, 3, gs->menu_cursor, 280, 24, 40);
}

static void draw_paused(const GameState *gs) {
    /* Dark overlay */
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 180});

    bool dead = gs->player.hp <= 0.0f;
    const char *title = dead ? "GAME OVER" : "PAUSED";
    int title_size = 40;
    int title_w = MeasureText(title, title_size);
    DrawText(title, (SCREEN_WIDTH - title_w) / 2, 150, title_size, dead ? RED : RAYWHITE);

    if (dead) {
        /* No Resume when player is dead */
        const char *items[] = {"Settings", "Main Menu", "Quit"};
        draw_menu_items(items, 3, gs->menu_cursor, 240, 22, 36);
    } else {
        const char *items[] = {"Resume", "Settings", "Main Menu", "Quit"};
        draw_menu_items(items, 4, gs->menu_cursor, 240, 22, 36);
    }
}

static void draw_settings(const GameState *gs) {
    /* Dark overlay if coming from pause (game is behind) */
    if (gs->settings_return_phase != PHASE_MAIN_MENU) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 200});
    }

    const char *title = "SETTINGS";
    int title_size = 40;
    int title_w = MeasureText(title, title_size);
    DrawText(title, (SCREEN_WIDTH - title_w) / 2, 120, title_size, (Color){0, 220, 200, 255});

    /* Movement layout option */
    const char *label = "Movement:";
    int label_size = 22;
    int label_w = MeasureText(label, label_size);
    int label_x = SCREEN_WIDTH / 2 - label_w - 10;
    int y = 240;
    DrawText(label, label_x, y, label_size, RAYWHITE);

    const char *value =
        (gs->settings.movement_layout == MOVEMENT_TANK) ? "< Tank Controls >" : "< 8-Directional >";
    int value_size = 22;
    DrawText(value, SCREEN_WIDTH / 2 + 10, y, value_size, (Color){0, 220, 200, 255});

    /* Description */
    const char *desc = NULL;
    if (gs->settings.movement_layout == MOVEMENT_TANK) {
        desc = "W/S = forward/back relative to aim, A/D = strafe";
    } else {
        desc = "W = up, S = down, A = left, D = right (screen-relative)";
    }
    int desc_size = 14;
    int desc_w = MeasureText(desc, desc_size);
    DrawText(desc, (SCREEN_WIDTH - desc_w) / 2, y + 40, desc_size, GRAY);

    /* Instructions */
    const char *hint = "Left/Right to change  |  ESC to go back";
    int hint_size = 14;
    int hint_w = MeasureText(hint, hint_size);
    DrawText(hint, (SCREEN_WIDTH - hint_w) / 2, SCREEN_HEIGHT - 60, hint_size, DARKGRAY);
}

/* ── Public ────────────────────────────────────────────────────────────────── */

void game_init(GameState *gs) {
    /* Preserve settings and audio across restarts */
    Settings saved_settings = gs->settings;
    GameAudio saved_audio = gs->audio;

    /* Generate world */
    int spawn_tx = WORLD_COLS / 2;
    int spawn_ty = WORLD_ROWS / 2;
    tilemap_generate(&gs->tilemap, spawn_tx, spawn_ty, 0);
    gs->arena = tilemap_get_world_bounds(&gs->tilemap);

    /* Place player at center of spawn tile */
    Vector2 center = tilemap_tile_to_world(&gs->tilemap, spawn_tx, spawn_ty);
    center.x += TILE_SIZE / 2.0f;
    center.y += TILE_SIZE / 2.0f;

    player_init(&gs->player, center);
    bullet_pool_init(&gs->bullets);
    enemy_pool_init(&gs->enemies);
    enemy_bullet_pool_init(&gs->enemy_bullets);
    particle_pool_init(&gs->particles);
    damage_number_pool_init(&gs->damage_numbers);
    weapon_pickup_pool_init(&gs->weapon_pickups);
    screenshake_init(&gs->shake);
    gs->spawn_timer = SPAWN_INTERVAL;
    gs->phase = PHASE_PLAYING;
    gs->settings_return_phase = PHASE_MAIN_MENU;
    gs->menu_cursor = 0;
    gs->should_quit = false;
    gs->stats = (GameStats){.kills = 0, .survival_time = 0.0f, .waves_spawned = 0};
    gs->combo = (ComboState){.count = 0, .timer = 0.0f, .display_timer = 0.0f, .best = 0};
    gs->floor = 1;
    gs->floor_waves = 0;
    gs->exit_active = false;
    gs->exit_position = (Vector2){0};

    /* Camera centered on player */
    gs->camera.target = center;
    gs->camera.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
    gs->camera.rotation = 0.0f;
    gs->camera.zoom = 1.0f;

    /* Restore preserved state */
    gs->settings = saved_settings;
    gs->audio = saved_audio;
}

void game_update(GameState *gs) {
    switch (gs->phase) {
    case PHASE_FIRST_RUN:
        update_first_run(gs);
        break;
    case PHASE_MAIN_MENU:
        update_main_menu(gs);
        break;
    case PHASE_PLAYING:
        update_playing(gs);
        break;
    case PHASE_PAUSED:
        update_paused(gs);
        break;
    case PHASE_SETTINGS:
        update_settings(gs);
        break;
    case PHASE_GAME_OVER:
        update_game_over(gs);
        break;
    }
}

void game_check_death(GameState *gs) {
    if (gs->phase == PHASE_PLAYING && gs->player.hp <= 0.0f) {
        gs->phase = PHASE_GAME_OVER;
        audio_play_death_music(&gs->audio);
    }
}

bool game_should_quit(const GameState *gs) {
    return gs->should_quit;
}

void game_draw(const GameState *gs) {
    BeginDrawing();
    ClearBackground(BLACK);

    switch (gs->phase) {
    case PHASE_FIRST_RUN:
        draw_first_run(gs);
        break;

    case PHASE_MAIN_MENU:
        draw_main_menu(gs);
        break;

    case PHASE_PLAYING:
    case PHASE_PAUSED:
    case PHASE_GAME_OVER: {
        /* Apply screen shake to camera for world rendering */
        Camera2D draw_cam = screenshake_apply(&gs->shake, gs->camera);

        /* Draw the world behind any overlay */
        BeginMode2D(draw_cam);
        tilemap_draw(&gs->tilemap, gs->camera);
        /* Exit portal */
        if (gs->exit_active) {
            float pulse = 0.5f + 0.5f * sinf((float)gs->stats.survival_time * 3.0f);
            unsigned char ga = (unsigned char)(60.0f + 60.0f * pulse);
            DrawCircleV(gs->exit_position, EXIT_RADIUS + 8.0f, (Color){0, 255, 150, ga});
            DrawCircleV(gs->exit_position, EXIT_RADIUS, (Color){0, 40, 30, 255});
            DrawCircleLinesV(gs->exit_position, EXIT_RADIUS, (Color){0, 255, 150, 200});
            DrawCircleLinesV(gs->exit_position, EXIT_RADIUS + 2.0f, (Color){0, 200, 120, 100});
        }

        weapon_pickup_pool_draw(&gs->weapon_pickups);
        bullet_pool_draw(&gs->bullets);
        enemy_bullet_pool_draw(&gs->enemy_bullets);
        enemy_pool_draw(&gs->enemies);
        particle_pool_draw(&gs->particles);
        damage_number_pool_draw(&gs->damage_numbers);
        player_draw(&gs->player);

        /* Melee swing arc visual */
        if (gs->player.melee_cooldown > MELEE_COOLDOWN - MELEE_DURATION * 2.0f) {
            float fade = gs->player.melee_cooldown / MELEE_COOLDOWN;
            unsigned char alpha = (unsigned char)(200.0f * fade);
            Color arc_color = {200, 200, 255, alpha};
            float half_arc = MELEE_ARC_DEGREES * 0.5f * DEG2RAD;
            float range = MELEE_RANGE + PLAYER_RADIUS;
            Vector2 dir = gs->player.melee_direction;
            int segments = 8;
            for (int s = 0; s < segments; s++) {
                float t0 = -half_arc + (float)s / (float)segments * 2.0f * half_arc;
                float t1 = -half_arc + (float)(s + 1) / (float)segments * 2.0f * half_arc;
                float cos0 = cosf(t0);
                float sin0 = sinf(t0);
                float cos1 = cosf(t1);
                float sin1 = sinf(t1);
                Vector2 d0 = {dir.x * cos0 - dir.y * sin0, dir.x * sin0 + dir.y * cos0};
                Vector2 d1 = {dir.x * cos1 - dir.y * sin1, dir.x * sin1 + dir.y * cos1};
                Vector2 p0 = Vector2Add(gs->player.position, Vector2Scale(d0, range));
                Vector2 p1 = Vector2Add(gs->player.position, Vector2Scale(d1, range));
                DrawLineEx(p0, p1, 2.0f, arc_color);
            }
        }

        EndMode2D();

        draw_hud(gs);

        if (gs->phase == PHASE_GAME_OVER) {
            draw_game_over(gs);
        } else if (gs->phase == PHASE_PAUSED) {
            draw_paused(gs);
        }
        break;
    }

    case PHASE_SETTINGS:
        /* Draw world behind if returning to pause, black bg if from main menu */
        if (gs->settings_return_phase != PHASE_MAIN_MENU) {
            BeginMode2D(gs->camera);
            tilemap_draw(&gs->tilemap, gs->camera);
            bullet_pool_draw(&gs->bullets);
            enemy_pool_draw(&gs->enemies);
            particle_pool_draw(&gs->particles);
            player_draw(&gs->player);
            EndMode2D();
            draw_hud(gs);
        }
        draw_settings(gs);
        break;
    }

    EndDrawing();
}
