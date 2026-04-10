# Changelog

All notable changes to Astro Blitz are documented here.
Format based on [Keep a Changelog](https://keepachangelog.com/).

## [Unreleased]

### Added

- Raylib 5.5 build tooling via CMake FetchContent -- `cmake -B build` fetches and builds Raylib automatically
- Hello window: `src/main.c` opens an 800x600 window with "ASTRO BLITZ" title text
- Initial project structure: `AGENTS.md`, `STATUS.md`, `CHANGELOG.md`
- Game design document: `design/DESIGN.md`
- Design assets directory: `design/assets/`
- OpenCode configuration: `opencode.jsonc`
- Full game design: core loop, weapons (6 ranged + 5 melee), enemies (9 types), upgrades (16 passives), visual style, audio direction
- Tech decision: Raylib 5.5 with C99
