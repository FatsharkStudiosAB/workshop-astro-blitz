# Status

Persistent memory for the Astro Blitz project. Keep this file under ~150 lines.
When it grows too long, summarize older entries and remove resolved items.

## Current State

- **Phase:** Scrollable world prototype with audio -- camera follows player over a procedurally generated tilemap with obstacles, procedural audio for SFX and death screen music.
- **Engine/Framework:** Raylib 5.5 (C99), built via CMake FetchContent.
- **Build:** `task build` (or `cmake -B build && cmake --build build --config Release`)
- **Test framework:** Unity (ThrowTheSwitch) v2.6.1 via FetchContent + CTest.
- **Playable:** Yes (player moves in a large world [4096x3072 px], camera follows, tilemap with scattered obstacles and border walls, enemies spawn offscreen relative to viewport). Bullet fire plays a punchy SFX; enemy hits play a metallic ping; taking damage plays a low thud. Game ends when HP reaches zero; game-over screen shows stats, plays looping sci-fi synth music, and pressing R restarts with a new map.

## Recent Changes

| Date | Change |
|------|--------|
| 2026-04-10 | Added BFS flow field pathfinding for enemies: enemies navigate around obstacles via shortest path. Flow field computed each frame from player position. Enemies use flow field at long range, direct-seek at close range (<3 tiles). 8 new tilemap tests, 3 new enemy tests. |
| 2026-04-10 | Added audio system: procedural bullet SFX, enemy-hit SFX, bullet-hit-enemy SFX, and death screen music (sci-fi synth with detuned oscillators, drone bass, tritone melody) |
| 2026-04-10 | PR #16 review fixes: spawn_wave() uses camera state for viewport calc, clamps to walkable interior (TILE_SIZE inset), retries on solid tiles; update_camera() accounts for zoom; tilemap PRNG replaced with local xorshift32 (no global srand/rand coupling); tests derive arena/screen constants from headers. |
| 2026-04-10 | Added Camera2D, tilemap module, procedural generation, and scrollable world (128x96 tile grid, ~4096x3072 px). Player, enemies, and bullets now operate in world coordinates. Camera follows player with edge clamping. Obstacles block movement and bullets. Enemies spawn offscreen relative to camera. 27 new tilemap tests, updated game/bullet/enemy tests. |
| 2026-04-10 | Upgraded player and swarmer visuals: layered neon sci-fi art with glow, outlines, facing indicators |
| 2026-04-10 | Added game-over state: death screen with stats (kills, time, waves), restart with R key |
| 2026-04-10 | Fixed .clang-format config (`Language: C` -> `Language: Cpp` for clang-format 18 compatibility) |
| 2026-04-10 | Changed player movement from screen-relative 8-directional to tank controls (WASD relative to aim direction) |
| 2026-04-10 | Added enemy swarmer system: enemy pool, swarmer AI (seek player), wave spawning, bullet-enemy and enemy-player collisions, 30 tests |
| 2026-04-10 | Added vec2 math module (src/vec2.h, src/vec2.c) with 28 tests; added to astro_blitz_lib |
| 2026-04-10 | Added `task test` command; strengthened AGENTS.md with TDD lifecycle, test mandate, self-maintenance |
| 2026-04-10 | Added unit tests for player, bullet, and game modules (49 tests across 4 suites, all passing) |
| 2026-04-10 | First playable: player movement (WASD), mouse aiming, shooting (left-click), dash (spacebar), arena bounds, HUD |
| 2026-04-10 | Added linting/formatting: `task fmt` with clang-format, yamllint, markdownlint-cli2 |
| 2026-04-10 | Added Unity test framework v2.6.1 via FetchContent, sample test, CTest integration |
| 2026-04-10 | Added Taskfile.yml with configure, build, run, and clean tasks (go-task) |
| 2026-04-10 | Set up Raylib 5.5 build tooling: CMakeLists.txt with FetchContent, hello window compiles and runs |
| 2026-04-10 | Completed game design document: core loop, weapons, enemies, upgrades, visual style, tech choice (Raylib) |
| 2026-04-10 | Repository created with initial structure (AGENTS.md, STATUS.md, CHANGELOG.md, design/) |

## Known Issues / Next Steps

- Implement basic enemy spawning (Swarmers first -- simplest behavior) -- done
- Add bullet-enemy collision -- done
- Add game-over state when HP reaches zero -- done
- Add Camera2D and scrollable world -- done
- Add procedural tilemap with obstacles -- done
- Add melee attack (right-click)
- Room-based level generation (corridors, doors) -- currently open-world with scattered obstacles
- Minimap overlay (design doc calls for top-right corner)
- Decide on level structure (linear floors vs branching paths)
- Source or create placeholder sprite assets -- partially addressed with layered geometric art

## Workarounds & Patterns

- **go-task preconditions on Windows:** `preconditions` in Taskfile.yml use `sh:` which runs through a POSIX shell. On Windows, go-task uses Git Bash (`sh.exe`) if available. Ensure Git is installed so `test -f` works in preconditions.
- **clang-format `Language` field:** clang-format 18 does not recognize `Language: C`. Use `Language: Cpp` instead -- clang-format treats C/C++/ObjC as the same language kind (`Cpp`).
- **CMake multi-config vs single-config:** Pass `-DCMAKE_BUILD_TYPE` at configure time (for single-config generators like Ninja/Makefiles) AND `--config` at build time (for multi-config generators like Visual Studio). Both are harmless when the other generator type is used.
- **Taskfile `run` task:** Uses platform-specific commands to find the executable in both `build/<Config>/` (multi-config) and `build/` (single-config) layouts.
- **FetchContent + GIT_SHALLOW + commit hash does not work.** `GIT_SHALLOW TRUE` only supports branch/tag names, not commit hashes. Use `URL` + `URL_HASH` with a release tarball instead for pinned, reproducible builds.
- **Raylib `option()` clears normal variables (CMP0077).** Set `BUILD_EXAMPLES` and `BUILD_GAMES` as `CACHE BOOL` (without `FORCE`) so raylib's `option()` doesn't override them.
- **astro_blitz_lib pattern:** Game logic goes into `astro_blitz_lib` static library (links Raylib publicly). Tests link against `astro_blitz_lib` + Unity. The main executable links `astro_blitz_lib`. This shares game logic between exe and test suites.
