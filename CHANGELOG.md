# Changelog

All notable changes to Astro Blitz are documented here.
Format based on [Keep a Changelog](https://keepachangelog.com/).

## [Unreleased]

### Added

- `task test` command in Taskfile.yml -- single command to build and run all tests
- Development Lifecycle in AGENTS.md -- TDD workflow: plan, test, implement, green, simplify
- Stronger test mandate: all code changes require tests, not just gameplay systems
- Self-Maintenance section in AGENTS.md -- explicit triggers for keeping docs up to date
- Documentation convention: all structs, public functions, and modules require header comments
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
