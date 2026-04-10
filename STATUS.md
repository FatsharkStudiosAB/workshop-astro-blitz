# Status

Persistent memory for the Astro Blitz project. Keep this file under ~150 lines.
When it grows too long, summarize older entries and remove resolved items.

## Current State

- **Phase:** Early development -- build tooling and test framework set up, hello window running.
- **Engine/Framework:** Raylib 5.5 (C99), built via CMake FetchContent.
- **Build:** `cmake -B build -G "Visual Studio 17 2022" -A x64` then `cmake --build build --config Release`
- **Test framework:** Unity (ThrowTheSwitch) v2.6.1 via FetchContent + CTest.
- **Playable:** No (window opens with title text only).

## Recent Changes

| Date | Change |
|------|--------|
| 2026-04-10 | Added vec2 math module (src/vec2.h, src/vec2.c) with 27 tests; introduced game_lib static library pattern |
| 2026-04-10 | Added `task test` command; strengthened AGENTS.md with TDD lifecycle, test mandate, self-maintenance |
| 2026-04-10 | Added linting/formatting: `task fmt` with clang-format, yamllint, markdownlint-cli2 |
| 2026-04-10 | Added Unity test framework v2.6.1 via FetchContent, sample test, CTest integration |
| 2026-04-10 | Added Taskfile.yml with configure, build, run, and clean tasks (go-task) |
| 2026-04-10 | Set up Raylib 5.5 build tooling: CMakeLists.txt with FetchContent, hello window compiles and runs |
| 2026-04-10 | Completed game design document: core loop, weapons, enemies, upgrades, visual style, tech choice (Raylib) |
| 2026-04-10 | Repository created with initial structure (AGENTS.md, STATUS.md, CHANGELOG.md, design/) |

## Known Issues / Next Steps

- Create first playable prototype: player movement + shooting in an empty arena
- Implement basic enemy spawning (Swarmers first -- simplest behavior)
- Decide on level structure (linear floors vs branching paths)
- Source or create placeholder sprite assets

## Workarounds & Patterns

- **go-task preconditions on Windows:** `preconditions` in Taskfile.yml use `sh:` which runs through a POSIX shell. On Windows, go-task uses Git Bash (`sh.exe`) if available. Ensure Git is installed so `test -f` works in preconditions.
- **CMake multi-config vs single-config:** Pass `-DCMAKE_BUILD_TYPE` at configure time (for single-config generators like Ninja/Makefiles) AND `--config` at build time (for multi-config generators like Visual Studio). Both are harmless when the other generator type is used.
- **Taskfile `run` task:** Uses platform-specific commands to find the executable in both `build/<Config>/` (multi-config) and `build/` (single-config) layouts.
- **FetchContent + GIT_SHALLOW + commit hash does not work.** `GIT_SHALLOW TRUE` only supports branch/tag names, not commit hashes. Use `URL` + `URL_HASH` with a release tarball instead for pinned, reproducible builds.
- **Raylib `option()` clears normal variables (CMP0077).** Set `BUILD_EXAMPLES` and `BUILD_GAMES` as `CACHE BOOL` (without `FORCE`) so raylib's `option()` doesn't override them.
- **game_lib pattern:** Game logic goes into a `game_lib` static library (no Raylib dependency). Tests link against `game_lib` + Unity. The main executable links `game_lib` + Raylib. This keeps game logic testable without window/rendering dependencies.
