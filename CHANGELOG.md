# Changelog

All notable completed milestones for the TRPG game project.

Each entry is keyed to its **commit**, ordered **newest first** by commit date.
`git show <hash>` for details.

---

## `(uncommitted)` — 2026-09-09 — "debug roster: Debug Overlord (99 stats, all skills), from-scratch units, +1 level per battle victory"

### Roster / debug setup

- New **Debug Overlord** unit (`assets/units/debug_overlord.json`): 99 in
  maxHp/maxMp/defense/magicDefense/evasion, normal attack/magic/speed,
  learns **every** skill (real + placeholder).
- Roster now seeds 5 units on a new game: Aria, Soldier, Elf Archer,
  Elin Duelist, and the Debug Overlord (`RosterSystem::ensureDefaultUnits`).
- Normal units are back to **from scratch**: level 1, authored base stats, and
  no combat skills (`skills` arrays removed from their JSONs).

### Progression rule (level-up per combat)

- `RosterUnit` gained a `level` field — it is now the source of truth for a
  unit's level, persisted in the campaign/battle save formats.
- Every battle victory grants **exactly +1 level to every alive, deployed
  roster unit** (capped at the unit's max level), replacing the old
  exp-add award in `BattleState::awardVictoryBonusXp`.
- Battle units are rebuilt from their template and grown to the persisted
  level via the new `Unit::growToLevel()` (per-level stat growth only;
  skill-point spending / skill progression is the next milestone). Raw
  `exp` is still accumulated for a future XP curve but no longer drives level.

---

## `(uncommitted)` — 2026-09-09 — "abilities: heal/buff/debuff skills, MP + cooldowns, enemy-attack VFX"

### Real skill effects

- Skills now do more than damage. `effectTypes` (`heal`/`buff`/`debuff`) drive
  resolution:
  - **Heal** restores `basePower + caster magic` (no miss roll), shows a green
    `+N` float and green ring.
  - **Buff/debuff** apply a flat signed stat mod for a set number of the
    target's own turns (`statKey`/`statAmount`/`buffTurns`), with a labelled
    float (`ATK +3`, `ATK -2`) and blue/violet ring. One mod per stat —
    re-applying replaces.
  - Offensive skills may carry a rider debuff/buff that lands on hit.
- Targeting splits by role: supportive skills (heal/buff) target allies/self
  (self-cast allowed — range tiles now include the caster's tile for them);
  offensive skills keep targeting enemies. Buff skills with an area still only
  need one valid ally in the splash.
- MP was already costed; **cooldowns** are new (`cooldown` in skill JSON,
  `>0` disables the skill in the menu, labelled `[CD N]`, ticking down on the
  caster's own turn starts).
- Damage/heal/buff previews are now effect-aware everywhere (top battle text
  and the preview box: `DMG 12 ACC 92%` / `HEAL +24` / `ATK +3 (3T)`).

### New content

- Skills: `heal` (Aria, Elf Archer), `valor` (buff — Soldier, Elin Duelist),
  `weaken` (debuff — Aria). Fireball gained `cooldown: 2`.
- New `compat/combat/SkillEffects.*` helper module (classification, heal math,
  stat-mod text); unit stat model gained `TemporaryStatMod` + per-skill
  cooldown tracking (`Unit::{applyStatMod, activeStatMod, skillCooldownLeft,
  setSkillCooldown}`), ticked in `resetTurn()`.

### Enemy-attack VFX

- Enemy attacks are now fully animated: the AI returns a decision
  (`EnemyAttackResult`) without applying damage; `BattleState` applies it
  through the session once the unit arrives, spawning the hit/dodge ring plus
  floating damage/MISS exactly like player attacks.

---

## `(uncommitted)` — 2026-09-08 — "battle VFX: fix flashing damage text, ring artifact; strip debug scaffolding"

### Battle VFX

- Fixed the floating damage number "flickering once" at the end of its rise.
  **Fix, in brief:** `TTF_RenderText_Blended` ignores the requested color alpha
  (`fg.a`), so the number rendered fully opaque its whole life and then hard-cut
  on the erase frame. `Renderer::renderText` now rasterizes at `a=255` and fades
  via `SDL_SetTextureAlphaMod(texture, color.a)` (plus an early-out when
  `a==0`), so the fade actually reaches the pixels and the end pop is gone.
  (Full details: `ISSUE.md`.)
- Placeholder hit ring no longer shows solid "red line" bars at its top/bottom
  poles: `drawRing` now draws two sampled circle outlines instead of an
  integer-scanline disc.

### Cleanup

- Removed the debug Tab FX-mode cycler, its on-screen banner, the per-frame
  VFX logs (`FloatText`/`CombatAnim`/`Render`), the `fx_mid.png`/`fx_end.png`
  probe dumps, and the temporary `Renderer::saveRenderTarget` helper. The F1
  AI/Human toggle stays.

---

## `(uncommitted)` — 2026-09-07 — "exp from victories + in-game save deletion"

### Gameplay / progression

- Victory now grants quest-completion EXP to every alive, deployed roster
  member: `GameConstants::BATTLE_VICTORY_XP` plus any event-driven
  `m_pendingRewardXp`, written back to `RosterUnit.exp` (awarded exactly once
  per battle, guarded by `m_rewardXpAwarded`).
- Battle `Unit`s are reconstructed from their roster's accumulated EXP
  (replayed via `Unit::gainExp` at spawn), so levels carry across battles.
- `Unit` stores its roster identity (`getRosterInstanceId()` /
  `setRosterInstanceId()`).

### Save management

- All save data now lives in a single `user.dat` file (the whole queue:
  metadata + every campaign/battle payload, obfuscated) instead of a
  separate save index + per-slot files. `deleteSave(slotIndex)` and
  `consumeBattleSave` remove an entry from that one file.
- Fixed save read-back so saves round-trip across restarts: the obfuscation
  wrote a stray sentinel byte (`0x5A` after XOR) that broke JSON parsing,
  which showed up as "Load" staying disabled after a restart. The queue is
  now encoded length-exact.
- `SaveManager::loadCampaign(slotIndex, out)` loads a specific campaign save;
  `loadLatestCampaign` delegates to it.
- Title `Load Game` and world-map `Load` open a save list (campaign saves,
  newest first) instead of loading the latest automatically.
- `ButtonMenuWindow` gains an optional delete mode used by the save list:
  a red `X` per row, the X key emits `ActionId::DeleteSave` for the focused
  row, and a footer hint ("X: Delete save file") renders under the panel.
- Deleting goes through a `WindowId::SaveListDeleteConfirm` prompt
  (Cancel-selected by default) before the file is removed.
- `MainMenuState` refreshes save presence at boot (calls `loadIndex`).

---

## `ac27522` — 2026-09-06 — "inventory fixed"

### Inventory / gear

- Inventory/gear integration pass: equipped gear is reserved (not duplicated),
  `RosterSystem` equip/unequip is atomic, and battle `Unit` objects receive a
  resolved `EquipmentLoadout` snapshot from their roster entry on spawn.
- `Gear.cpp` one-line stub removed and its type folded into the inventory
  subsystem.
- Unit HP/MP current-value sync across gear swaps (no clamp/loss on unequip).

### Turn & enemy flow

- New `EnemyTurnController` (in addition to `HumanTurnController`) owns enemy
  turns.
- New `MovementCommitSystem` for logical move commit, decoupled from visual
  interpolation.
- New `AttackRange` + `RangeRule` (ranged attack validity by rule) in `battle/map`.

### Tools / data

- `SkillLoader` gains directory/bulk load and progression metadata handling.

### Refactors / cleanups

- `DeploymentPhaseController`, `BattleState`, `WorldMapState`, and the battle
  renderer refactored toward focused controllers/systems.
- Attack-range rendering and hit resolution reworked.
- Movement cost stays real path cost (no reintroduction of Manhattan-based cost).

---

## `c178d4f` — 2026-09-04 — "Fix stale inspect dim reset, pause-menu dim, and deduplicate engine helpers"

- `BattleMenuController`: reset `m_inspectWindow` on `BattleInspect` ActionCanceled
  so closing unit inspect clears the render dim (previously only `closeAllMenus`
  reset it, leaving the darken stuck until the pause menu happened to close).
- `BattleState`: dim behind the system/pause menu too, so ESC in combat reads as
  a paused state.
- `Pathfinder`: use engine `MathUtils::manhattanDistance` instead of a local copy.
- `WorldMapState`: use engine `MathUtils::lerp` per-component instead of a local
  `Vec2f` lerp.
- `UnitPanelWindow` / `DamagePreview`: use `Renderer` `measureText`/`alignInRect`/
  `renderTextInRect` instead of hand-computed text centering.
- `DeploymentWindow`: removed unused `centeredTextX` dead code.

---

## `92375bc` — 2026-09-03 — "ui/gear: enemy debug loadout, dual-wield, hold-to-repeat, pause menu"

- Debug-only spawn-time enemy loadout using catalog IDs seeded by
  `seedDebugInventoryWithTestGear`.
- `EquipRules::isOffhandEligibleGear()` lets one-handed non-ranged weapons appear
  in the Offhand filter (dual-wield); `canEquip` also validates Weapon-slot equips
  against existing Offhand items.
- New game-side `UIKeyRepeat` helper (350ms initial / 90ms steady), wired into
  `InventoryWindow` and `UnitDetailWindow` SlotSelect/ItemSelect for W/S and A/D
  continuous navigation.
- SlotSelect cursor remembers/restores the previously selected slot after
  returning from ItemSelect; Tab in ItemSelect opens a read-only description card.
- `BattleMenuController` now handles `WindowId::BattleSystemMenu` in both
  Deployment and Combat (was previously gated to Deployment only, so in-combat
  Resume/Quit were dropped events).
- Tighter stat-delta spacing (6px gap) in `UnitDetailWindow`.

---

## `d1d3453` — 2026-09-03 — "inventory integration"

- First inventory system integration: `Item`/`Inventory`/`Gear`/`EquipRules`/
  `GearCatalog` subsystem wired into the game target.
- Added placeholder promotion skills and new unit templates
  (`elf_archer`, `elin_duelist`, etc.).
- Introduced/expanded design docs: `ARCHITECTURE.md`, `DATA_SCHEMAS.md`,
  `PLAN.md`, `RACE_DESIGN.md`, `UNIT_DESIGN.md`, `AGENTS.md`.

---

## `05a3030` — 2026-08-06 — "Fix: include DialogueController in game target"

- Added `DialogueController` to `game/CMakeLists.txt` (missing source in the
  target).

---

## `6125f05` — 2026-08-06 — "some fixes"

- Assorted bug fixes.

---

## `034e6c2` — 2026-07-31 — "fixed a lot of things + refactor + reestructuration"

- Broad re-structure/reorganization of the `game/src/` tree and battle modules
  toward the current module ownership layout.

---

## `d435eb8` — 2026-07-28 — "ui fixed"

- UI fixes.

---

## `c95f4ad` — 2026-07-28 — "Refactor battle UI manager and menu system"

- Refactored the battle UI manager and action/skill/system/inspect menu system.

---

## `68a888b` — 2026-07-27 — "idk"

- Miscellaneous changes (message-less bucket).

---

## `a7cac24` — 2026-07-27 — "fixed camera issues"

- Camera fixes and regression cleanup.

---

## `8e4cf8b` — 2026-07-22 — "some refactor + renaming"

- Refactor and renaming pass.

---

## `6ac0bef` — 2026-07-21 — "Settings unification: SettingsRowWindow, IFocusable integration, ConfirmWindow rebuild, ButtonControl callback refactor"

- Unified settings rows via `SettingsRowWindow`, integrated `IFocusable`,
  rebuilt `ConfirmWindow`, and refactored `ButtonControl` callbacks.

---

## `2f9dcfd` — 2026-07-20 — "refactored all buttons?"

- Button/menu refactor pass.

---

## `9715db9` — 2026-07-19 — "some refactor about menus buttons"

- Menu/button refactor.

---

## `b8d05c4` — 2026-07-19 — "WIP: split SettingsState into Graphics/Audio via SettingsRowWindow"

- WIP: split `SettingsState` into Graphics and Audio settings via
  `SettingsRowWindow`.

---

## `2c49ac7` — 2026-07-18 — "some refactor SETTINGS STATE"

- Settings state refactor.

---

## `3b344ce` — 2026-07-18 — "some refactor + removed magic strings"

- Refactor and removed magic strings.

---

## `7152533` — 2026-07-14 — "added placing unit enhancement, letter on units, and some refactor"

- Enhanced unit placement, added letters on units, and refactored.

---

## `d50b9cc` — 2026-07-11 — "minor ui and battling fixes"

- Minor UI and battle fixes.

---

## `fe5b02b` — 2026-07-10 — "some refactor - need more"

- Refactor (incomplete).

---

## `40a585e` — 2026-07-09 — "ui refactor"

- UI refactor.

---

## `d9f7ee7` — 2026-07-09 — "tried to fix fonts"

- Font fixes.

---

## `6fa5df6` — 2026-07-09 — "fixed sliders/value display"

- Fixed sliders and value display.

---

## `c9d9fe7` — 2026-07-08 — "fixes"

- Assorted fixes.

---

## `ded2d7d` — 2026-07-08 — "idk"

- Miscellaneous changes (message-less bucket).

---

## `895d56d` — 2026-07-08 — "some shit"

- Assorted changes.

---

## `e190d8e` — 2026-07-07 — "fixed menus?"

- Menu fixes.

---

## `55e8e13` — 2026-07-06 — "fix ESC key"

- Fixed ESC key handling.

---

## `194bba9` — 2026-07-06 — "refactor(battle): split BattleState into focused controllers/systems"

- Split `BattleState` into focused controllers/systems (first major
  orchestration refactor).

---

## Notes

- `RACE_DESIGN.md` / `UNIT_DESIGN.md`: design-only race roster + stat growth
  tables are in progress (6-race roster: Human, Elin, Kragan, Fennari, Tok,
  Gria). Unit progression / class system remains deferred — not yet wired into
  code.
