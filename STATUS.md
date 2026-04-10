# Status

Persistent memory for the Astro Blitz project. Keep this file under ~150 lines.
When it grows too long, summarize older entries and remove resolved items.

## Current State

- **Phase:** Visual juice and gameplay depth -- weapon system, multiple enemy types, melee attack, floor progression, combo system, bullet trails, elite modifiers, weapon pickups.
- **Engine/Framework:** Raylib 5.5 (C99), built via CMake FetchContent.
- **Build:** `task build` (or `cmake -B build && cmake --build build --config Release`)
- **Test framework:** Unity (ThrowTheSwitch) v2.6.1 via FetchContent + CTest. 13 test suites, all passing.
- **Branch:** `feature/visual-juice-and-gameplay-depth` -- 16 commits ahead of main.
- **Playable:** Yes. Four weapon types (Pistol, SMG, Shotgun, Plasma), four enemy types (Swarmer, Grunt, Stalker, Bomber), elite modifiers, weapon drops, melee attack, combo system, floor progression with exit portals. All 6 passive upgrades now functional.

## Recent Changes

| Date | Change |
|------|--------|
| 2026-04-11 | Wired floor difficulty scaling: enemy HP scales by +15% per floor. Wired Speed and Dash CD upgrades to player movement and dash cooldown. 7 new tests. |
| 2026-04-11 | Fixed linting infrastructure: installed LLVM/clang-format, fixed Taskfile fmt:c (PowerShell script workaround), fixed yamllint CRLF issues, suppressed pre-existing markdownlint MD060/MD036 rules. All three linters pass. |
| 2026-04-11 | Added weapon system (src/weapon.h/c): 4 weapon presets (Pistol, SMG, Shotgun, Plasma) with per-weapon fire rate, damage, spread, projectile count, bullet speed/lifetime/color. Bullet struct now carries damage and color. 12 weapon unit tests. |
| 2026-04-11 | Added 3 new enemy types: Grunt (ranged, fires enemy bullets), Stalker (circle + dash AI), Bomber (charges, AoE explosion on death). Enemy bullet pool for Grunt projectiles. Progressive wave spawning. |
| 2026-04-11 | Added weapon pickup drops: non-swarmer enemies have 30% drop chance, walk-over to swap weapon. Pulsing visual with weapon initial letter. |
| 2026-04-11 | Added bullet trails: fading afterimage lines behind player and enemy projectiles. |
| 2026-04-11 | Added combo/killstreak system: consecutive kills within 2s build combo counter with escalating HUD text and screen shake. Best combo shown on game-over screen. |
| 2026-04-11 | Added elite enemy modifiers: Armored (+HP, slower), Swift (faster, smaller), Burning (extra damage). 20% chance after wave 5. Colored ring visual indicator. |
| 2026-04-11 | Added right-click melee attack: 120-degree arc, 3 damage, knockback, 0.5s cooldown, visual arc slash, HUD cooldown bar. |
| 2026-04-11 | Added floor progression: exit portal after 5 waves, walk in to advance floor with new map. Floor counter in HUD and game-over screen. |
| 2026-04-11 | Added 5 new integration tests: weapon system (shotgun pellets), grunt shooting, combo increments, elite stats, weapon pickup swap. |
| 2026-04-10 | Previous session: particle system, screen shake, hit feedback (enemy flash, damage numbers). |

## Known Issues / Next Steps

- **Remaining features:** Post-processing shaders (A5-A8: bloom, CRT, vignette), minimap (B11), gameplay music (B12)
- **Redundant include path in CMakeLists.txt:** Harmless but could be cleaned up.
- Room-based level generation would improve map variety

## Workarounds & Patterns

- **go-task preconditions on Windows:** Use Git Bash (`sh.exe`) via Git installation.
- **clang-format `Language` field:** Use `Language: Cpp` (not `C`) for clang-format 18+ compatibility.
- **Taskfile `fmt:c` on Windows:** Go template `$_` escaping prevents inline PowerShell. Uses `scripts/clang-format-all.ps1` with `-ExecutionPolicy Bypass`.
- **YAML config files on Windows:** Must be saved with LF line endings for yamllint. Git `autocrlf` may re-CRLF on checkout.
- **astro_blitz_objs + dual-link pattern:** Game logic compiled once as OBJECT lib. `astro_blitz_lib` = objs + real Raylib. `test_integration` = objs + stubs.
- **Raylib stubs:** Must stub any new Raylib function used by game code (e.g., `IsMouseButtonPressed` added for melee).
