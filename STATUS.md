# Status

Persistent memory for the Astro Blitz project. Keep this file under ~150 lines.
When it grows too long, summarize older entries and remove resolved items.

## Current State

- **Phase:** Pre-development -- design document complete, no game code yet.
- **Engine/Framework:** Raylib 5.5 (C99).
- **Playable:** No.

## Recent Changes

| Date | Change |
|------|--------|
| 2026-04-10 | Added Taskfile.yml with configure, build, run, and clean tasks (go-task) |
| 2026-04-10 | Completed game design document: core loop, weapons, enemies, upgrades, visual style, tech choice (Raylib) |
| 2026-04-10 | Repository created with initial structure (AGENTS.md, STATUS.md, CHANGELOG.md, design/) |

## Known Issues / Next Steps

- Set up Raylib build tooling (download Raylib, configure compiler, verify "hello window" compiles)
- Create first playable prototype: player movement + shooting in an empty arena
- Implement basic enemy spawning (Swarmers first -- simplest behavior)
- Decide on level structure (linear floors vs branching paths)
- Source or create placeholder sprite assets

## Workarounds & Patterns

- **go-task preconditions on Windows:** `preconditions` in Taskfile.yml use `sh:` which runs through a POSIX shell. On Windows, go-task uses Git Bash (`sh.exe`) if available. Ensure Git is installed so `test -f` works in preconditions.
- **CMake multi-config vs single-config:** Pass `-DCMAKE_BUILD_TYPE` at configure time (for single-config generators like Ninja/Makefiles) AND `--config` at build time (for multi-config generators like Visual Studio). Both are harmless when the other generator type is used.
- **Taskfile `run` task:** Uses platform-specific commands to find the executable in both `build/<Config>/` (multi-config) and `build/` (single-config) layouts.
