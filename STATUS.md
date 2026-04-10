# Status

Persistent memory for the Astro Blitz project. Keep this file under ~150 lines.
When it grows too long, summarize older entries and remove resolved items.

## Current State

- **Phase:** Pre-development -- build system and test framework in place, placeholder game window.
- **Engine/Framework:** Raylib 5.5 (C99).
- **Build system:** CMake with FetchContent (Raylib + Unity test framework).
- **Playable:** No (placeholder "Coming Soon" window).

## Recent Changes

| Date | Change |
|------|--------|
| 2026-04-10 | Added CMake build system, Unity test framework, placeholder game window, sample test |
| 2026-04-10 | Completed game design document: core loop, weapons, enemies, upgrades, visual style, tech choice (Raylib) |
| 2026-04-10 | Repository created with initial structure (AGENTS.md, STATUS.md, CHANGELOG.md, design/) |

## Known Issues / Next Steps

- ~~Set up Raylib build tooling~~ (done -- CMake + FetchContent, hello window compiles)
- Create first playable prototype: player movement + shooting in an empty arena
- Implement basic enemy spawning (Swarmers first -- simplest behavior)
- Decide on level structure (linear floors vs branching paths)
- Source or create placeholder sprite assets

## Workarounds & Patterns

*(None yet -- document anything that fails before succeeding so future sessions don't repeat it.)*
