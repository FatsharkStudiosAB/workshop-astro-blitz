# Game Design: Astro Blitz

## Concept

Top-down sci-fi roguelike shooter. The player navigates procedurally
generated floors of arena-style combat rooms, fighting waves of enemies
with modifiable weapons and stackable passive upgrades. Permadeath with
meta-progression between runs. **No extraction mechanic.**

## Genre & Inspirations

- **Genre:** Top-down roguelike shooter (hybrid arena-dungeon)
- **Setting:** Sci-fi / space station & alien worlds
- **Inspirations:**
  - **Risk of Rain** -- stacking temporary upgrades, steep difficulty
    scaling, all items available from the start (rarer ones just less
    likely early), build-defining synergies
  - **Binding of Isaac** -- room-based procedural layouts where combat
    rooms lock until cleared, loot rooms reward exploration, bosses gate
    floor progression
  - **Diablo / Borderlands** -- prefix + suffix weapon modifier system,
    procedurally generated loot with combinatorial variety
  - **Nethack** -- permadeath consequences, emergent gameplay from
    item/enemy interactions
  - **Darktide** -- enemy variety and behavior (hordes, specials,
    elites), visceral combat feel
  - **Hotline Miami** -- fast top-down combat, tight controls, retro
    neon aesthetic

## Core Loop

1. **Enter floor** -- procedurally generated map with combat rooms,
   corridors, loot rooms, and an exit
2. **Explore** -- move through rooms. Combat rooms lock on entry and
   unlock when cleared. Loot rooms and corridors remain open.
3. **Fight** -- shoot and slash through enemies, dodge incoming fire and
   melee attacks. Enemies may be elite variants with random modifiers.
4. **Loot** -- pick up weapon drops (with random prefix/suffix
   modifiers) and passive upgrades from enemies, chests, and secret
   rooms
5. **Find the exit** -- locate and reach the floor exit to descend. The
   player chooses how much to explore before leaving.
6. **Escalate** -- difficulty ramps steeply as floors progress: stat
   scaling, new enemy abilities, higher elite spawn rates
7. **Die or win** -- permadeath. A run ends when the player dies or
   clears the final floor. Meta-progression unlocks new starting
   options for future runs.

## Run Structure

- **Floor count:** 5--7 floors per full run (~20--30 minutes)
- **Floor completion:** Find and reach the exit. Exploration is
  optional but rewarded with loot.
- **Boss encounters:** Every 2--3 floors (2--3 bosses per run). The
  final floor always ends with a boss. Mid-run bosses are elite
  variants of existing archetypes with enhanced stats and one unique
  mechanic. The final boss is a handcrafted unique encounter with
  multi-phase behavior.
- **First boss guaranteed melee drop:** The first boss always drops a
  melee weapon, ensuring every player has access to melee by mid-run.

## Difficulty Curve

**Steep ramp** -- inspired by Risk of Rain. Without upgrades, later
floors become very hard. Good play and strong build synergies are
rewarded.

### Scaling per floor

Unless otherwise noted, stats scale **linearly from floor-1 base
values** (not compounded). General formula:
`scaled = base * (1 + rate * (floor - 1))`.

| Stat | Per-floor increase | Formula | Example (floor 1 -> floor 5) |
|------|--------------------|---------|-------------------------------|
| Enemy HP | +20% linear | `base_hp * (1 + 0.20 * (floor - 1))` | Swarmer: 10 -> 18 HP |
| Enemy speed | +8% linear | `base_speed * (1 + 0.08 * (floor - 1))` | Swarmer: 150 -> 198 px/s |
| Wave interval | x0.85 per floor | `base_interval * 0.85 ^ (floor - 1)` | 3.0s -> 1.6s |
| Spawn group size | +1 per 2 floors | `base + floor_div(floor - 1, 2)` | 2--5 -> 4--7 |
| Elite spawn chance | +10 pct points per floor (additive, capped at 100%) | `min(base + 10 * (floor - 1), 100)` | 5% -> 45% |

Note: wave interval uses multiplicative decay (x0.85 per floor)
rather than linear scaling because linear subtraction would reach zero
at floor 7+.

### Enemy ability progression

Enemies gain new abilities on later floors (in addition to stat
buffs):

| Floor | New enemy abilities |
|-------|---------------------|
| 1--2 | Base behaviors only |
| 3 | Swarmers split into 2 mini-swarmers on death |
| 4 | Grunts fire 3-round bursts instead of single shots |
| 5+ | Heavies gain a ground-slam AoE, Bombers leave lingering fire |

Elite enemies (see [Procedural Enemies](#procedural-enemies)) spawn at
increasing rates and always have 1--2 random modifiers, adding
unpredictable variety on top of the floor-based progression.

## Player

- **Movement:** Configurable -- Tank controls (WASD relative to aim
  direction) or 8-directional (WASD = screen-relative). Selectable in
  the settings menu. Persisted to `settings.ini`.
- **Aiming:** Mouse-aimed (cursor determines facing and shot direction)
- **Actions:**
  - **Shoot** (left click) -- fire equipped ranged weapon
  - **Melee** (right click) -- swing equipped melee weapon (if one is
    equipped)
  - **Dash/dodge** (spacebar) -- short invincible dash on a cooldown
  - **Interact** (E) -- pick up weapons/items, open chests, use doors
- **Starting loadout:** Basic Pistol only (no melee). Melee weapons are
  found as drops during the run.

### Player Stats (Base)

| Stat | Value |
|------|-------|
| Max HP | 100 |
| Movement speed | 200 px/s |
| Dash speed | 600 px/s |
| Dash duration | 0.15s |
| Dash cooldown | 1.0s |
| Player radius | 12 px |

## Weapons

### Loadout

The player carries up to **two weapons**:

- **One ranged slot** (always filled -- start with Basic Pistol)
- **One melee slot** (empty at start -- filled by finding a melee
  weapon)

Picking up a weapon of the same slot type **replaces** the current
weapon. The dropped weapon appears on the ground briefly in case the
player wants to swap back.

### Weapon Modifiers (Prefix + Suffix)

Every weapon drop independently rolls 0--2 modifiers from a single
shared pool. The number of modifiers is random and independent of weapon
rarity tier:

- **0 modifiers** -- plain weapon (more common on early floors)
- **1 modifier** -- prefix or suffix
- **2 modifiers** -- prefix + suffix (jackpot -- rare early, more
  common later)

Modifier roll probability shifts with floor progression: later floors
weight toward more modifiers and stronger modifier types.

#### Modifier Pool

| Modifier | Type | Effect |
|----------|------|--------|
| **Rapid** | Offensive | +25% fire rate (percentage) |
| **Heavy** | Offensive | +40% damage (percentage), -20% fire rate (percentage) |
| **Scorching** | Offensive | Hits ignite enemies (damage over time) |
| **Piercing** | Offensive | Projectiles pass through 1 extra enemy |
| **Volatile** | Offensive | Kills trigger a small AoE explosion |
| **Chilling** | Utility | Hits slow enemies by 30% for 1.5s |
| **Bouncing** | Utility | +2 extra wall bounces |
| **Vampiric** | Utility | Kills heal 2 HP |
| **Seeking** | Utility | Slight homing on projectiles |
| **Energized** | Utility | Kills reduce dash cooldown by 0.3s |

Modifiers stack with passive upgrades additively. Example: a Scorching
SMG + Incendiary Rounds passive = longer burn duration, not double
proc.

### Weapon Rarity

Weapons have three rarity tiers affecting base stat quality. All tiers
are available from floor 1 -- rarer tiers just drop less frequently.
Later floors increase the probability of higher-rarity drops.

| Rarity | Drop weight (floor 1) | Drop weight (floor 5) | Stat bonus |
|--------|----------------------|----------------------|------------|
| **Common** | 70% | 40% | Base stats |
| **Uncommon** | 25% | 40% | +15% to damage and fire rate |
| **Rare** | 5% | 20% | +30% to damage and fire rate |

Rarity bonuses apply to **damage** and **fire rate** only. Projectile
speed, arc, and special properties (pierce, AoE radius) are fixed per
weapon type and not affected by rarity.

### Ranged Weapons

| Weapon | Fire Rate | Damage | Speed | Notes |
|--------|-----------|--------|-------|-------|
| Basic Pistol | 4/s | 5 | 500 px/s | Starting weapon. Reliable, no spin-up. |
| SMG | 8/s | 3 | 450 px/s | Fast fire rate, wider spread. |
| Shotgun | 1.5/s | 4 x 5 pellets | 400 px/s | Devastating at close range, spread at distance. |
| Plasma Rifle | 3/s | 8 | 550 px/s | Projectiles pierce one enemy. |
| Railgun | 0.5/s | 30 | Hitscan | Line through all enemies. 1.5s charge-up telegraph. |
| Grenade Launcher | 1/s | 25 (AoE) | 300 px/s | Arcing projectile, explodes on impact. 48 px blast radius. |

### Melee Weapons

| Weapon | Speed | Damage | Arc | Notes |
|--------|-------|--------|-----|-------|
| Combat Knife | 3/s | 15 | 60° | Quick stabs, short range. |
| Energy Sword | 1.5/s | 25 | 120° | Wide arc, good crowd control. |
| Battle Axe | 1/s | 35 | 90° | Cleaves through multiple enemies. |
| Stun Baton | 3/s | 10 | 60° | Stuns enemies for 0.8s on hit. |
| War Hammer | 0.7/s | 50 | 90° | Massive knockback, 32 px AoE on impact. |

## Enemies

Enemies come in distinct behavioral archetypes. Each floor mixes
these types to create varied combat encounters.

### Damage Scale Reference

The game uses a **medium number scale** (10--100 HP range). This gives
room for granular upgrades and meaningful damage differences without
numbers becoming hard to reason about.

| Scale point | Value |
|-------------|-------|
| Player HP | 100 |
| Weakest enemy (Swarmer) | 10 HP |
| Toughest normal enemy (Heavy) | 80 HP |
| Weakest weapon hit (SMG bullet) | 3 |
| Strongest weapon hit (War Hammer) | 50 |

### Melee Enemies

| Enemy | Speed | HP | Damage | Radius | Behavior |
|-------|-------|----|--------|--------|----------|
| **Swarmer** | 150 | 10 | 10 (contact) | 8 | Rushes in packs. Uses BFS flow field to navigate obstacles. Groups of 2--5. Floor 3+: splits into 2 mini-swarmers on death. |
| **Stalker** | 120 | 20 | 15 (contact) | 10 | Flanker. Circles behind the player, moves erratically to dodge. Attacks from behind. |
| **Heavy** | 60 | 80 | 30 (contact), 40 (charge) | 16 | Armored brute. Steady advance. Charge attack with 0.8s wind-up telegraph. Floor 5+: ground-slam AoE. |
| **Bomber** | 180 | 15 | 25 (explosion, 48 px radius) | 8 | Suicide unit. Sprints at player, explodes on contact or death. Damages nearby enemies. Floor 5+: leaves lingering fire. |

### Ranged Enemies

| Enemy | Speed | HP | Damage | Radius | Behavior |
|-------|-------|----|--------|--------|----------|
| **Grunt** | 50 | 30 | 8 (projectile) | 10 | Basic ranged attacker. Fires while slowly advancing. Floor 4+: fires 3-round bursts. |
| **Sniper** | 0 (repositions) | 15 | 35 (hitscan) | 10 | Telegraphs with laser sight for 1s, then fires. Relocates after each shot. |
| **Suppressor** | 50 | 30 | 5 (projectile, rapid) | 10 | Fires bursts creating danger zones. Area denial -- forces player movement. |

### Special Enemies

| Enemy | Speed | HP | Behavior |
|-------|-------|----|----------|
| **Shielder** | 40 | 40 | Frontal energy shield blocks projectiles. Must be flanked or hit with melee. Can shield nearby allies. |
| **Spawner** | 0 | 60 | Stationary nest. Produces 2 Swarmers every 5s. Must be destroyed to stop reinforcements. |

### Procedural Enemies

Normal enemies use their base archetype stats and behavior. **Elite
enemies** are enhanced variants that spawn at increasing rates on later
floors (5% on floor 1, +10 percentage points per floor; see
[Scaling per floor](#scaling-per-floor)).

Elites are visually distinct (glowing aura, size increase) and always
roll 1--2 random modifiers:

| Enemy Modifier | Effect |
|---------------|--------|
| **Burning** | Leaves a fire trail that damages the player |
| **Armored** | 50% damage reduction |
| **Swift** | +50% movement speed |
| **Splitting** | Spawns 2 mini-versions on death |
| **Regenerating** | Heals 2 HP/s |
| **Shielded** | Has a breakable shield (absorbs 20 damage before breaking) |
| **Enraged** | +30% damage, +20% speed when below 50% HP |
| **Magnetic** | Pulls the player slightly toward it |

The mid-run boss is an elite variant of an existing archetype (e.g., a
"Burning Armored Heavy") with a massive stat boost and one unique
phase mechanic. The final boss is handcrafted with multi-phase
behavior.

## Loot System

All items are available from floor 1. Rarer and more powerful items
simply drop less frequently on early floors. Later floors increase the
probability of higher-rarity weapons, double-modifier rolls, and
powerful passive upgrades. This is the Risk of Rain model: a lucky
floor 1 Railgun drop is rare but possible and exciting.

### Drop Sources

| Source | What drops | Notes |
|--------|-----------|-------|
| **Common enemies** (Swarmer, Grunt) | Passive upgrades (5--10% chance) | No weapon drops -- too numerous |
| **Uncommon enemies** (Stalker, Heavy, Sniper) | Passives (15--20%), ranged weapons (5%) | |
| **Elite enemies** | Guaranteed 1 drop: passive or weapon | Always worth killing |
| **Boss kills** | 1 high-tier passive + 1 weapon. First boss guarantees melee. | Run-defining moment |
| **Loot rooms** | Chest with 1--2 items | Reward for exploration |
| **Secret rooms** | Rare/powerful passive or weapon | Hidden, require finding |

### Weapon drops on the ground

Dropped weapons show their name, rarity color, and modifier names
floating above them. Picking up replaces the current weapon in that
slot. The old weapon stays on the ground for 10 seconds.

## Upgrades (Passives)

Upgrades are **temporary** -- they last for the current run only. Found
as pickups in the world. The player can stack multiple upgrades.
Upgrades activate immediately on pickup (no menu).

### Stat Boosts

| Upgrade | Effect | Stack behavior |
|---------|--------|----------------|
| Reinforced Plating | +15 max HP | Additive |
| Adrenaline Shot | +12% movement speed | Additive |
| Quick Loader | +15% fire rate | Additive |
| Targeting Module | +4 ranged damage | Additive |
| Sharpened Edge | +6 melee damage | Additive |
| Reflex Enhancer | -0.15s dash cooldown | Additive (min 0.2s) |

### On-Hit Effects

| Upgrade | Effect | Stack behavior |
|---------|--------|----------------|
| Incendiary Rounds | Ranged hits ignite (3 DPS for 2s) | +1s duration per stack |
| Bleed Edge | Melee hits cause bleed (4 DPS for 2s) | +1s duration per stack |
| Chain Lightning | 20% chance to arc to a nearby enemy (8 damage) | +10% chance per stack |
| Cryo Coating | Hits slow enemies 25% for 1.5s | +0.5s duration per stack |
| Explosive Tips | Kills trigger 24 px AoE (10 damage) | +5 damage per stack |

### Defensive / Utility

| Upgrade | Effect | Stack behavior |
|---------|--------|----------------|
| Vampire Fang | Melee kills restore 3 HP | +1 HP per stack |
| Thorn Aura | Enemies that hit player take 5 damage | +3 damage per stack |
| Shield Capacitor | Kills grant a 10 HP shield (5s) | +5 HP per stack |
| Scavenger Magnet | +50% pickup radius | Additive |
| Second Wind | Survive a killing blow once per floor (1 HP) | +1 use per stack |

**Synergy design:** Percentage bonuses from modifiers and passives
stack **additively with each other** (not multiplicatively). A
Scorching weapon modifier + Incendiary Rounds passive = combined burn
(longer duration, not double proc). See the
[Damage Model](#damage-model) for the full formula.

## Meta-Progression

Between runs, the player unlocks new options via a **hybrid system**:
achievements unlock items for purchase, accumulated currency buys them.

### Unlock Currency

- **Astro Credits** -- earned from kills, floor clears, and run
  completion. Bad runs still earn some credits. Completing a full run
  earns a large bonus.

### What Unlocks

Meta-progression does **not** carry over in-run power. It expands
options.

#### Starter Weapons (pick 1 per run)

Unlocked weapons replace the Basic Pistol as the starting ranged
weapon. The player selects one before each run.

| Weapon | Unlock condition | Cost |
|--------|-----------------|------|
| Basic Pistol | Default | Free |
| SMG | Reach floor 3 | 50 credits |
| Shotgun | Kill 200 enemies (cumulative) | 100 credits |
| Plasma Rifle | Clear floor 5 | 150 credits |

#### Talents (pick 1 per run)

Talents are build-defining abilities that fundamentally alter playstyle.
The player selects one before each run. 6--8 total talents; unlocked
gradually. Each talent's percentage bonuses feed into the same stat
formula as modifiers and passives (additive within the percentage
pool). Flat bonuses add directly.

| Talent | Type | Effect | Unlock condition | Cost |
|--------|------|--------|-----------------|------|
| **None** | -- | No talent (default) | Default | Free |
| **Pyromaniac** | Mechanical | Dash leaves a fire trail (5 DPS, 2s). Immune to fire damage. | Complete a run | 100 |
| **Berserker** | Stat tradeoff | +50% melee damage, +25% melee speed. -30% ranged damage. | Kill 50 enemies with melee in one run | 75 |
| **Gunslinger** | Stat tradeoff | +30% ranged damage, +20% fire rate. -25% max HP. | Kill 500 enemies (cumulative) | 75 |
| **Ghost** | Mechanical | Dash cooldown halved. Dash distance doubled. Become invisible for 0.5s after dash. | Reach floor 5 without taking damage on floor 1 | 150 |
| **Scavenger** | Mechanical | Start each floor with a random passive upgrade. +25% drop rates. -15% damage. | Collect 30 upgrades (cumulative) | 100 |
| **Juggernaut** | Stat tradeoff | +50 max HP, +Thorn Aura. -25% movement speed. | Take 1000 damage (cumulative) | 75 |

Talents are intentionally limited to 1 per run. Each one should make
the player think "this changes how I play" rather than "this is a small
stat boost."

## Level Generation

### Approach: Hybrid Arena-Dungeon

Each floor is a procedurally generated layout of connected rooms. Rooms
come in distinct types:

| Room type | Behavior |
|-----------|----------|
| **Combat room** | Doors lock on entry. Enemies spawn in waves. Doors unlock when all enemies are dead. |
| **Corridor** | Open passageway connecting rooms. May contain wandering enemies but doors never lock. |
| **Loot room** | Contains a chest with 1--2 items. No enemies. Optional side room off the critical path. |
| **Secret room** | Hidden (no visible door). Contains a rare item. Discoverable via environmental clues or melee-breaking a cracked wall. |
| **Boss room** | Large arena. Floor exit is behind the boss. Appears every 2--3 floors. |
| **Exit room** | Contains the staircase/elevator to the next floor. On non-boss floors, reachable without clearing all rooms. |

### Generation Algorithm

Recommended approach: **BSP (Binary Space Partitioning)** tree or
**room placement with corridor connection**.

1. Subdivide the floor area into regions
2. Place rooms (random size within constraints) in each region
3. Connect adjacent rooms with corridors
4. Assign room types (combat, loot, exit, boss) based on distance from
   spawn
5. Place enemies per room type and floor difficulty
6. Guarantee a clear path from spawn to exit

### Current Implementation

The current tilemap (128x96 tiles, scattered random obstacles and
clusters) serves as the open-world prototype. It will be replaced by
room-based generation as the level system matures. The existing tile
collision, BFS flow field, and camera systems carry over directly.

## Camera & Viewport

- **Type:** Raylib Camera2D, follows the player
- **Viewport:** 800x600 window (configurable)
- **Behavior:** Camera target = player position. Offset = screen
  center. Edge-clamped to world bounds so the camera never shows
  outside the map.
- **Zoom:** 1.0x default. May be adjusted for boss rooms or UI
  overlays.

## Damage Model

### Per-Hit Damage Formula

This formula calculates **per-hit damage** only. It does not cover
fire rate, projectile count, DoT, or on-hit procs -- those are
handled separately below.

Bonuses come in two types: **percentage multipliers** (applied to the
base) and **flat additions** (added after). They never mix in the same
step.

```text
base_damage    = weapon.damage * rarity_multiplier
pct_multiplier = 1.0 + sum of percentage damage bonuses
                 (e.g., Heavy modifier: +0.40, Gunslinger talent: +0.30)
flat_bonus     = sum of flat damage bonuses
                 (e.g., Targeting Module passive: +4, Sharpened Edge: +6)
final_damage   = base_damage * pct_multiplier + flat_bonus
```

- Percentage bonuses stack **additively with each other** (Heavy +40%
  and Gunslinger +30% = x1.70, not x1.40 x 1.30).
- Flat bonuses are added **after** percentage scaling.
- Talents apply their effects to the relevant stat category (damage,
  fire rate, HP, etc.) -- they are not a single global multiplier.

### Non-Damage Stat Stacking

**Weapon stats:**

| Stat | How it combines |
|------|-----------------|
| **Fire rate** | `final_rate = base_rate * (1 + sum_of_pct_bonuses)`. Rapid (+25%) and Quick Loader (+15%) stack: `base * 1.40`. |
| **Projectile count** | Fixed per weapon (e.g., Shotgun = 5 pellets). Not affected by modifiers or passives. |
| **Bounces** | Base bounces (3) + flat modifier bonus (Bouncing: +2) = 5 total. |
| **DoT effects** | Scorching modifier and Incendiary Rounds passive combine by extending duration, not stacking damage ticks. |
| **Slow effects** | Chilling modifier and Cryo Coating passive combine by extending duration. Slow percentage does not stack. |
| **On-hit procs** | Each source rolls independently per hit. |

**Player stats:**

| Stat | How it combines |
|------|-----------------|
| **Movement speed** | `final_speed = base_speed * (1 + sum_of_pct_bonuses)`. Adrenaline Shot (+12%) stacks additively with talent bonuses. Juggernaut's -25% is a negative bonus in the same sum. |
| **Max HP** | `final_hp = base_hp + sum_of_flat_bonuses`. Reinforced Plating (+15) and Juggernaut (+50) stack additively. |
| **Dash cooldown** | `final_cd = max(base_cd - sum_of_flat_reductions, 0.2)`. Reflex Enhancer (-0.15s) stacks with floor. Ghost talent halves the result. |

### Enemy HP Scaling

See [Scaling per floor](#scaling-per-floor) for the formula and all
per-floor scaling rates. Floor 1 is baseline (1.0x).

### Invincibility Frames

- Player has 0.5s of invincibility after taking damage (prevents
  multi-hit from clustered enemies)
- Dash grants full invincibility for its 0.15s duration

## Spawn System

### Current Implementation

Wave-based spawning: 2--5 swarmers every 3 seconds, spawned just
outside the camera viewport on walkable tiles.

### Target Implementation

Room-triggered spawning for combat rooms:

1. Player enters a combat room
2. Doors lock
3. Enemies spawn in 2--4 waves with short delays between waves
4. Room composition determined by floor number and room difficulty
   rating
5. Doors unlock when all waves are cleared

Corridors may contain pre-placed wandering enemies (not wave-spawned).

## AI Behavior

### Swarmer

- **State:** Seek (only state)
- **Behavior:** Move toward player using BFS flow field (long range) or
  direct seek (within 3 tiles). Die on player contact.
- **Implementation status:** Done

### Stalker

- **States:** Orbit -> Dash-in -> Attack -> Retreat
- **Behavior:** Circles at medium range (~5 tiles), picking a flank
  angle. Randomly dash-attacks from behind. Retreats after attacking.
  Erratic movement to dodge shots.

### Heavy

- **States:** Advance -> Telegraph -> Charge -> Recover
- **Behavior:** Walks steadily toward player. When within 4 tiles,
  stops and telegraphs a charge (0.8s wind-up, visual/audio cue), then
  charges in a straight line dealing massive damage. Recovers for 1.5s
  (vulnerable).

### Bomber

- **States:** Sprint -> Detonate
- **Behavior:** Sprints toward player at high speed. Explodes on
  contact or death. Explosion damages all entities in radius (including
  other enemies). Chain reactions possible.

### Grunt

- **States:** Advance -> Aim -> Fire -> Cooldown
- **Behavior:** Advances slowly. Stops at medium range (~6 tiles) to
  aim and fire. Predictable shot timing (useful for learning dodge
  patterns).

### Sniper

- **States:** Position -> Telegraph -> Fire -> Relocate
- **Behavior:** Finds a long-range position. Draws a visible laser
  sight line for 1s (telegraph). Fires a high-damage hitscan shot.
  Immediately relocates to a new position.

### Suppressor

- **States:** Advance -> Burst-fire -> Reposition
- **Behavior:** Advances to medium range, fires rapid bursts creating
  danger zones (projectile clusters). Repositions between bursts. Area
  denial role.

### Shielder

- **States:** Advance -> Shield-up
- **Behavior:** Advances slowly with frontal shield facing the player.
  Shield blocks all frontal projectiles. Must be flanked (shot from
  behind) or hit with melee. Can extend shield to cover nearby allies.

### Spawner

- **States:** Idle -> Spawn -> Cooldown
- **Behavior:** Stationary. Produces 2 Swarmers every 5s. Destroy to
  stop reinforcements. High HP to require sustained focus.

## Visual Style

**Stylized neon sci-fi with layered geometric art.**

- Currently: layered geometric shapes with neon outlines, glow auras,
  and facing indicators. Reads well at small sizes.
- Target: refined geometric art or pixel sprites with clean outlines
  and vibrant palettes. Maintain the neon sci-fi aesthetic.
- High-contrast visual language: player (cyan), enemies (red/orange),
  projectiles (orange/magenta), pickups (green/gold), walls (steel
  gray)
- Dark backgrounds with bright entity rendering
- Screen shake, muzzle flash, hit sparks, and damage numbers for
  combat feedback

Reference images and concept art go in `design/assets/`.

## UI & Feedback

### HUD

| Element | Position | Description |
|---------|----------|-------------|
| **Health bar** | Bottom-left | Horizontal bar. Green -> yellow -> red as HP drops. Flashes critically low. |
| **Dash cooldown** | Below health bar | Bar indicator. Fills as cooldown recharges. Flash when ready. |
| **Current weapons** | Bottom-right | Icons for ranged and melee slots. Active weapon highlighted. Modifier names shown. |
| **Active upgrades** | Top-left | Small icon strip of collected passives. Tooltip on pause. |
| **Floor indicator** | Top-center | Current floor number (e.g., "Floor 3"). |
| **Kill counter** | Top-center (below floor) | Total kills this run. |

### Minimap

| Aspect | Design |
|--------|--------|
| **Position** | Top-right corner |
| **Size** | ~15% of screen width, semi-transparent |
| **Content** | Explored rooms filled, unexplored adjacent rooms as dim outlines. Player dot. Exit marked. |
| **Fog of war** | Rooms appear only after the player enters them. |

### Gameplay Feedback

| Event | Feedback |
|-------|----------|
| **Player hit** | Red vignette flash, screen shake, damage number, hurt SFX |
| **Enemy hit** | Hit spark, damage number floats up, hit SFX |
| **Enemy kill** | Death animation, score pop-up, kill SFX |
| **Elite kill** | Enhanced effects: larger shake, loot drop fanfare |
| **Multi-kill** | Combo text ("DOUBLE KILL"), intensified shake |
| **Upgrade pickup** | Icon enlarges on HUD, chime, text flash |
| **Weapon pickup** | Name + modifiers displayed, equip SFX |
| **Dash used** | Motion trail, whoosh SFX, cooldown drains |
| **Dash ready** | Indicator flash, subtle chime |
| **Low HP** | Health bar pulses, heartbeat SFX, red screen tint |
| **Floor clear** | Fanfare, brief stat summary |
| **Player death** | Slow-motion frame, fade to run summary |

### Design Principles

- **Readability over decoration.** High-contrast colors that stand out
  against any background.
- **No menu-driven gameplay.** Upgrades activate on pickup, weapons
  swap on interact.
- **Feedback layering.** Important events use visual + audio + HUD
  cues. Minor events use one channel.
- **Scale with intensity.** Shake, particles, and text scale with
  action intensity.

## Audio

**All audio is procedurally generated at runtime** -- no external asset
files. This keeps the project self-contained and eliminates asset
management overhead.

### Current Implementation

| Sound | Description |
|-------|-------------|
| Bullet fire SFX | Descending square wave burst (880 -> 440 Hz) |
| Player hit SFX | Low-frequency impact thud with distortion (120 Hz) |
| Enemy hit SFX | Metallic ping with inharmonic overtone (1200 Hz) |
| Death screen music | Looping sci-fi synth: detuned oscillators, A1 drone, tritone melody |

### Planned

| Sound | Description |
|-------|-------------|
| Melee swing SFX | Whoosh (per weapon type) |
| Dash SFX | Short burst swoosh |
| Upgrade pickup | Bright chime |
| Weapon pickup | Mechanical equip click |
| Low HP warning | Heartbeat pulse |
| Gameplay music | Driving synth tracks, intensity scales with combat |
| Boss music | Distinct per-boss theme |

### Priorities

Gameplay-critical audio (enemy telegraphs, low HP) takes priority over
ambient audio. Every SFX should convey information, not just ambiance.

## Technical Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Engine | **Raylib 5.5** | Zero deps, small binary, minimal API, excellent workshop fit |
| Language | **C99** | Raylib's native language. Simple, fast compilation. |
| Build | **CMake** with FetchContent | Auto-fetches Raylib 5.5 + Unity 2.6.1. Task runner via go-task. |
| Test framework | **Unity** (ThrowTheSwitch) | C-native, minimal, integrates with CTest. |
| Audio | **Procedural** (runtime-generated) | No asset files. Self-contained. |
| Level gen | **Code-driven** procedural generation | No external editor dependency. |
| Architecture | **Struct-per-archetype** with object pools | Player, Enemy, Bullet as separate types. Static lib shared between exe and tests. |
| Input | **Decoupled** (extract input -> pass struct to update) | Enables testing game logic without Raylib window. Partially done (`player_calc_move_dir`). |

## Architecture Notes

### Module Structure

| Module | Purpose |
|--------|---------|
| `main.c` | Window init, audio device, game loop |
| `game.c/h` | GameState, update/draw orchestration, phase management |
| `player.c/h` | Player movement, aiming, dash, drawing |
| `bullet.c/h` | Bullet pool, firing, movement, wall bounce, lifetime |
| `enemy.c/h` | Enemy pool, spawning, AI, drawing |
| `tilemap.c/h` | World grid, procedural generation, tile collision, BFS flow field |
| `audio.c/h` | Procedural audio generation and playback |
| `vec2.c/h` | 2D vector math (independent of raymath) |
| `settings.c/h` | Persistent settings (INI format) -- PR #20 |

### Patterns

- **Object pools** with fixed-size arrays (`MAX_BULLETS`, `MAX_ENEMIES`)
  for all entity types. Linear scan for activation/deactivation.
- **Static library** (`astro_blitz_lib`): game logic shared between
  main exe and test suites.
- **Input decoupling**: pure logic functions receive computed inputs
  (not raw Raylib calls) for testability. Extend this pattern to all
  new systems.
- **NULL tilemap in tests**: update functions accept `const Tilemap *`
  and skip wall collision when NULL, allowing tests to run without
  generating a full map.
- **Deterministic generation**: tilemap uses a local xorshift32 PRNG
  with seed parameter for reproducible test scenarios.

## Implementation Status

Last updated: 2026-04-10

**Note:** The current implementation uses placeholder damage values
(Swarmer HP = 1, bullet damage = 1). The medium number scale defined
in this document (Swarmer HP = 10, Pistol damage = 5, etc.) is the
target and will be applied when the weapon system is implemented in
Sprint 3.

| System | Status | Test Count |
|--------|--------|------------|
| Game loop + window (800x600, 60 FPS) | Done | -- |
| Tank controls + 8-directional (configurable) | Done (PR #20) | 19 + 14 |
| Mouse aiming (world-space) | Done | -- |
| Shooting (Basic Pistol, 128-bullet pool) | Done | 30 |
| Bullet wall bouncing (3 max) | Done | 3 |
| Dash (invincible, 0.15s, 1.0s CD) | Done | 3 |
| Camera2D (follows player, edge-clamped) | Done | 3 |
| Tilemap (128x96, obstacles, clusters, borders) | Done | 36 |
| Tile collision (player, enemy, bullet) | Done | -- |
| BFS flow field pathfinding | Done | 8 |
| Enemy: Swarmer (seek AI, contact damage) | Done | 35 |
| Wave spawning (2--5 swarmers, 3s interval) | Done | -- |
| Collision (bullet-enemy, enemy-player) | Done | 6 |
| Damage (bullets -> enemies, enemies -> player) | Done | -- |
| Game over (death screen, stats, restart) | Done | 6 |
| Procedural audio (shoot, hit, enemy hit, death music) | Done | 19 |
| Vec2 math module | Done | 28 |
| Main menu + pause menu + settings | Done (PR #20) | 26 |
| **Total unit tests** | **190 + 26 (PR #20)** | **216** |
| | | |
| Melee attack | Not started | -- |
| Weapon variety (SMG, Shotgun, etc.) | Not started | -- |
| Weapon modifier system (prefix/suffix) | Not started | -- |
| Weapon pickups | Not started | -- |
| Passive upgrade system | Not started | -- |
| Room-based level generation | Not started | -- |
| Floor progression | Not started | -- |
| Additional enemy types (8 remaining) | Not started | -- |
| Elite enemy modifiers | Not started | -- |
| Boss encounters | Not started | -- |
| Meta-progression (talents, unlocks) | Not started | -- |
| Minimap | Not started | -- |
| Game feel (screen shake, damage numbers) | Not started | -- |
| Gameplay music | Not started | -- |
| Sprites/refined art | Not started | -- |

## Development Roadmap

### Short-Term (Sprints 1--3): Fun Core Loop

**Sprint 1: Melee + Game Feel**

- Melee attack (right-click): arc hitbox, damage, cooldown
- Screen shake (on player hit, on enemy kill)
- Damage numbers (floating text)
- Enemy hit flash (brief white overlay)
- Tests for melee and game feel systems

**Sprint 2: Grunt Enemy + Enemy Bullets**

- Grunt AI (advance, aim, fire, cooldown)
- Enemy bullet pool (tagged `BULLET_ENEMY`)
- Enemy projectile-player collision
- Mixed spawning (Swarmers + Grunts)
- Tests for Grunt AI and enemy bullets

**Sprint 3: Weapon System + Modifiers**

- Weapon struct: fire rate, damage, speed, spread, projectile count
- Implement SMG and Shotgun
- Weapon modifier system (prefix/suffix roll on drop)
- Weapon pickups (interact with E)
- HUD: weapon display with modifier names
- Tests for weapon stats, modifier application, pickup logic

### Medium-Term (Sprints 4--8): Full Roguelike

**Sprint 4: Room-Based Levels**

- BSP or room-placement generation
- Room types: combat (lock-in), corridor, loot, exit
- Room-triggered enemy spawns
- Floor transitions

**Sprint 5: Upgrades + Loot**

- Passive upgrade system (stackable modifiers)
- Drop tables (enemy drops, chests, boss loot)
- 6 stat boost + 5 on-hit + 5 defensive upgrades
- HUD: upgrade icon strip

**Sprint 6: More Enemies + Elites**

- Stalker, Heavy, Bomber AI
- Elite modifier system (1--2 random mods, visual glow)
- Per-floor enemy composition tables

**Sprint 7: Run Structure + Difficulty**

- 5--7 floor progression
- Difficulty scaling per floor (HP, speed, spawn rate, abilities)
- Floor exit discovery mechanic
- Run summary screen

**Sprint 8: Bosses + On-Hit Effects**

- Mid-run boss (elite variant with unique phase)
- Final boss (handcrafted multi-phase)
- On-hit effects: Incendiary, Cryo, Chain Lightning, Explosive Tips

### Long-Term (Sprints 9+): Polish & Ship

- Meta-progression: talents, starter weapons, unlock currency
- Secret rooms, environmental storytelling
- Minimap with fog of war
- Audio expansion: gameplay music, per-weapon SFX, boss themes
- Visual polish: sprites or refined procedural art, particles, trails
- Performance profiling (60 FPS with 64 enemies + 128 bullets + BFS)
- Pause menu improvements, settings expansion
- Save/load (optional, stretch goal)
