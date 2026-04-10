# Session Handoff: Visual Juice & Gameplay Depth

## Branch
`feature/visual-juice-and-gameplay-depth` at commit `7885c3b` (merged latest main).

## Repo Location
`C:\Users\bilal.medkouri\workshop-astro-blitz`

## What's Done (committed)

1. **Fix STATUS.md** (`fe02fa2`) -- Removed stray `<<<<<<< HEAD` merge conflict marker.
2. **Particle system** (`7e721a2`) -- `src/particle.h/c`, 512-particle pool with emit/burst API. Wired into game.c: muzzle flash (4 particles on shoot), hit sparks (6 on bullet-enemy hit), death explosions (15+8 on kill), dash trail (continuous emit while dashing). 12 unit tests in `tests/test_particle.c`.
3. **Screen shake** (`b30bd2b`) -- `src/screenshake.h/c`, trauma-based system with squared falloff. Triggers: 0.15 trauma on enemy kill, 0.35 on player damage. Applied to camera during draw. 11 unit tests in `tests/test_screenshake.c`.

## What's In Progress (uncommitted, stashed and restored)

**A3: Hit feedback** -- partially implemented across 4 files:

- `src/enemy.h` -- Added `float hit_flash` field to `Enemy` struct.
- `src/enemy.c` -- `hit_flash` initialized to 0 on spawn, ticked down in update, drawing flashes white when `hit_flash > 0`.
- `src/game.h` -- Added `DamageNumber` struct, `DamageNumberPool` (32 entries), constants (`DAMAGE_NUMBER_LIFETIME 0.8f`, `DAMAGE_NUMBER_RISE_SPEED 60.0f`). Added `DamageNumberPool damage_numbers` to `GameState`.
- `src/game.c` -- Added `damage_number_pool_init/spawn/update/draw` static helpers. Wired: damage numbers spawn on bullet-enemy hit (yellow "1") and enemy-player hit (red damage value). `HIT_FLASH_DURATION 0.1f` constant. Init/update/draw calls wired into game loop. Added `#include <string.h>`.

**NOT YET DONE for A3:**
- Needs `task build` + `task test` to verify it compiles and passes.
- Needs to be committed.
- No unit tests written yet for damage numbers (they're static helpers in game.c, so integration test coverage is more appropriate).

## What's Remaining (the full plan, 21 items)

### Phase A: Visual Juice
- [x] A1: Particle system
- [x] A2: Screen shake
- [~] A3: Hit feedback (enemy flash, death animation, damage numbers) -- IN PROGRESS
- [ ] A4: Bullet trails (fading afterimages behind projectiles)
- [ ] A5: Post-processing pipeline (RenderTexture2D + shader chain in main.c)
- [ ] A6: Bloom/glow shader (neon aesthetic)
- [ ] A7: CRT scanlines + chromatic aberration shader
- [ ] A8: Dynamic vignette shader (intensifies at low HP)
- [ ] A9: Combo/killstreak system (multiplier text, escalating effects)

### Phase B: Gameplay Depth
- [ ] B1: Melee attack (right-click arc, damage, knockback, cooldown)
- [ ] B2: Grunt enemy (ranged AI, enemy bullet pool, projectile-player collision)
- [ ] B3: Stalker enemy (fast flanker, circle + dash AI)
- [ ] B4: Bomber enemy (charges at player, AoE explosion on death)
- [ ] B5: Elite enemy modifiers (Burning, Armored, Swift -- visual glow + buffed stats)
- [ ] B6: Weapon system (weapon struct: fire rate, damage, speed, spread, projectile count)
- [ ] B7: SMG + Shotgun + Plasma Rifle weapons
- [ ] B8: Weapon pickups (drop on kill, walk to swap, HUD display)
- [ ] B9: Passive upgrade system (6 types: Speed+, Damage+, Fire Rate+, Max HP+, Bullet Speed+, Dash CD-. Stackable. HUD icons.)
- [ ] B10: Floor progression (exit tile, difficulty scaling, floor counter HUD)
- [ ] B11: Minimap (top-right, fog of war, enemy blips, wall outlines)
- [ ] B12: Gameplay music (procedural synth during combat, intensity scales with combo/enemies)

### Final
- [ ] Integration tests for all new cross-module features
- [ ] STATUS.md + CHANGELOG.md update

## Key Architecture Notes for Next Session

- **All new .c files** must be added to `astro_blitz_objs` OBJECT library in CMakeLists.txt (NOT astro_blitz_lib).
- **Tests** link against `astro_blitz_lib` (unit) or `astro_blitz_objs + raylib_stubs` (integration).
- **Raylib stubs** (`tests/raylib_stubs.c`) must stub any new Raylib function called by game code. Currently stubs: input, time, rendering, audio, GetRandomValue.
- **Post-processing shaders** (A5-A8) go in main.c wrapping the game_draw call. Use `LoadShaderFromMemory` with GLSL 330 string constants. Render to `RenderTexture2D`, then draw that texture through shaders.
- **New enemy types** (B2-B4) extend the `EnemyType` enum and add cases in `enemy_pool_spawn`/`enemy_pool_update`/`enemy_pool_draw`.
- **Weapon system** (B6) should be a new module `src/weapon.h/c` with a `Weapon` struct. Player gets a `Weapon current_weapon` field.
- **Breadth over depth** -- user wants maximum scope at 80% polish. Ship fast, don't over-engineer.

## Build & Test Commands

```
cmake -B build                              # configure
cmake --build build --config Debug          # build
ctest --test-dir build -C Debug --output-on-failure  # test
```

Or use task runner: `task build`, `task test`.
