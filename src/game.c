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

/* ── Juice helpers (scaled by settings) ────────────────────────────────────── */

/* Apply hitstop, scaled by the hitstop setting. */
static void apply_hitstop(GameState *gs, float duration) {
    float scaled = duration * gs->settings.hitstop;
    if (scaled > gs->hitstop_timer) {
        gs->hitstop_timer = scaled;
    }
}

/* Apply screen shake, scaled by the screen_shake setting. */
static void apply_shake(GameState *gs, float trauma) {
    screenshake_add_trauma(&gs->shake, trauma * gs->settings.screen_shake);
}

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

/* ── Corpse helpers ─────────────────────────────────────────────────────────── */

static void corpse_pool_init(CorpsePool *pool) {
    for (int i = 0; i < MAX_CORPSES; i++) {
        pool->corpses[i].active = false;
    }
}

static void corpse_spawn(CorpsePool *pool, Vector2 position, float radius, Color color) {
    /* Find an inactive slot (overwrite oldest if full) */
    int slot = -1;
    float oldest = CORPSE_LIFETIME + 1.0f;
    for (int i = 0; i < MAX_CORPSES; i++) {
        if (!pool->corpses[i].active) {
            slot = i;
            break;
        }
        if (pool->corpses[i].lifetime < oldest) {
            oldest = pool->corpses[i].lifetime;
            slot = i;
        }
    }
    if (slot < 0) {
        slot = 0;
    }
    Corpse *c = &pool->corpses[slot];
    c->position = position;
    c->radius = radius;
    c->lifetime = CORPSE_LIFETIME;
    c->color = color;
    c->active = true;
}

static void corpse_pool_update(CorpsePool *pool, float dt) {
    for (int i = 0; i < MAX_CORPSES; i++) {
        Corpse *c = &pool->corpses[i];
        if (!c->active) {
            continue;
        }
        c->lifetime -= dt;
        if (c->lifetime <= 0.0f) {
            c->active = false;
        }
    }
}

static void corpse_pool_draw(const CorpsePool *pool) {
    for (int i = 0; i < MAX_CORPSES; i++) {
        const Corpse *c = &pool->corpses[i];
        if (!c->active) {
            continue;
        }
        float ratio = c->lifetime / CORPSE_LIFETIME;
        unsigned char alpha = (unsigned char)(ratio * 120.0f); /* max 120 alpha, faded */
        Color col = c->color;
        col.a = alpha;
        DrawCircleV(c->position, c->radius, col);
        /* Faint outline */
        col.a = (unsigned char)(alpha * 0.5f);
        DrawCircleLinesV(c->position, c->radius, col);
    }
}

static Color enemy_type_color(EnemyType type) {
    switch (type) {
    case ENEMY_SWARMER:
        return (Color){255, 60, 30, 255};
    case ENEMY_GRUNT:
        return (Color){100, 100, 255, 255};
    case ENEMY_STALKER:
        return (Color){50, 255, 50, 255};
    case ENEMY_BOMBER:
        return (Color){255, 150, 30, 255};
    default:
        return (Color){200, 200, 200, 255};
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

/* ── Upgrade pickup helpers ─────────────────────────────────────────────────── */

static void upgrade_pickup_pool_init(UpgradePickupPool *pool) {
    memset(pool->pickups, 0, sizeof(pool->pickups));
}

static void upgrade_pickup_spawn(UpgradePickupPool *pool, Vector2 pos, UpgradeType type) {
    for (int i = 0; i < MAX_UPGRADE_PICKUPS; i++) {
        UpgradePickup *up = &pool->pickups[i];
        if (!up->active) {
            up->position = pos;
            up->type = type;
            up->lifetime = UPGRADE_PICKUP_LIFETIME;
            up->active = true;
            return;
        }
    }
}

static void upgrade_pickup_pool_update(UpgradePickupPool *pool, float dt) {
    for (int i = 0; i < MAX_UPGRADE_PICKUPS; i++) {
        UpgradePickup *up = &pool->pickups[i];
        if (!up->active) {
            continue;
        }
        up->lifetime -= dt;
        if (up->lifetime <= 0.0f) {
            up->active = false;
        }
    }
}

static void upgrade_pickup_pool_draw(const UpgradePickupPool *pool) {
    for (int i = 0; i < MAX_UPGRADE_PICKUPS; i++) {
        const UpgradePickup *up = &pool->pickups[i];
        if (!up->active) {
            continue;
        }

        Color c = upgrade_get_color(up->type);
        float pulse = 0.5f + 0.5f * sinf(up->lifetime * 5.0f);
        unsigned char ga = (unsigned char)(40.0f + 40.0f * pulse);
        Color glow = c;
        glow.a = ga;

        /* Fade in last 3 seconds */
        float alpha_mult = 1.0f;
        if (up->lifetime < 3.0f) {
            alpha_mult = up->lifetime / 3.0f;
        }
        c.a = (unsigned char)(255.0f * alpha_mult);

        DrawCircleV(up->position, UPGRADE_PICKUP_RADIUS + 3.0f, glow);
        DrawCircleV(up->position, UPGRADE_PICKUP_RADIUS, (Color){c.r / 4, c.g / 4, c.b / 4, c.a});
        DrawCircleLinesV(up->position, UPGRADE_PICKUP_RADIUS, c);

        /* Label */
        const char *name = upgrade_get_name(up->type);
        int fs = 8;
        int w = MeasureText(name, fs);
        DrawText(name, (int)(up->position.x - w / 2.0f), (int)(up->position.y - fs / 2.0f), fs, c);
    }
}

static void resolve_upgrade_pickup_collisions(GameState *gs) {
    Player *p = &gs->player;
    UpgradePickupPool *pool = &gs->upgrade_pickups;

    for (int i = 0; i < MAX_UPGRADE_PICKUPS; i++) {
        UpgradePickup *up = &pool->pickups[i];
        if (!up->active) {
            continue;
        }

        if (check_circle_collision(p->position, PLAYER_RADIUS, up->position,
                                   UPGRADE_PICKUP_RADIUS)) {
            /* Only show menu if we can actually add the upgrade */
            if (gs->upgrades.stacks[up->type] < MAX_UPGRADE_STACKS) {
                gs->pending_upgrade = up->type;
                gs->pending_upgrade_index = i;
                gs->menu_cursor = 0; /* default to "Keep" */
                gs->phase = PHASE_PICKUP_UPGRADE;
                return; /* pause -- handle one at a time */
            }
        }
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

    /* Calculate viewport edges in world space from the active camera.
     * Use camera.offset * 2 as the effective viewport size. */
    float screen_w = gs->camera.offset.x * 2.0f;
    float screen_h = gs->camera.offset.y * 2.0f;
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

                int prev = enemy_pool_active_count(&gs->enemies);
                if (elite != ELITE_NONE) {
                    enemy_pool_spawn_elite(&gs->enemies, type, pos, elite);
                } else {
                    enemy_pool_spawn(&gs->enemies, type, pos);
                }

                /* Scale HP by floor level */
                if (gs->floor > 0 && enemy_pool_active_count(&gs->enemies) > prev) {
                    float scale = 1.0f + FLOOR_ENEMY_HP_SCALE * (float)gs->floor;
                    for (int fi = 0; fi < MAX_ENEMIES; fi++) {
                        Enemy *fe = &gs->enemies.enemies[fi];
                        if (fe->active && fe->position.x == pos.x && fe->position.y == pos.y) {
                            fe->hp *= scale;
                            fe->max_hp = fe->hp;
                            break;
                        }
                    }
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
                /* Knockback: push enemy in bullet direction */
                float bspeed = Vector2Length(bullet->velocity);
                if (bspeed > 0.01f) {
                    Vector2 kb_dir = Vector2Scale(bullet->velocity, 1.0f / bspeed);
                    enemy->velocity =
                        Vector2Add(enemy->velocity, Vector2Scale(kb_dir, KNOCKBACK_BULLET));
                }

                bullet->active = false;
                enemy->hp -= bullet->damage;
                enemy->hit_flash = HIT_FLASH_DURATION;
                audio_play_enemy_hit(&gs->audio);

                /* Hitstop */
                apply_hitstop(gs, HITSTOP_HIT);

                /* Hit sparks */
                particle_burst(&gs->particles, bullet->position, 6, 40.0f, 120.0f, 0.1f, 0.3f, 2.5f,
                               (Color){255, 200, 50, 255});

                /* Damage number */
                damage_number_spawn(&gs->damage_numbers, enemy->position, dmg,
                                    (Color){255, 255, 100, 255});

                if (enemy->hp <= 0.0f) {
                    /* Spawn corpse before deactivating */
                    corpse_spawn(&gs->corpses, enemy->position, enemy->radius,
                                 enemy_type_color(enemy->type));
                    enemy->active = false;
                    gs->stats.kills++;

                    /* Kill juice: longer hitstop + slowmo */
                    apply_hitstop(gs, HITSTOP_KILL);
                    gs->slowmo_timer = SLOWMO_DURATION;
                    gs->slowmo_scale = SLOWMO_SCALE;

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
                    apply_shake(gs, shake_amount);

                    /* Death explosion particles */
                    particle_burst(&gs->particles, enemy->position, 15, 30.0f, 150.0f, 0.2f, 0.6f,
                                   3.5f, (Color){255, 60, 30, 255});
                    particle_burst(&gs->particles, enemy->position, 8, 20.0f, 80.0f, 0.3f, 0.8f,
                                   2.0f, (Color){255, 160, 40, 200});

                    /* Bomber AoE explosion on death */
                    if (enemy->type == ENEMY_BOMBER) {
                        apply_shake(gs, 0.4f);
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

                    /* Upgrade drop (elites have a chance) */
                    if (enemy->elite != ELITE_NONE &&
                        GetRandomValue(1, 100) <= ELITE_UPGRADE_DROP_CHANCE) {
                        UpgradeType utype = (UpgradeType)GetRandomValue(0, UPGRADE_COUNT - 1);
                        upgrade_pickup_spawn(&gs->upgrade_pickups, enemy->position, utype);
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
            corpse_spawn(&gs->corpses, enemy->position, enemy->radius,
                         enemy_type_color(enemy->type));
            enemy->active = false; /* swarmer dies on contact */
            audio_play_hit(&gs->audio);
            apply_shake(gs, 0.35f);
            apply_hitstop(gs, HITSTOP_PLAYER);

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
            apply_shake(gs, 0.2f);
            apply_hitstop(gs, HITSTOP_PLAYER);

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
        apply_hitstop(gs, HITSTOP_MELEE);

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
            corpse_spawn(&gs->corpses, enemy->position, enemy->radius,
                         enemy_type_color(enemy->type));
            enemy->active = false;
            gs->stats.kills++;

            /* Kill juice */
            apply_hitstop(gs, HITSTOP_KILL + (2.0f / 60.0f)); /* melee kills: extra hitstop */
            gs->slowmo_timer = SLOWMO_DURATION;
            gs->slowmo_scale = SLOWMO_SCALE;

            gs->combo.count++;
            gs->combo.timer = COMBO_TIMEOUT;
            gs->combo.display_timer = COMBO_DISPLAY_DURATION;
            if (gs->combo.count > gs->combo.best) {
                gs->combo.best = gs->combo.count;
            }
            apply_shake(gs, 0.25f);

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
            gs->pending_weapon = wp->weapon;
            gs->pending_weapon_index = i;
            gs->menu_cursor = 0; /* default to "Keep Current" */
            gs->phase = PHASE_PICKUP_WEAPON;
            return; /* pause -- handle one at a time */
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
    float cd_mult = (p->dash_cd_mult > 0.0f) ? p->dash_cd_mult : 1.0f;
    float eff_dash_cd = DASH_COOLDOWN * cd_mult;
    float cd_ratio = 1.0f;
    if (p->dash_cooldown > 0.0f) {
        cd_ratio = 1.0f - (p->dash_cooldown / eff_dash_cd);
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

    /* ── Upgrade icons (below weapon name) ─────────────────────────────── */
    int ux = 10;
    int uy = 34;
    for (int i = 0; i < UPGRADE_COUNT; i++) {
        int stacks = gs->upgrades.stacks[i];
        if (stacks > 0) {
            Color c = upgrade_get_color((UpgradeType)i);
            const char *label = TextFormat("%s x%d", upgrade_get_name((UpgradeType)i), stacks);
            DrawText(label, ux, uy, 12, c);
            uy += 14;
        }
    }

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
    /* Left/Right or A/D to toggle between the two cards */
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        gs->menu_cursor = 0;
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        gs->menu_cursor = 1;
    }

    if (menu_confirm()) {
        gs->settings.movement_layout = (gs->menu_cursor == 0) ? MOVEMENT_8DIR : MOVEMENT_TANK;
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

/* Number of items in the settings menu */
#define SETTINGS_ITEM_COUNT 8

/* Clamp float to [0,1] for settings sliders */
static float settings_clamp01(float v) {
    if (v < 0.0f) {
        return 0.0f;
    }
    if (v > 1.0f) {
        return 1.0f;
    }
    return v;
}

/* Get pointer to the float setting for a given settings menu row (1-7).
 * Row 0 is movement layout (not a float). Returns NULL for invalid. */
static float *settings_float_ptr(Settings *s, int row) {
    switch (row) {
    case 1:
        return &s->screen_shake;
    case 2:
        return &s->hitstop;
    case 3:
        return &s->bloom;
    case 4:
        return &s->scanlines;
    case 5:
        return &s->chromatic_aberration;
    case 6:
        return &s->vignette;
    case 7:
        return &s->lighting;
    default:
        return NULL;
    }
}

static void update_settings(GameState *gs) {
    /* ESC to go back */
    if (IsKeyPressed(KEY_ESCAPE)) {
        gs->phase = gs->settings_return_phase;
        gs->menu_cursor = 0;
        return;
    }

    /* Navigate with W/S/Up/Down */
    gs->menu_cursor = menu_navigate(gs->menu_cursor, SETTINGS_ITEM_COUNT);

    /* Left/Right to adjust selected setting */
    bool changed = false;
    float step = 0.1f;

    if (gs->menu_cursor == 0) {
        /* Movement layout: toggle */
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
            if (gs->settings.movement_layout == MOVEMENT_TANK) {
                gs->settings.movement_layout = MOVEMENT_8DIR;
            } else {
                gs->settings.movement_layout = MOVEMENT_TANK;
            }
            changed = true;
        }
    } else {
        /* Float slider: adjust by step */
        float *val = settings_float_ptr(&gs->settings, gs->menu_cursor);
        if (val) {
            if (IsKeyPressed(KEY_LEFT)) {
                *val = settings_clamp01(*val - step);
                changed = true;
            }
            if (IsKeyPressed(KEY_RIGHT)) {
                *val = settings_clamp01(*val + step);
                changed = true;
            }
        }
    }

    if (changed) {
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
    corpse_pool_init(&gs->corpses);
    damage_number_pool_init(&gs->damage_numbers);
    weapon_pickup_pool_init(&gs->weapon_pickups);
    upgrade_pickup_pool_init(&gs->upgrade_pickups);
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

    /* ── Hitstop: freeze gameplay but keep rendering ──────────────────── */
    if (gs->hitstop_timer > 0.0f) {
        gs->hitstop_timer -= GetFrameTime();
        return; /* skip ALL gameplay logic this frame */
    }

    float dt = GetFrameTime();

    /* ── Slow-motion: scale time after kills ──────────────────────────── */
    if (gs->slowmo_timer > 0.0f) {
        gs->slowmo_timer -= dt;
        dt *= gs->slowmo_scale;
    }

    /* ── Camera kick decay ────────────────────────────────────────────── */
    float kick_len = Vector2Length(gs->camera_kick);
    if (kick_len > 0.1f) {
        float decay = CAMERA_KICK_DECAY * dt;
        if (decay > kick_len) {
            gs->camera_kick = (Vector2){0, 0};
        } else {
            gs->camera_kick = Vector2Scale(gs->camera_kick, 1.0f - decay / kick_len);
        }
    } else {
        gs->camera_kick = (Vector2){0, 0};
    }
    gs->stats.survival_time += dt;

    /* Apply upgrade-driven modifiers before movement */
    gs->player.speed_bonus = upgrade_get_speed_bonus(&gs->upgrades);
    gs->player.dash_cd_mult = upgrade_get_dash_cd_mult(&gs->upgrades);

    player_update(&gs->player, dt, gs->arena, &gs->tilemap, gs->camera,
                  gs->settings.movement_layout);

    /* ── Shooting (weapon-driven, upgraded) ──────────────────────────── */
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 muzzle = Vector2Add(gs->player.position,
                                    Vector2Scale(gs->player.aim_direction, PLAYER_RADIUS + 2.0f));
        Weapon *w = &gs->player.current_weapon;
        float eff_fire_rate = w->fire_rate * upgrade_get_fire_rate_mult(&gs->upgrades);
        float eff_damage = w->damage + upgrade_get_damage_bonus(&gs->upgrades);
        float eff_bullet_speed = w->bullet_speed * upgrade_get_bullet_speed_mult(&gs->upgrades);
        int fired =
            bullet_pool_fire_weapon(&gs->bullets, muzzle, gs->player.aim_direction, eff_fire_rate,
                                    eff_damage, eff_bullet_speed, w->spread_angle,
                                    w->projectile_count, w->bullet_lifetime, w->bullet_color);
        if (fired > 0) {
            audio_play_shoot(&gs->audio);

            /* Camera kick (nudge opposite to aim direction) */
            Vector2 kick_dir = {-gs->player.aim_direction.x, -gs->player.aim_direction.y};
            gs->camera_kick =
                Vector2Add(gs->camera_kick, Vector2Scale(kick_dir, CAMERA_KICK_STRENGTH));

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
    corpse_pool_update(&gs->corpses, dt);
    damage_number_pool_update(&gs->damage_numbers, dt);

    /* ── Ambient particles (cosmetic dust motes) ─────────────────────── */
    {
        /* Spawn new motes near the camera viewport */
        float cam_x = gs->camera.target.x;
        float cam_y = gs->camera.target.y;
        for (int i = 0; i < MAX_AMBIENT_PARTICLES; i++) {
            AmbientParticle *ap = &gs->ambient[i];
            if (!ap->active) {
                ap->position.x = cam_x + (float)GetRandomValue(-500, 500);
                ap->position.y = cam_y + (float)GetRandomValue(-400, 400);
                ap->velocity.x = (float)GetRandomValue(-15, 15);
                ap->velocity.y = (float)GetRandomValue(-15, 15);
                ap->alpha = 0.0f;
                ap->alpha_speed = 0.3f + (float)GetRandomValue(0, 40) / 100.0f;
                ap->active = true;
                break; /* spawn one per frame */
            }
        }
        /* Update existing motes */
        for (int i = 0; i < MAX_AMBIENT_PARTICLES; i++) {
            AmbientParticle *ap = &gs->ambient[i];
            if (!ap->active) {
                continue;
            }
            ap->position.x += ap->velocity.x * dt;
            ap->position.y += ap->velocity.y * dt;
            ap->alpha += ap->alpha_speed * dt;
            if (ap->alpha > 0.6f) {
                ap->alpha_speed = -fabsf(ap->alpha_speed); /* start fading out */
            }
            if (ap->alpha < 0.0f) {
                ap->active = false; /* despawn */
            }
            /* Cull if far from camera */
            float dx = ap->position.x - cam_x;
            float dy = ap->position.y - cam_y;
            if (dx * dx + dy * dy > 700.0f * 700.0f) {
                ap->active = false;
            }
        }
    }

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
    upgrade_pickup_pool_update(&gs->upgrade_pickups, dt);

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
    resolve_upgrade_pickup_collisions(gs);

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

/* ── Pickup phase update/draw ───────────────────────────────────────────────── */

static void update_pickup_weapon(GameState *gs) {
    /* Left/Right or A/D to toggle selection (0 = Keep, 1 = Replace) */
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        gs->menu_cursor = 0;
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        gs->menu_cursor = 1;
    }

    /* Enter / Space to confirm */
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        if (gs->menu_cursor == 1) {
            /* Replace: swap weapon */
            gs->player.current_weapon = gs->pending_weapon;
        }
        /* Either way, consume the pickup */
        WeaponPickup *wp = &gs->weapon_pickups.pickups[gs->pending_weapon_index];
        particle_burst(&gs->particles, wp->position, 8, 30.0f, 100.0f, 0.1f, 0.3f, 2.5f,
                       wp->weapon.bullet_color);
        wp->active = false;
        gs->phase = PHASE_PLAYING;
    }
}

static void update_pickup_upgrade(GameState *gs) {
    /* Up/Down or W/S to toggle (0 = Keep, 1 = Discard) */
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        gs->menu_cursor = 0;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        gs->menu_cursor = 1;
    }

    /* Enter / Space to confirm */
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        UpgradePickup *up = &gs->upgrade_pickups.pickups[gs->pending_upgrade_index];
        if (gs->menu_cursor == 0) {
            /* Keep: apply the upgrade */
            if (upgrade_add(&gs->upgrades, up->type)) {
                /* Apply max HP upgrade immediately */
                if (up->type == UPGRADE_MAX_HP) {
                    float bonus = UPGRADE_MAX_HP_BONUS;
                    gs->player.max_hp += bonus;
                    gs->player.hp += bonus;
                }
            }
        }
        /* Either way, consume the pickup */
        particle_burst(&gs->particles, up->position, 8, 30.0f, 80.0f, 0.1f, 0.3f, 2.5f,
                       upgrade_get_color(up->type));
        up->active = false;
        gs->phase = PHASE_PLAYING;
    }
}

/* ── Card drawing helper ───────────────────────────────────────────────────── */

/* Draw a rounded-corner card frame at (x, y) with given size and border color. */
static void draw_card(int x, int y, int w, int h, Color border, bool selected) {
    Color bg = {15, 15, 25, 240};
    DrawRectangle(x, y, w, h, bg);
    Color outline = selected ? border : (Color){80, 80, 80, 255};
    DrawRectangleLines(x, y, w, h, outline);
    if (selected) {
        /* Double border for emphasis */
        DrawRectangleLines(x - 1, y - 1, w + 2, h + 2, outline);
    }
}

/* Draw a selectable button at (x, y) centered in given width. */
static void draw_button(int cx, int y, const char *label, int font_size, Color border_color,
                        bool selected) {
    int w = MeasureText(label, font_size) + 30;
    int h = font_size + 12;
    int x = cx - w / 2;
    Color bg = selected ? (Color){border_color.r, border_color.g, border_color.b, 40}
                        : (Color){20, 20, 30, 200};
    DrawRectangle(x, y, w, h, bg);
    DrawRectangleLines(x, y, w, h, border_color);
    Color text_color = selected ? RAYWHITE : GRAY;
    DrawText(label, cx - MeasureText(label, font_size) / 2, y + 6, font_size, text_color);
}

/* Draw weapon stats inside a card area. */
static void draw_weapon_stats(int x, int y, const Weapon *w) {
    int fs = 14;
    int line_h = 18;
    DrawText(w->name, x, y, 20, w->bullet_color);
    y += 26;
    DrawText(weapon_get_description(w->type), x, y, 12, GRAY);
    y += 20;
    DrawText(TextFormat("Damage: %.1f", (double)w->damage), x, y, fs, RAYWHITE);
    y += line_h;
    if (w->projectile_count > 1) {
        DrawText(TextFormat("Pellets: %d", w->projectile_count), x, y, fs, RAYWHITE);
        y += line_h;
    }
    DrawText(TextFormat("Fire Rate: %.2fs", (double)w->fire_rate), x, y, fs, RAYWHITE);
    y += line_h;
    DrawText(TextFormat("Speed: %.0f", (double)w->bullet_speed), x, y, fs, RAYWHITE);
    y += line_h;
    if (w->spread_angle > 0.0f) {
        DrawText(TextFormat("Spread: %.0f deg", (double)w->spread_angle), x, y, fs,
                 (Color){255, 200, 100, 255});
    } else {
        DrawText("Spread: None", x, y, fs, (Color){100, 255, 100, 255});
    }
}

static void draw_pickup_weapon(const GameState *gs) {
    /* Dark overlay over the frozen game world */
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 180});

    /* Title */
    const char *title = "WEAPON FOUND";
    int title_w = MeasureText(title, 28);
    DrawText(title, (SCREEN_WIDTH - title_w) / 2, 40, 28, (Color){0, 220, 200, 255});

    /* Two side-by-side cards: Current (left) vs New (right) */
    int card_w = 280;
    int card_h = 260;
    int gap = 40;
    int total_w = card_w * 2 + gap;
    int left_x = (SCREEN_WIDTH - total_w) / 2;
    int right_x = left_x + card_w + gap;
    int card_y = 85;

    /* "Current" label */
    const char *cur_label = "CURRENT";
    int cur_lw = MeasureText(cur_label, 14);
    DrawText(cur_label, left_x + (card_w - cur_lw) / 2, card_y - 18, 14, GRAY);

    /* "New" label */
    const char *new_label = "NEW";
    int new_lw = MeasureText(new_label, 14);
    DrawText(new_label, right_x + (card_w - new_lw) / 2, card_y - 18, 14,
             gs->pending_weapon.bullet_color);

    /* Current weapon card */
    draw_card(left_x, card_y, card_w, card_h, RAYWHITE, gs->menu_cursor == 0);
    draw_weapon_stats(left_x + 16, card_y + 16, &gs->player.current_weapon);

    /* New weapon card (highlighted with weapon color border) */
    draw_card(right_x, card_y, card_w, card_h, gs->pending_weapon.bullet_color,
              gs->menu_cursor == 1);
    draw_weapon_stats(right_x + 16, card_y + 16, &gs->pending_weapon);

    /* Buttons */
    int btn_y = card_y + card_h + 20;
    draw_button(left_x + card_w / 2, btn_y, "Keep Current", 16, RAYWHITE, gs->menu_cursor == 0);
    draw_button(right_x + card_w / 2, btn_y, "Replace", 16, gs->pending_weapon.bullet_color,
                gs->menu_cursor == 1);

    /* Hint */
    const char *hint = "A/D to choose  |  Enter to confirm";
    int hint_w = MeasureText(hint, 14);
    DrawText(hint, (SCREEN_WIDTH - hint_w) / 2, SCREEN_HEIGHT - 40, 14, DARKGRAY);
}

static void draw_pickup_upgrade(const GameState *gs) {
    /* Dark overlay */
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 180});

    /* Title */
    const char *title = "UPGRADE FOUND";
    int title_w = MeasureText(title, 28);
    DrawText(title, (SCREEN_WIDTH - title_w) / 2, 60, 28, (Color){0, 220, 200, 255});

    /* Single centered card */
    Color uc = upgrade_get_color(gs->pending_upgrade);
    int card_w = 340;
    int card_h = 200;
    int card_x = (SCREEN_WIDTH - card_w) / 2;
    int card_y = 110;

    draw_card(card_x, card_y, card_w, card_h, uc, true);

    /* Upgrade name (large, colored) */
    const char *uname = upgrade_get_name(gs->pending_upgrade);
    const char *full_name = NULL;
    switch (gs->pending_upgrade) {
    case UPGRADE_SPEED:
        full_name = "Speed Boost";
        break;
    case UPGRADE_DAMAGE:
        full_name = "Damage Up";
        break;
    case UPGRADE_FIRE_RATE:
        full_name = "Rapid Fire";
        break;
    case UPGRADE_MAX_HP:
        full_name = "Vitality";
        break;
    case UPGRADE_BULLET_SPEED:
        full_name = "Velocity";
        break;
    case UPGRADE_DASH_CD:
        full_name = "Swift Dash";
        break;
    default:
        full_name = "Unknown";
        break;
    }

    int name_x = card_x + 20;
    int name_y = card_y + 16;
    DrawText(full_name, name_x, name_y, 24, uc);

    /* Short code tag */
    DrawText(TextFormat("[%s]", uname), name_x + MeasureText(full_name, 24) + 10, name_y + 4, 16,
             (Color){uc.r, uc.g, uc.b, 160});

    /* Description */
    const char *desc = upgrade_get_description(gs->pending_upgrade);
    DrawText(desc, name_x, name_y + 34, 16, RAYWHITE);

    /* Current stacks */
    int stacks = gs->upgrades.stacks[gs->pending_upgrade];
    DrawText(TextFormat("Current stacks: %d / %d", stacks, MAX_UPGRADE_STACKS), name_x, name_y + 60,
             14, GRAY);

    /* Rarity bar (colored stripe on the right side of card) */
    int bar_w = 4;
    DrawRectangle(card_x + card_w - bar_w - 4, card_y + 8, bar_w, card_h - 16, uc);

    /* Buttons below card */
    int btn_y = card_y + card_h + 25;
    Color keep_color = (Color){60, 200, 120, 255};
    Color discard_color = (Color){200, 60, 60, 255};
    int btn_cx = SCREEN_WIDTH / 2;
    draw_button(btn_cx, btn_y, "Keep", 18, keep_color, gs->menu_cursor == 0);
    draw_button(btn_cx, btn_y + 50, "Discard", 18, discard_color, gs->menu_cursor == 1);

    /* Hint */
    const char *hint = "W/S to choose  |  Enter to confirm";
    int hint_w = MeasureText(hint, 14);
    DrawText(hint, (SCREEN_WIDTH - hint_w) / 2, SCREEN_HEIGHT - 40, 14, DARKGRAY);
}

/* ── Phase-specific draw helpers ───────────────────────────────────────────── */

static void draw_first_run(const GameState *gs) {
    /* Title */
    const char *title = "ASTRO BLITZ";
    int title_w = MeasureText(title, 50);
    DrawText(title, (SCREEN_WIDTH - title_w) / 2, 50, 50, (Color){0, 220, 200, 255});

    /* Prompt */
    const char *prompt = "Choose your movement style:";
    int prompt_w = MeasureText(prompt, 20);
    DrawText(prompt, (SCREEN_WIDTH - prompt_w) / 2, 120, 20, RAYWHITE);

    /* Two side-by-side cards */
    Color teal = {0, 220, 200, 255};
    int card_w = 280;
    int card_h = 260;
    int gap = 40;
    int total_w = card_w * 2 + gap;
    int left_x = (SCREEN_WIDTH - total_w) / 2;
    int right_x = left_x + card_w + gap;
    int card_y = 165;

    /* 8-Directional card */
    draw_card(left_x, card_y, card_w, card_h, teal, gs->menu_cursor == 0);
    int lx = left_x + 20;
    int ly = card_y + 16;
    DrawText("8-Directional", lx, ly, 22, gs->menu_cursor == 0 ? teal : GRAY);
    ly += 32;
    DrawText("Simple and intuitive.", lx, ly, 14, GRAY);
    ly += 24;
    DrawText("W = Up", lx, ly, 16, RAYWHITE);
    ly += 20;
    DrawText("S = Down", lx, ly, 16, RAYWHITE);
    ly += 20;
    DrawText("A = Left", lx, ly, 16, RAYWHITE);
    ly += 20;
    DrawText("D = Right", lx, ly, 16, RAYWHITE);
    ly += 28;
    DrawText("Aim with mouse.", lx, ly, 14, (Color){150, 150, 150, 255});
    ly += 18;
    DrawText("Movement is screen-relative.", lx, ly, 14, (Color){150, 150, 150, 255});

    /* Tank Controls card */
    draw_card(right_x, card_y, card_w, card_h, teal, gs->menu_cursor == 1);
    int rx = right_x + 20;
    int ry = card_y + 16;
    DrawText("Tank Controls", rx, ry, 22, gs->menu_cursor == 1 ? teal : GRAY);
    ry += 32;
    DrawText("Strafing relative to aim.", rx, ry, 14, GRAY);
    ry += 24;
    DrawText("W = Toward cursor", rx, ry, 16, RAYWHITE);
    ry += 20;
    DrawText("S = Away from cursor", rx, ry, 16, RAYWHITE);
    ry += 20;
    DrawText("A = Strafe left", rx, ry, 16, RAYWHITE);
    ry += 20;
    DrawText("D = Strafe right", rx, ry, 16, RAYWHITE);
    ry += 28;
    DrawText("Aim with mouse.", rx, ry, 14, (Color){150, 150, 150, 255});
    ry += 18;
    DrawText("Movement relative to aim.", rx, ry, 14, (Color){150, 150, 150, 255});

    /* Buttons under each card */
    int btn_y = card_y + card_h + 20;
    draw_button(left_x + card_w / 2, btn_y, "Select", 16, teal, gs->menu_cursor == 0);
    draw_button(right_x + card_w / 2, btn_y, "Select", 16, teal, gs->menu_cursor == 1);

    /* Note */
    const char *note = "(You can change this later in Settings)";
    int note_w = MeasureText(note, 14);
    DrawText(note, (SCREEN_WIDTH - note_w) / 2, btn_y + 48, 14, DARKGRAY);

    /* Hint */
    const char *hint = "A/D to choose  |  Enter to confirm";
    int hint_w = MeasureText(hint, 14);
    DrawText(hint, (SCREEN_WIDTH - hint_w) / 2, SCREEN_HEIGHT - 40, 14, DARKGRAY);
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

/* Draw a horizontal slider bar at (x,y) with given width/height and fill ratio (0-1). */
static void draw_slider_bar(int x, int y, int w, int h, float ratio, Color fill_color,
                            bool selected) {
    Color bg = selected ? (Color){60, 60, 60, 255} : (Color){40, 40, 40, 255};
    Color border = selected ? (Color){0, 220, 200, 255} : (Color){80, 80, 80, 255};
    DrawRectangle(x, y, w, h, bg);
    DrawRectangle(x, y, (int)((float)w * ratio), h, fill_color);
    DrawRectangleLines(x, y, w, h, border);
}

static void draw_settings(const GameState *gs) {
    /* Dark overlay if coming from pause (game is behind) */
    if (gs->settings_return_phase != PHASE_MAIN_MENU) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 200});
    }

    const char *title = "SETTINGS";
    int title_size = 40;
    int title_w = MeasureText(title, title_size);
    DrawText(title, (SCREEN_WIDTH - title_w) / 2, 60, title_size, (Color){0, 220, 200, 255});

    int font_size = 18;
    int start_y = 130;
    int row_h = 32;
    int label_x = 120;
    int slider_x = 360;
    int slider_w = 200;
    int slider_h = 14;

    /* Setting labels */
    const char *labels[SETTINGS_ITEM_COUNT] = {"Movement", "Screen Shake", "Hitstop",
                                               "Bloom",    "Scanlines",    "Chromatic Ab.",
                                               "Vignette", "Lighting"};

    for (int i = 0; i < SETTINGS_ITEM_COUNT; i++) {
        int y = start_y + i * row_h;
        bool sel = (gs->menu_cursor == i);
        Color label_color = sel ? (Color){0, 220, 200, 255} : GRAY;

        /* Selection arrow */
        if (sel) {
            DrawText(">", label_x - 20, y, font_size, label_color);
        }

        DrawText(labels[i], label_x, y, font_size, label_color);

        if (i == 0) {
            /* Movement layout: toggle text */
            const char *value = (gs->settings.movement_layout == MOVEMENT_TANK)
                                    ? "< Tank Controls >"
                                    : "< 8-Directional >";
            DrawText(value, slider_x, y, font_size, sel ? RAYWHITE : GRAY);
        } else {
            /* Float slider */
            float val = 0.0f;
            switch (i) {
            case 1:
                val = gs->settings.screen_shake;
                break;
            case 2:
                val = gs->settings.hitstop;
                break;
            case 3:
                val = gs->settings.bloom;
                break;
            case 4:
                val = gs->settings.scanlines;
                break;
            case 5:
                val = gs->settings.chromatic_aberration;
                break;
            case 6:
                val = gs->settings.vignette;
                break;
            case 7:
                val = gs->settings.lighting;
                break;
            }
            Color fill = sel ? (Color){0, 200, 180, 255} : (Color){60, 140, 130, 255};
            draw_slider_bar(slider_x, y + 3, slider_w, slider_h, val, fill, sel);

            /* Percentage label */
            const char *pct = TextFormat("%d%%", (int)(val * 100.0f));
            DrawText(pct, slider_x + slider_w + 10, y, font_size, sel ? RAYWHITE : GRAY);
        }
    }

    /* Movement description below all settings */
    const char *desc = NULL;
    if (gs->settings.movement_layout == MOVEMENT_TANK) {
        desc = "W/S = forward/back toward aim, A/D = strafe";
    } else {
        desc = "W/S/A/D = screen-relative directions";
    }
    int desc_y = start_y + SETTINGS_ITEM_COUNT * row_h + 16;
    int desc_w = MeasureText(desc, 14);
    DrawText(desc, (SCREEN_WIDTH - desc_w) / 2, desc_y, 14, (Color){100, 100, 100, 255});

    /* Instructions */
    const char *hint = "W/S to navigate  |  Left/Right to adjust  |  ESC to go back";
    int hint_w = MeasureText(hint, 14);
    DrawText(hint, (SCREEN_WIDTH - hint_w) / 2, SCREEN_HEIGHT - 50, 14, DARKGRAY);
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
    corpse_pool_init(&gs->corpses);
    damage_number_pool_init(&gs->damage_numbers);
    weapon_pickup_pool_init(&gs->weapon_pickups);
    upgrade_pickup_pool_init(&gs->upgrade_pickups);
    upgrade_state_init(&gs->upgrades);
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

    /* Juice state */
    gs->hitstop_timer = 0.0f;
    gs->slowmo_timer = 0.0f;
    gs->slowmo_scale = SLOWMO_SCALE;
    gs->camera_kick = (Vector2){0, 0};

    /* Camera centered on player */
    gs->camera.target = center;
    gs->camera.offset = (Vector2){RENDER_WIDTH / 2.0f, RENDER_HEIGHT / 2.0f};
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
    case PHASE_PICKUP_WEAPON:
        update_pickup_weapon(gs);
        break;
    case PHASE_PICKUP_UPGRADE:
        update_pickup_upgrade(gs);
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

void game_draw_world(const GameState *gs) {
    switch (gs->phase) {
    case PHASE_FIRST_RUN:
    case PHASE_MAIN_MENU:
        /* No world to draw on menu screens */
        break;

    case PHASE_PLAYING:
    case PHASE_PAUSED:
    case PHASE_GAME_OVER:
    case PHASE_PICKUP_WEAPON:
    case PHASE_PICKUP_UPGRADE: {
        /* Apply screen shake + camera kick to camera for world rendering */
        Camera2D draw_cam = screenshake_apply(&gs->shake, gs->camera);
        draw_cam.target.x += gs->camera_kick.x;
        draw_cam.target.y += gs->camera_kick.y;

        BeginMode2D(draw_cam);
        tilemap_draw(&gs->tilemap, gs->camera, gs->stats.survival_time);
        corpse_pool_draw(&gs->corpses);

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
        upgrade_pickup_pool_draw(&gs->upgrade_pickups);
        bullet_pool_draw(&gs->bullets);
        enemy_bullet_pool_draw(&gs->enemy_bullets);
        enemy_pool_draw(&gs->enemies);
        particle_pool_draw(&gs->particles);
        damage_number_pool_draw(&gs->damage_numbers);

        /* Ambient dust motes */
        for (int i = 0; i < MAX_AMBIENT_PARTICLES; i++) {
            const AmbientParticle *ap = &gs->ambient[i];
            if (!ap->active || ap->alpha <= 0.0f) {
                continue;
            }
            unsigned char a = (unsigned char)(ap->alpha * 255.0f);
            DrawCircleV(ap->position, AMBIENT_PARTICLE_RADIUS, (Color){0, 180, 170, a});
        }

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
        break;
    }

    case PHASE_SETTINGS:
        /* Draw world behind if returning to pause */
        if (gs->settings_return_phase != PHASE_MAIN_MENU) {
            BeginMode2D(gs->camera);
            tilemap_draw(&gs->tilemap, gs->camera, gs->stats.survival_time);
            bullet_pool_draw(&gs->bullets);
            enemy_pool_draw(&gs->enemies);
            particle_pool_draw(&gs->particles);
            player_draw(&gs->player);
            EndMode2D();
        }
        break;
    }
}

void game_draw_ui(const GameState *gs) {
    switch (gs->phase) {
    case PHASE_FIRST_RUN:
        draw_first_run(gs);
        break;

    case PHASE_MAIN_MENU:
        draw_main_menu(gs);
        break;

    case PHASE_PLAYING:
    case PHASE_PAUSED:
    case PHASE_GAME_OVER:
        draw_hud(gs);
        if (gs->phase == PHASE_GAME_OVER) {
            draw_game_over(gs);
        } else if (gs->phase == PHASE_PAUSED) {
            draw_paused(gs);
        }
        break;

    case PHASE_SETTINGS:
        if (gs->settings_return_phase != PHASE_MAIN_MENU) {
            draw_hud(gs);
        }
        draw_settings(gs);
        break;

    case PHASE_PICKUP_WEAPON:
        draw_hud(gs);
        draw_pickup_weapon(gs);
        break;

    case PHASE_PICKUP_UPGRADE:
        draw_hud(gs);
        draw_pickup_upgrade(gs);
        break;
    }
}
