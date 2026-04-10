# Changelog

All notable changes to Astro Blitz are documented here.
Format based on [Keep a Changelog](https://keepachangelog.com/).

## [Unreleased]

### Added

- First-run movement picker: on first launch (no `settings.ini`), a dedicated screen lets the player choose between 8-Directional and Tank Controls before proceeding to the main menu
- Main menu screen with Play, Settings, and Quit options
- Pause menu (ESC during gameplay): Resume, Settings, Main Menu, Quit
- Settings menu with movement layout toggle (Tank Controls vs 8-Directional)
- Persistent settings saved to `settings.ini` (text-based key=value format, survives game restarts)
- 8-directional movement mode: WASD maps to fixed screen directions (up/down/left/right) independent of aim direction
- New `settings` module (`src/settings.h`, `src/settings.c`) with init, save, load API and path-parameterized variants for testing
- 14 unit tests for settings module (`tests/test_settings.c`) covering defaults, save/load round-trips, missing files, malformed input, comments
- 8 unit tests for 8-directional movement (`test_player.c`) covering all cardinal directions, diagonals, normalization, cancellation
- 4 unit tests for new game phases (`test_game.c`) covering phase enum, settings preservation, menu cursor, and death-check phase guard
- `.opencode/agents/` -- Lightweight Haiku subagents (lint-check, build-and-test, changelog-drafter) for delegating mechanical checks at lower cost
- Bullet wall bouncing: bullets reflect off solid tiles up to 3 times before being destroyed (new `BULLET_MAX_BOUNCES` constant and `bounces` field on `Bullet`)
- 3 new bullet tests: `test_bullet_bounces_off_wall`, `test_bullet_deactivates_after_max_bounces`, `test_fire_initializes_bounces_to_zero`
- BFS flow field pathfinding for enemies: enemies now navigate around obstacles instead of getting stuck against walls
- `tilemap_compute_flow_field()` function computes a grid-wide BFS from the player's position each frame
- Tilemap struct now stores flow direction (`flow`) and BFS distance (`flow_dist`) arrays
- 8 new unit tests for flow field computation (`test_tilemap.c`) and 3 new tests for enemy flow field movement (`test_enemy.c`)
- Tilemap system (`src/tilemap.c/h`): procedural world generation with a 128x96 tile grid (~4096x3072 pixels). Generates border walls, randomly scattered obstacles, and obstacle clusters. Spawn area is guaranteed clear.
- Camera2D follows the player, with edge clamping to keep the viewport within world bounds
- Tile collision for player, enemies, and bullets: entities slide along walls, bullets are destroyed on wall impact
- Enemy spawning is now camera-relative: swarmers appear just outside the visible viewport
- 27 unit tests for tilemap module (`tests/test_tilemap.c`) covering generation, collision queries, coordinate conversion, and constants
- Camera and tilemap tests added to `test_game.c`
- Audio system with procedurally generated sounds -- no external asset files required
- Bullet fire SFX: punchy descending square wave burst (880 Hz to 440 Hz, exponential decay)
- Enemy-hit-player SFX: low-frequency impact thud with distortion and noise (120 Hz descending, rapid decay)
- Bullet-hit-enemy SFX: metallic ping with inharmonic overtone (1200 Hz descending, sharp attack)
- Death screen music: looping sci-fi synth -- detuned dual oscillators, low A1 drone bass, tritone/minor melody (Bb2-E3-Bb3-B3-F3 sequence), LFO amplitude wobble, saw-like filtered timbre
- New module: `src/audio.c/h` with `GameAudio` struct and init/cleanup/update/play/stop API
- Unit tests for audio module (`tests/test_audio.c`) -- 18 tests covering state guards, constants, and safe uninitialized behavior
- `bullet_pool_fire` now returns `bool` (true if a bullet was actually fired) for SFX triggering
- 2 new bullet tests: `test_fire_returns_true_on_success`, `test_fire_returns_false_when_rate_limited`

### Changed

- Rewrote `design/DESIGN.md` to resolve the project's major design decisions
- `design/DESIGN.md` now defines run structure: hybrid arena-dungeon model, steep difficulty curve, 5--7 floor runs with find-the-exit completion
- `design/DESIGN.md` now documents combat and progression: medium damage scale (10--100 HP), prefix/suffix weapon modifiers, elite enemy modifiers, talent-based meta-progression, Risk of Rain-style loot weighting
- `design/DESIGN.md` now includes a full development roadmap with short-, medium-, and long-term sprint goals
- Default movement layout changed from Tank Controls to 8-Directional (screen-relative WASD)
- `settings_init` now returns `bool` (true if existing file loaded, false on first run)
- ESC key now pauses the game instead of closing the window (Raylib's exit key disabled via `SetExitKey(0)`)
- Game starts at a main menu instead of immediately entering gameplay
- `GamePhase` enum expanded: `PHASE_MAIN_MENU`, `PHASE_PLAYING`, `PHASE_PAUSED`, `PHASE_SETTINGS`, `PHASE_GAME_OVER`
- `GameState` now holds a `Settings` struct (persisted across game restarts)
- `player_update` accepts a `MovementLayout` parameter to select between tank and 8-directional controls
- `game_init` preserves settings and audio state across restarts
- Fixed Windows linker error: removed explicit `winmm` link that conflicted with Raylib's `PlaySound` symbol
- `AGENTS.md` -- Rewrite: tighten to under 200 lines, add gates from autobuilds-testify (After PR feedback, Before merging, cherry-pick rule, branch name regex), add delegation patterns and context hygiene, extract reference material to `docs/REFERENCE.md`
- `.gitignore` -- Change `.opencode/` from blanket ignore to selective: session data stays ignored, `skills/`, `agents/`, and `commands/` subdirectories are now committed
- `.markdownlint-cli2.yaml`, `.yamllint.yml` -- Exclude `build/` directory from linters to avoid noise from fetched dependencies (Raylib, Unity)
- Enemy spawn waves reduced from 4-8 swarmers per wave to 2-5 for more balanced difficulty
- Enemy swarmers now use BFS flow field for pathfinding when far from the player (>3 tiles); fall back to direct seek at close range for smooth final approach
- Game world is now much larger than the screen (4096x3072 vs 800x600); player moves freely across the full area
- All entity rendering happens in world-space via `BeginMode2D`/`EndMode2D`; HUD remains in screen-space
- Mouse aiming uses `GetScreenToWorld2D` to correctly aim in world coordinates through the camera
- `bullet_pool_update` and `enemy_pool_update` accept an optional `Tilemap*` for wall collision (NULL to skip)
- `player_update` accepts `Tilemap*` and `Camera2D` for wall collision and world-space aiming
- Updated `test_bullet.c` and `test_enemy.c` to use larger test arena and pass NULL tilemap
- Game-over state: when player HP reaches zero, gameplay freezes and a "GAME OVER" overlay displays run stats (kills, survival time as MM:SS, waves spawned) with a prompt to press R to restart
- `GamePhase` enum (`PHASE_PLAYING`, `PHASE_GAME_OVER`) and `GameStats` struct for tracking per-run statistics
- Kill counter increments when enemies are destroyed by bullets
- Survival time accumulates each frame during gameplay
- Wave counter increments each time a new enemy wave spawns
- 6 new unit tests for game-over phase transitions, stats initialization, and death check
- Enemy swarmer system: `src/enemy.c/h` with fixed-size enemy pool, swarmer AI (seek toward player), wave spawning from arena edges
- Player visual upgraded from plain green circle to layered sci-fi mech: dark teal body with neon cyan outline, directional nose triangle for clear facing, bright reactor core, hot magenta aim line with crosshair dot, and semi-transparent glow aura; electric blue palette shift when dashing
- Swarmer visual upgraded from plain red circle to layered alien bug: dark maroon body with neon red-orange outline, mandible "V" lines and bright yellow eye dots that face movement direction, and semi-transparent red glow aura
- Player movement now uses tank controls: W moves toward mouse cursor, S moves away, A/D strafe left/right relative to aim direction
- Dash direction also follows the new tank controls when WASD keys are held
- Bullet-enemy and enemy-player collision detection with circle-circle overlap
- Player takes damage on enemy contact (swarmers die on contact); dash invincibility respected
- Enemy spawn waves every 3 seconds, groups of 2-5 swarmers from random arena edges
- Unit tests for enemy module (`tests/test_enemy.c`) -- 30 tests covering pool, spawning, movement, collision, constants
- Vec2 2D vector math module (`src/vec2.h`, `src/vec2.c`) with add, subtract, scale, length, normalize, distance, dot product, lerp
- 28 unit tests for vec2 (`tests/test_vec2.c`) covering all operations including edge cases
- `task test` command in Taskfile.yml -- single command to build and run all tests
- Development Lifecycle in AGENTS.md -- TDD workflow: plan, simplify, test, implement, green, simplify
- Stronger test mandate: all code changes require tests, not just gameplay systems
- Self-Maintenance section in AGENTS.md -- explicit triggers for keeping docs up to date
- Documentation convention: all structs, public functions, and modules require header comments
- First playable prototype: player movement (WASD, tank controls), mouse aiming, shooting (left-click, Basic Pistol), dash (spacebar with cooldown)
- Arena with boundary clamping -- player and bullets stay within the play area
- HUD: health bar and dash cooldown indicator
- New source modules: `game.c/h`, `player.c/h`, `bullet.c/h`
- Unit tests for player, bullet, and game modules (`tests/test_player.c`, `tests/test_bullet.c`, `tests/test_game.c`)
- Static library target `astro_blitz_lib` for sharing game logic between exe and tests
- Linting and formatting: `task fmt` (changed file types) and `task fmt:all` (all files) with clang-format for C, yamllint for YAML, markdownlint-cli2 for Markdown
- Linter configs: `.clang-format`, `.yamllint.yml`, `.markdownlint-cli2.yaml`
- Installer tasks: `task install-yamllint`, `task install-markdownlint`
- Unity test framework v2.6.1 with CTest integration and sample test (`tests/test_sample.c`)
- Task runner configuration (`Taskfile.yml`) with `task configure`, `task build`, `task run`, and `task clean`
- Raylib 5.5 build tooling via CMake FetchContent -- `cmake -B build` fetches and builds Raylib automatically
- Hello window: `src/main.c` opens an 800x600 window with "ASTRO BLITZ" title text
- Initial project structure: `AGENTS.md`, `STATUS.md`, `CHANGELOG.md`
- Game design document: `design/DESIGN.md`
- Design assets directory: `design/assets/`
- OpenCode configuration: `opencode.jsonc`
- Full game design: core loop, weapons (6 ranged + 5 melee), enemies (9 types), upgrades (16 passives), visual style, audio direction
- Tech decision: Raylib 5.5 with C99
