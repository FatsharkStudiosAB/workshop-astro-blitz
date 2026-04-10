# Game Design: Astro Blitz

## Concept

Top-down sci-fi roguelike shooter. The player navigates procedurally generated levels, fighting waves of enemies with upgradeable weapons and abilities. Die or win -- permadeath with meta-progression between runs. **No extraction mechanic.**

## Genre & Inspirations

- **Genre:** Top-down roguelike shooter
- **Setting:** Sci-fi / space station & alien worlds
- **Inspirations:**
  - **Risk of Rain** -- stacking temporary upgrades, escalating difficulty, run-based progression
  - **Nethack** -- permadeath consequences, emergent gameplay from item/enemy interactions
  - **Darktide** -- enemy variety and behavior (hordes, specials, elites), visceral combat feel
  - **Hotline Miami** -- fast top-down combat, instant lethality, tight controls, retro aesthetic

## Core Loop

1. **Enter level** -- procedurally generated map with rooms, corridors, and arenas
2. **Fight** -- shoot and slash through enemies, dodge incoming fire and melee attacks
3. **Loot** -- pick up weapon drops and passive upgrades from enemies, chests, and hidden rooms
4. **Escalate** -- difficulty ramps as the player progresses; new enemy types, denser spawns, tougher elites
5. **Die or win** -- permadeath. A run ends when the player dies or clears the final level. Meta-progression unlocks new starting options for future runs.

### Meta-Progression

Between runs, the player unlocks:
- New starting weapon variants (still start with one ranged + one melee)
- New passive upgrades added to the loot pool
- Cosmetic rewards

Meta-progression does **not** carry over power -- each run starts from scratch.

## Player

- **Movement:** 8-directional (WASD)
- **Aiming:** Mouse-aimed (cursor determines facing and shot direction)
- **Actions:**
  - **Shoot** (left click) -- fire equipped ranged weapon
  - **Melee** (right click) -- swing equipped melee weapon
  - **Dash/dodge** (spacebar) -- short invincible dash on a cooldown
  - **Interact** (E) -- pick up weapons/items, open chests, use doors
  - **Swap weapon** (Q) -- quick-swap between ranged and melee (or handled implicitly via left/right click)

## Weapons

### Loadout

The player carries exactly **two weapons** at all times:
- **One ranged slot**
- **One melee slot**

Starting loadout: **Basic Pistol** + **Combat Knife**.

### Acquisition

Weapons drop from enemies, spawn in chests, or appear as world pickups. Picking up a weapon of the same slot type **replaces** the current weapon -- there is no persistent inventory. The dropped weapon appears on the ground for a short time in case the player wants to swap back.

### Ranged Weapons

| Weapon | Fire Rate | Damage | Notes |
|--------|-----------|--------|-------|
| Basic Pistol | Medium | Low | Starting weapon. Reliable, no spin-up. |
| SMG | High | Low | Fast fire rate, wider spread. |
| Shotgun | Low | High (burst) | Multiple pellets, devastating at close range, slow reload. |
| Plasma Rifle | Medium | Medium-High | Projectiles pierce one enemy. |
| Railgun | Very Low | Very High | Hitscan line through all enemies. Long charge-up. |
| Grenade Launcher | Low | High (AoE) | Arcing projectile, explodes on impact. |

### Melee Weapons

| Weapon | Speed | Damage | Notes |
|--------|-------|--------|-------|
| Combat Knife | Fast | Low | Starting weapon. Quick stabs, short range. |
| Energy Sword | Medium | Medium | Wide arc, good crowd control. |
| Battle Axe | Slow | High | Overhead swing, cleaves through multiple enemies. |
| Stun Baton | Fast | Low | Stuns enemies briefly on hit. |
| War Hammer | Very Slow | Very High | Massive knockback, small AoE on impact. |

## Enemies

Enemies come in distinct behavioral archetypes. Each level mixes these types to create varied combat encounters.

### Melee Enemies

| Enemy | Speed | HP | Behavior |
|-------|-------|----|----------|
| **Swarmer** | Fast | Very Low | Rushes the player in packs. Individually weak, dangerous in numbers. Spawns in groups of 4-8. Think Poxwalkers from Darktide. |
| **Stalker** | Medium | Low | Flanker. Tries to circle behind the player before attacking. Moves erratically to dodge shots. Nethack-style ambush predator. |
| **Heavy** | Slow | High | Armored brute. Walks toward the player steadily. Has a charge attack that deals massive damage. Telegraphed by a brief wind-up. |
| **Bomber** | Fast | Low | Suicide unit. Sprints at the player and explodes on contact or death. Explosion damages nearby enemies too. Chain reactions possible. |

### Ranged Enemies

| Enemy | Speed | HP | Behavior |
|-------|-------|----|----------|
| **Grunt** | Slow | Medium | Basic ranged attacker. Fires at the player while slowly advancing. Predictable shot patterns. Bread-and-butter enemy. |
| **Sniper** | Stationary | Low | Positions at long range. Telegraphs shots with a visible laser sight for ~1 second before firing a high-damage hitscan shot. Relocates after firing. |
| **Suppressor** | Slow | Medium | Fires rapid bursts that create "danger zones" the player must avoid. Forces movement, doesn't deal huge per-hit damage. Area denial. |

### Special Enemies

| Enemy | Speed | HP | Behavior |
|-------|-------|----|----------|
| **Shielder** | Slow | Medium | Carries a frontal energy shield that blocks projectiles. Must be flanked or hit with melee. Can shield nearby allies. |
| **Spawner** | Stationary | High | Vent or nest that periodically produces Swarmers. Must be destroyed to stop reinforcements. |

## Upgrades (Passives)

Upgrades are **temporary** -- they last for the current run only. Found as pickups in the world (dropped by enemies, in chests, in secret rooms). The player can stack multiple upgrades. Upgrades are **not** a choice from a menu -- you find them and they activate immediately.

### Stat Boosts

| Upgrade | Effect |
|---------|--------|
| Reinforced Plating | +Max HP |
| Adrenaline Shot | +Movement speed |
| Quick Loader | +Reload speed |
| Targeting Module | +Ranged damage |
| Sharpened Edge | +Melee damage |
| Reflex Enhancer | -Dash cooldown |

### On-Hit Effects

| Upgrade | Effect |
|---------|--------|
| Incendiary Rounds | Ranged hits ignite enemies (damage over time) |
| Bleed Edge | Melee hits cause bleed (damage over time) |
| Chain Lightning | Hits have a chance to arc to a nearby enemy |
| Cryo Coating | Hits slow enemies briefly |
| Explosive Tips | Kills trigger a small explosion |

### Defensive / Utility

| Upgrade | Effect |
|---------|--------|
| Vampire Fang | Melee kills restore a small amount of HP |
| Thorn Aura | Enemies that hit the player take damage back |
| Shield Capacitor | Killing an enemy grants a temporary shield |
| Scavenger Magnet | Increased pickup radius |
| Second Wind | Survive a killing blow once per level (1 HP) |

Upgrades are designed to **stack and synergize**. Example: Incendiary Rounds + Explosive Tips = kills from burn damage trigger explosions. This creates the Risk of Rain "build discovery" feeling.

## Visual Style

**Stylized pixel-vector arcade retro.**

- Pixel art sprites with clean outlines and limited but vibrant color palettes
- Inspired by Hotline Miami's neon-drenched top-down aesthetic
- High-contrast visual language: player, enemies, projectiles, and pickups must read instantly
- Screen shake, muzzle flash, and hit sparks for satisfying combat feedback
- Dark backgrounds (space station interiors, alien hive corridors) with bright entity sprites
- Minimal UI -- see the **UI & Feedback** section for full details

Reference images and concept art go in `design/assets/`.

## UI & Feedback

The UI follows the same "minimal but readable" philosophy as the visual style. Every element must justify its screen space -- if it doesn't help the player make a split-second decision, it doesn't belong on the HUD.

### HUD

Always visible during gameplay. Positioned at screen edges to keep the play area uncluttered.

| Element | Position | Description |
|---------|----------|-------------|
| **Health bar** | Bottom-left | Horizontal bar. Color shifts green -> yellow -> red as HP drops. Flashes when critically low. |
| **Dash cooldown** | Below health bar | Small circular or bar indicator. Fills up as cooldown recharges. Bright flash when ready. |
| **Current weapons** | Bottom-right | Icons for ranged and melee slots. Active weapon highlighted. Ammo count (if applicable) next to ranged weapon. |
| **Active upgrades** | Top-left | Small icon strip showing collected passive upgrades for the current run. Tooltip on pause. |
| **Level / floor indicator** | Top-center | Current floor number and level name (e.g. "Floor 3 -- Reactor Core"). |

### Minimap

| Aspect | Design |
|--------|--------|
| **Position** | Top-right corner |
| **Size** | Small (~15% of screen width), semi-transparent background |
| **Content** | Explored rooms shown as filled shapes, unexplored rooms as outlines (if adjacent/detected). Player as a bright dot. Exit doors marked. |
| **Toggle** | Tab key to expand to a full overlay map. Press again (or Tab) to dismiss. |
| **Fog of war** | Rooms only appear on the minimap once the player enters them. Adjacent rooms show as dim outlines. |

### Stats Legend

Displayed as a persistent overlay in the top-right area (below the minimap) or toggled via a hotkey.

| Stat | Description |
|------|-------------|
| **Kills** | Total enemies killed this run |
| **Score** | Running score based on kills, speed, and style (e.g. multi-kills, no-damage clears) |
| **Time** | Elapsed time for the current run |
| **Floor** | Current floor / total floors |

A full **run summary screen** appears on death or victory, showing final stats: kills, score, time, upgrades collected, weapons used, and cause of death (if applicable).

### Gameplay Feedback

Visual and audio cues that communicate game state changes to the player without requiring them to read UI elements.

| Event | Feedback |
|-------|----------|
| **Player hit** | Screen flash (brief red vignette), screen shake, damage number, hurt SFX |
| **Enemy hit** | Hit spark particle, damage number floats up from enemy, hit SFX |
| **Enemy kill** | Death animation, XP/score pop-up, distinct kill SFX |
| **Critical / multi-kill** | Larger damage numbers, intensified screen shake, combo text (e.g. "DOUBLE KILL") |
| **Upgrade pickup** | Icon briefly enlarges on HUD, short chime, text flash naming the upgrade |
| **Weapon swap** | New weapon icon slides into the HUD slot, equip SFX |
| **Dash used** | Motion trail behind player, whoosh SFX, cooldown indicator starts draining |
| **Dash ready** | Cooldown indicator flashes, subtle ready chime |
| **Low HP** | Health bar pulses, heartbeat SFX, slight red tint on screen edges |
| **Level clear** | Fanfare SFX, "CLEARED" text, brief stat summary before proceeding |
| **Player death** | Slow-motion death frame, fade to run summary screen |

### Design Principles

- **Readability over decoration.** Every UI element uses high-contrast colors that stand out against any background.
- **No menu-driven gameplay.** Upgrades activate on pickup, weapons swap on interact -- the HUD reflects state, it doesn't gate actions.
- **Feedback layering.** Important events combine visual + audio + HUD cues. Less important events use only one channel.
- **Scale with intensity.** Screen shake, particle density, and combo text scale with the action -- a single kill is subtle, a chain explosion is loud.

## Audio

- **Music:** Retro synth / chiptune hybrid. Pulsing, driving tracks that escalate with gameplay intensity.
- **SFX:** Punchy, arcade-style. Distinct sounds for each weapon type, enemy death, pickup collection, dash.
- **Priority:** Gameplay-critical audio (enemy telegraph sounds, low HP warning) over ambient.

## Technical Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Engine/Framework | **Raylib 5.5** | Zero external dependencies. Compiles to a small native .exe via any C compiler. Minimal API (~120 functions), huge example library, excellent AI coding assistant support. |
| Language | **C** (C99) | Raylib's native language. Simple, fast compilation, no runtime. Matches workshop "get things working fast" philosophy. |
| Build system | **CMake** or direct compiler invocation | Raylib supports both. Start with direct `gcc`/MSVC invocation for simplicity, move to CMake if needed. |
| Asset format | PNG (sprites), WAV/OGG (audio) | Natively supported by raylib with no additional libraries. |
| Level generation | Code-driven procedural generation | No external editor dependency. Generate rooms + corridors from templates/algorithms at runtime. |

### Why Raylib Over Alternatives

- **vs Love2D:** Raylib produces a true native binary (~1-5 MB) with no embedded VM. Love2D fuses a LuaJIT runtime (~30 MB). Both are good; Raylib gives more control and smaller output.
- **vs Godot 4:** Godot's editor is powerful but heavyweight (~100 MB editor + ~500 MB export templates). For a code-focused workshop, Raylib's "no editor, just code" approach has less friction.
- **vs Macroquad (Rust):** Rust's learning curve and compile times hurt workshop pacing. Raylib's C API is simpler to iterate on.
- **vs SDL2:** SDL2 requires multiple satellite libraries (SDL_image, SDL_mixer, SDL_ttf). Raylib bundles everything with zero deps.

## Open Questions

- Level structure: linear sequence of floors, or branching paths with choices?
- Boss encounters: one per floor, or only at key milestones?
- Meta-progression specifics: what exactly unlocks between runs?
- Multiplayer: single-player only, or co-op as a stretch goal?
- How many levels/floors constitute a "full run"?
