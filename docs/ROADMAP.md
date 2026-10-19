# TRPG Game (game_1) - Roadmap

## Mission

Deliver a stable tactical RPG vertical slice on top of the shared engine while keeping gameplay systems modular and testable.

## Current Vertical Slice Status

The core path is implemented in code (status 2026-10-19; what shipped and when
is in `CHANGELOG.md`):

- App boot and scene flow (`BootState` -> `MainMenuState` -> `IntroState` (New Game tutorial duel) -> `WorldMapState` -> `BattleState` -> `ResultState`)
- Deployment phase with roster selection and forced/critical unit handling
- Combat phase with turn queue, movement, attack/skill targeting, confirm, and resolution
- Event hooks (`BattleEventSystem`) for dialogue/reward/spawn/end-battle actions
- Tactical UI stack (action, skill, system, inspect, confirm, unit panel, damage preview)
- Settings flow (graphics/audio) with persisted settings
- Battle rendering split into world logical pass and native UI/effects pass

## Where the rest went

This file used to duplicate the module map, engineering rules, deferred list and doc
index. Those now have single homes:

- Module ownership / battle flow / render pipeline: `ARCHITECTURE.md`
- Agent rules, deferred ("do NOT build") list, doc index: `../AGENTS.md`
- Live work items and verification: `TODO.md`, `archive/MILESTONES.md`
- What shipped, when: `CHANGELOG.md`

## Roadmap — Remaining Work (living plan)

Everything below is on the table. Order matters: each phase unlocks the next
(economy before shop, save fidelity before any persisted meta-system, quests
before the tavern becomes the quest-giver). True hard no-go items live in
`../AGENTS.md` §10 (monster/Undead recruits, races beyond Human/Elf/Elin).

### Category A — Campaign loop & economy

- **Gold / currency** — **(M1 — shipped 2026-09-12)** campaign purse in
  `PartyContext` (starting kit `kStartingGold`, victory rewards, save v2,
  shown in the Party window + victory overlay). Remaining income sources:
  item sell value, quest rewards.
- **Item drops** — **(M1 — shipped 2026-09-12)** per-battle drop tables via
  `BattleRewards.items` (gear keys resolved by `GearCatalog::findByKey`).
  More drop item variety can come with new catalog content.
- **Battle reward summary screen** — **(M1 — shipped 2026-09-12)** victory
  overlay now lists gold / XP / level-ups / drops.
- **Shop** — **(M3 — shipped 2026-09-13)** buy + sell flow in the new
  `ShopWindow` (`ShopStock` stock table by gear key; buy = `goldValue * 2`,
  sell = `goldValue`; per-line confirm; live purse; refresh after each
  transaction).
- **Tavern / town hub** — **(M3 — shipped 2026-09-13)** a `NodeType::Town`
  ("Riverwatch") on the world map offering: quest pickup (`offeredAtTavern`
  quests via the quest board), recruit candidates (`TavernData` +
  `RosterSystem::recruit`, party capped at 6), and NPC lore/banter dialogue
  (`TavernData` + `lore.json`).
- **Quest system** — **(M2 — shipped 2026-09-13)** main quests + side quests:
  `QuestCatalog` (id, title, brief, objective), quest journal UI, active/complete
  quest state, and completion hooks via `BattleEventSystem` (`OnVictory`,
  `OnDefeat`, `OnUnitDeath`). Gated world-map progression (main quest unlocks
  regions).
- **Story & narrative dialogue** — scripted pre-battle / mid-battle /
  post-battle beats on top of the existing `DialogueController` +
  `BattleEventSystem` scaffolding (currently used by trigger-driven lines).
- **Save fidelity** — **(M1 gold portion — shipped 2026-09-12; format v2)** and
  **(M2 quest portion — shipped 2026-09-13; format v3)**: gold, roster
  equipment, inventory, EXP persist; the campaign save now also persists quest
  state (`questLog`). Remaining: world position and progression state for
  future meta-systems.

### Category B — Progression & combat depth

- **Unit ability progression** — per-level skill unlocks / ability points,
  growth preview, level-up notification + spending UI; promotions later.
  `SkillProgression` now carries the real active-skill catalog + class recipes
  and `RosterUnit` owns points/skills (M4). **(M5 slice 3 shipped
  2026-09-16): skill power scaling** — damage/heal skills grow by
  `powerPerLevel` per learned level (`makeHitContext` reads the skill's
  progression level; enemies without progression fight at authored power),
  and the real active skills now level to 3 (costs 1/1/2 pts). Remaining:
  passive/reaction combat effects, the promotion class tree, and a level-up
  notification in the victory screen. (`design/UNIT_PROGRESSION.md` is the design source.)
- **Status effect pipeline** — **(M5 slice 1 shipped 2026-09-15)**
  `StatusType`/`ActiveStatus`/`StatusEffectApply` model, `Unit` apply/tick
  (DoT stacking, turn locks, sleep-breaks-on-damage), skill JSON `statuses`
  riders applied on landed/supportive casts, turn-start ticks + skips, turn
  banner + unit-detail status display. **(slice 2 shipped 2026-09-16)**
  buffs & debuffs as statuses: `Barrier`/`AttackUp` (+25% base stat) and
  `ArmorDown`/`AttackDown` (−10% total stat) folded into `tempMod()`;
  skills `valor`/`weaken` migrated, new `barrier`/`sunder`. Remaining (later
  M5 slices): more content statuses, cursed terrain.
- **Battle variety** — **(terrain part shipped 2026-09-15)** `TileClass`
  `GrassAtk`/`GrassMag`/`GrassEva` flags are now consumed by combat
  (`CombatSystem::applyTileBonuses`) and actually placed on the 1v1 test map.
  **(M5 slice 4 shipped 2026-09-16): boss battle + scripted waves** — new
  `boss_arena` map, Bandit Chief boss + Raider reinforcements, `KillBoss`
  victory rule, turn-gated one-shot wave events (`BattleEvent.minTurn`/`once`),
  `EnemyDefinition.level` honoured via `UnitSpawn.level`. Remaining: more
  maps, enemy skill casting, balance pass.
- **Skill meta** — passive/equippable skills; multi-cast chains (previously
  deferred); rider debuffs already partially wired.
- **Battle variety** — more maps, enemy archetypes (Bandit Chief + Raiders
  shipped in the boss arena); terrain modifiers are live (M5 slice 1, see
  status pipeline above). Remaining: more maps, enemy skill casting, boss
  scripting variety, balance pass over front/side/rear and skills (MP costs).

### Category C — Presentation & platform

- **Sprites** — replace placeholder circles with real unit art (idle/walk/
  attack/damage frames, facing-aware) + map props + portraits.
- **Animation / VFX** — real action & skill animation pipeline beyond the
  current `CombatAnimationSystem` rings/diamonds; hit-stop, screen shake,
  projected damage numbers.
- **Audio** — **the engine has no audio backend at all** (only a persisted
  "Music Volume" setting row). Requires an engine-side audio module, then
  game-side music/SFX catalogs. Independent milestone with engine investment.
- **UI polish & onboarding** — portraits, transitions, save prompts, and a
  short controls tutorial for the battle flow (battle controls tutorial
  shipped 2026-09-24; shown once per new game at first combat start).
- **Gamepad support** — engine `GamepadCode` is a stub; optional later.

### Recommended first milestones (dependency-ordered)

1. **M1 — Rewards + economy + save fidelity** — **shipped 2026-09-12**: gold,
   drops, battle reward summary, and save/load for these systems. Unlocks
   everything money-related and makes the campaign persist.
2. **M2 — Quest spine (main + side)** — **shipped 2026-09-13**: catalog +
   journal + quest-gated nodes + completion hooks.
3. **M3 — Town hub** — **shipped 2026-09-13**: shop (buy/sell), tavern
   (recruit + quests + lore), town nodes on the world map.
4. **M4 — Progression** — **shipped 2026-09-14**: +1 skill point/level-up,
   `SkillProgressionWindow` Learn/Equip panel, real active-skill unlocks,
   battle import of learned/equipped skills, save format v4.
5. **M5 — Combat depth** — **(slices 1–4 shipped)**: status effects pipeline
   (poison/burn/stun/sleep DoT + turn locks + skill riders, slice 1),
   buffs/debuffs as statuses (slice 2), skill power scaling per level
   (slice 3), boss battle + scripted waves + enemy leveling (slice 4).
   Remaining slices: more content, enemy skill casting, balance.
6. **M6 — Presentation/platform**: sprites, VFX, audio, onboarding polish.

Audio (M6) is the only item that forces engine work; budget it separately or
start it early in parallel if sound matters for the game feel.
