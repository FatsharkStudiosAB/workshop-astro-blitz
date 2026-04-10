# Changelog

All notable changes to Astro Blitz are documented here.
Format based on [Keep a Changelog](https://keepachangelog.com/).

## [Unreleased]

### Added

- CMake build system with FetchContent for Raylib 5.5 and Unity test framework v2.6.1
- Placeholder game window (`src/main.c`) -- 800x600, "Astro Blitz - Coming Soon"
- Sample unit test (`tests/test_sample.c`) verifying the framework works
- CTest integration: `ctest --test-dir build -C Debug --output-on-failure`
- Initial project structure: `AGENTS.md`, `STATUS.md`, `CHANGELOG.md`
- Game design document: `design/DESIGN.md`
- Design assets directory: `design/assets/`
- OpenCode configuration: `opencode.jsonc`
- Full game design: core loop, weapons (6 ranged + 5 melee), enemies (9 types), upgrades (16 passives), visual style, audio direction
- Tech decision: Raylib 5.5 with C99
