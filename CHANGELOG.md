# Changelog

All notable changes to Astro Blitz are documented here.
Format based on [Keep a Changelog](https://keepachangelog.com/).

## [Unreleased]

### Added

- Game-over state: when player HP reaches zero, gameplay freezes and a "GAME OVER" overlay displays run stats (kills, survival time as MM:SS, waves spawned) with a prompt to press R to restart
- `GamePhase` enum (`PHASE_PLAYING`, `PHASE_GAME_OVER`) and `GameStats` struct for tracking per-run statistics
- Kill counter increments when enemies are destroyed by bullets
- Survival time accumulates each frame during gameplay
- Wave counter increments each time a new enemy wave spawns
- 6 new unit tests for game-over phase transitions, stats initialization, and death check
- Enemy swarmer system: `src/enemy.c/h` with fixed-size enemy pool, swarmer AI (seek toward player), wave spawning from arena edges

### Changed

- Player visual upgraded from plain green circle to layered sci-fi mech: dark teal body with neon cyan outline, directional nose triangle for clear facing, bright reactor core, hot magenta aim line with crosshair dot, and semi-transparent glow aura; electric blue palette shift when dashing
- Swarmer visual upgraded from plain red circle to layered alien bug: dark maroon body with neon red-orange outline, mandible "V" lines and bright yellow eye dots that face movement direction, and semi-transparent red glow aura
- Player movement now uses tank controls: W moves toward mouse cursor, S moves away, A/D strafe left/right relative to aim direction
- Dash direction also follows the new tank controls when WASD keys are held
- Bullet-enemy and enemy-player collision detection with circle-circle overlap
- Player takes damage on enemy contact (swarmers die on contact); dash invincibility respected
- Enemy spawn waves every 3 seconds, groups of 4-8 swarmers from random arena edges
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
