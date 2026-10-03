# TRPG Game — TODO

## Active Bugs

- [x] **Movement cost**  
       Movement points are spent based on Manhattan distance instead of actual path length from pathfinder.  
       _Files: `battle/map/MovementRange`, `battle/map/Pathfinder`_

- [x] **Cursor position after combat**  
       Cursor remains on target tile after attack/spell. Add cursor to `AttackResolutionContext` and reset to active unit when battle menu reopens.  
       _Files: `BattleState`, `AttackResolutionController`_

- [x] **Borderless graphics**  
       Intermittent graphics application when changing WindowMode. Investigate and fix if still reproducible.  
       _Files: `SettingsRowWindow`, `ValueControl`, input timing_

## M2 — Quest Spine (shipped, pending verification)

- [x] `QuestData` + `QuestLoader` (JSON catalog, duplicate-ID check)
- [x] `QuestLog` active/completed state, save `formatVersion` 3 (`questLog` payload)
- [x] Quest-gated world nodes (`isNodeUnlocked`) + locked node render tint
- [x] Quest journal UI (world-map menu → list → detail dialog)
- [x] Completion hooks (`PartyContext::completeQuestsForNode`) + completion toasts
- [ ] Manual verify in-game: New Game → journal shows 3 quests → win node 2 →
      node 6 unlocks → journal marks first_watch done → win node 6
- [ ] Manual verify save round-trips quest state (save, quit, load)

## M3 — Town Hub (shipped, pending verification)

- [x] `ItemDefinition.goldValue` authored on all starter + debug gear; sell = goldValue, buy = goldValue × 2
- [x] `ShopStock` + `assets/shop/stock.json` (gear-key stock table, silent-skip unresolvable keys)
- [x] Shop counter hub scene (Buy/Sell/Trade[disabled]/Leave) opening dedicated `ShopBuy`/`ShopSell` list windows; debug gear hidden from sell list
- [x] `ShopWindow` (price column, gold purse, per-line `ShopConfirm`, `refresh()`)
- [x] List navigation wraps at edges in shop / inventory / equip lists (incl. hold-to-repeat)
- [x] `NodeType::Town` + Riverwatch node 1 on the default world map + hub menu (Shop/Tavern/Leave) + town node color
- [x] `TavernData` + `assets/tavern/candidates.json` / `lore.json`
- [x] Tavern hub: quest board (accept tavern-offered quests), recruit (hire + party append, cap 6, hide hired), Talk (lore dialog)
- [x] Quest pickup: `offerredAtTavern` on quests; `QuestLog::accept`/`availableQuests`/`acceptAuto`; journal hides un-accepted tavern quests; `scout_the_east` tavern-offered
- [ ] Manual verify in-game: reach node 1 → hub → shop counter (buy + sell sides) with gold updates → tavern board picks up scout_the_east → hire a candidate → party grows → talk lore
- [ ] Manual verify: hub/tavern/shop **Leave** exits fully to the world map; difficulty comes from a fresh **New Game** (quest board shows "Scout the East" — the reset path must not pre-accept it)
- [ ] Manual verify: equipped gear shows yellow **E** in shop Sell and can't be sold; battle/deployment shows Liandra's name (not "elf archer"); save list shows date/time labels
- [ ] Manual verify shop/tavern state persists after save/load (roster + inventory + gold + questLog round-trip)
- [x] Identical gear stacks into one entry (e.g. two rings = `x2`); equip/unequip consumes/returns one unit (max stack 99)
- [x] Equip list shows fully-equipped items grayed-out with an `x0` (owned but 0 free copies); candidates sorted alphabetically; grayed rows stay focusable but Enter is a no-op
- [x] Shop sell list marks equipped gear with a yellow **E** and blocks selling it; dialog lines wrap instead of overflowing; battle uses recruited custom names; save slots labelled by date/time
- [ ] Deferred: when equipping, show otherwise-equipped candidates with a "currently equipped on <unit> — equip anyway?" confirm

## M4 — Progression (shipped, pending verification)

- [x] `SkillProgression` real active-only catalog (slash/fireball/heal/valor/weaken) with per-class recipe and descriptions
- [x] `RosterUnit` gains `baseClass` (from template), `skillPoints`, `SkillProgression`; recruit/new-game sets template class; load path restores persisted progression
- [x] Level-up flow: each battle victory still +1 level, now also +1 skill point; victory summary row "(level-ups grant +1 skill point)"
- [x] Battle sync: `Unit::importRosterProgression` copies points + learned/equipped skills onto template-rebuilt battle units; equipped actives grafted to battle skill ids
- [x] `SkillProgressionWindow` (window slot from unit-detail **Abilities**): Learn mode (spend points) + Equip mode (toggle active loadout, tab to switch); Esc returns to unit detail
- [x] Save **formatVersion 4**: campaign + battle saves persist `baseClass` / `skillPoints` / `skills`; v3 files load with defaults (template class, 0 points, nothing learned)
- [ ] Manual verify in-game: win a battle → victory overlay notes +1 SP → unit detail **Abilities** opens skills panel → learn a skill (points drop, cost row shows Learned) → Equip tab toggles loadout → next battle's Skills menu includes the newly learned skill
- [ ] Manual verify: save after spending → reload → points/skills intact; a pre-existing v3 save loads without error
- [ ] Manual verify: class recipe respected — e.g. Soldier cannot learn Heal; Equip mode has at most 4 actives; learned-but-unequipped skills stay known after load
- [ ] Deferred (M5): passive/reaction skill effects in combat, skill-level power scaling, promotion class tree

## M5 — Combat Depth (slices 1–4 shipped)

- [x] Status model: `StatusType` (Poison/Burn/Stun/Sleep) + `StatusInfo` registry in `battle/unit/StatusEffect.h`
- [x] `Unit::applyStatus` stacking/refresh rules + `tickStatuses` (DoT damage, duration countdown, expiry cleanup) + `StatusTickResult`
- [x] DoT floats green damage at turn start; death-by-DoT advances the queue; STUNNED/ASLEEP skips consume the turn (float + advance)
- [x] Sleep breaks only on real combat damage (`BattleSession::applyDamage`), not DoT ticks
- [x] Skill JSON optional `statuses` array parsed into `SkillData::statuses`; riders applied on landed hits + supportive casts (chance per target via `CombatSystem::chanceRoll`)
- [x] Skill previews append status tags (`BRN2x3(60%)`) - later moved out of the
      DMG/ACC preview into the confirm info block and the Tab description
- [x] Terrain: `CombatSystem::applyTileBonuses` (grass_atk +2 atk, grass_mag +3 mag, grass_eva +5 eva) wired into makeHitContext / EnemyAI / DamagePreview
- [x] Terrain tiles actually placed in `1v1_test.{tmx,tmj}` (lower arena patches)
- [x] Content: fireball burn rider; new `venom_strike` (PSN DoT) + `thunder_jolt` (stun) in skill catalog + progression recipes (Soldier/Duelist/Scout & Mage/Scout); debug_overlord smoke skills
- [x] UI: turn banner status tags; unit-detail Status line
- [x] Buffs/debuffs as statuses: `Barrier` (BRR), `AttackUp` (ATK+), `ArmorDown` (ARM-), `AttackDown` (ATK-) in `StatusEffect.h` + JSON type strings
- [x] `Unit::statusStatContribution` folded into `tempMod()`: buffs +25% base stat, curses −10% total stat (Barrier=>DEF+MDEF, AttackUp/AttackDown=>ATK, ArmorDown=>DEF+MDEF); auto-expire with the status tick
- [x] Content: valor→attack_up, weaken→attack_down (status-based); new `barrier` (Soldier/Mage) + `sunder` (Soldier/Duelist/Scout) in the catalog + progression recipes + debug_overlord smoke skills
- [x] `describeSkill` shows status tags for status-based buff/debuff skills (no phantom `ATK +0`); floaters color buffs blue `buff` burst vs curses purple `debuff`
- [x] Preview narrowed to DMG/ACC only (`describeSkill`); effects split out to
      `skill_effects::describeSkillDetails` (confirm info block) + Tab description;
      duplicate yellow preview removed from targeting and `updatePreview`
- [x] Tab (Details) in the Skills submenu -> highlighted skill description in the
      top battle text (`BattleMenuItem::detail`, `UIEventType::ActionChanged`,
      `ButtonMenuWindow::hasDetails/detailAt/setHintLine`)
- [ ] Manual verify in-game (slice 1): cast fireball → enemy burns (BRN floater + DoT ticks at their turn start) → stun with thunder_jolt → STUNNED skip → poison DoT kills → victory still resolves; terrain: stand an attacker on grass_atk / defender on grass_eva and see the prediction/damage shift
- [ ] Manual verify in-game (slice 2): cast Valor/Barrier on an ally → ATK+/BRR tags, preview & actual damage rise (+25% of base); cast Weaken/Sunder on an enemy → ATK-/ARM-, damage taken increases (−10% of total); statuses expire after the target's 3rd turn; re-cast refreshes duration; buffs don't stack damagePerTick
- [x] Slice 3 — skill power scaling: `SkillData.powerPerLevel` parsed + scaled in `makeHitContext` by the caster's `SkillProgression.levelOf` (enemies without progression fight at authored power); `powerPerLevel` added to slash/fireball/heal/venom_strike/thunder_jolt; damage/heal recipes raised to `maxLevel 3` (costs 1/1/2)
- [x] Slice 4 — boss battle + scripted waves: `EnemyDefinition.level` honoured (`UnitSpawn.level` → `growToLevel`); `BattleEvent.minTurn` + `once` wave gating (`BattleEventSystem::setTurn` fed by BattleState); new `boss_arena` map (6x5, 3 enemy/2 player spawns), Bandit Chief boss (KillBoss victory rule) + Raider reinforcements; boss node on the world map
- [ ] Manual verify in-game (slice 3): spend points to raise Slash/Fireball to Lv2/Lv3 → DMG preview and actual damage rise by `powerPerLevel` per level; unlearned/JSON skills unchanged
- [ ] Manual verify in-game (slice 4): enter node 8 boss arena → Bandit Chief with guards; a Raider spawns on the first enemy turn (dialogue floater); if the fight drags on a second wave of 2 Raiders spawns — but only once; killing the chief ends the battle in victory even with reinforcements alive
- [ ] Deferred (M5 later slices): enemy skill casting (AI uses skills, not just basic attacks), cursed terrain, further content statuses, skill cooldowns/MP balance pass, more maps

## World Map — Node Interactions

- [x] Node descriptions for battle nodes 2 (watchtower) and 6 (enemy main camp) — no more "Empty Node"
- [x] FFTA-style node event chooser (`WorldMapState::collectNodeEvents/presentNodeEvents/playNodeEvent`): on arrival every active quest whose objective targets the node becomes an event (VictoryAtNode → the node's battle; ReachNode → flavor dialog), plus a generic "Battle" when a battle node has no quest. One event auto-plays (battle confirm/dialog), multiple open a `WorldMapNodeEvents` ButtonMenuWindow
- [ ] Manual verify in-game: buy "Scout the Eastern Reach" at the tavern → reach node 0 → quest dialog plays; add a second victory quest sharing node 6 → arriving at 6 shows a chooser listing both quest battles + generic battle excluded when quests exist
- [x] Quest board "buy" flow: selecting a quest opens a detail window (client / description / objective / cost / reward) before the accept confirmation; accepting a quest with `cost > 0` spends gold (not-enough-gold is refused)
- [x] Town hub order + talk: Tavern, Shop, Talk, Quit — Talk plays `assets/town/talk.json` town gossip
- [x] Quests (journal) reachable from the initial world-map Back menu (`refreshWorldMapMenuItems` now used at open — previously "Quests" was missing there and only appeared after some flows)

## Tutorial Duel (shipped, pending verification)

- [x] New Game skips straight from the narrator into a tutorial duel
      (`IntroState` → `BattleState` with `BattleRequest.tutorial`, reusing
      `1v1_test.tmj`); victory or system-menu Quit goes to the world-map start
      node, a tutorial defeat retries the duel in place.
- [x] Tutorial duels use contextual one-line tips instead of the static
      controls block: welcome, first action-menu open, first Move/Attack/
      Skills/Defend/Wait. Each tip shows once per duel; non-tutorial battles
      keep the static controls dialog as a fallback.
- [x] The tutorial duel **starts already deployed**: the party is auto-placed
      on its spawn tile (facing the nearest enemy) and combat begins
      immediately, so the facing picker is never shown before the `Wait` tip
      explains facing. Real battles keep the full deployment phase.
- [x] Main menu **Continue** button (top, appears when any save exists) that
      instantly resumes the newest campaign save and auto-focuses; Load Game
      still browses the save list (pick older / delete with X).
- [ ] Manual verify in-game: New Game → narrator → tutorial duel; the player
      unit is already on its spawn tile and combat starts with no deployment
      step or facing picker → welcome tip → tips fire on the first Move/Attack/
      Defend/Wait; win → world map at the start node with a fresh save state.
- [ ] Manual verify: lose the tutorial → duel restarts automatically;
      system-menu Quit → skips to the world map (no retry loop trap).
- [ ] Manual verify: a non-tutorial battle (Continue on an old save, or the
      node-2 repeat fight) still shows the full static controls dialog on its
      first combat.
- [ ] Manual verify: with a save present the main menu reads
      Continue / New Game / Load Game / Options / Quit with keyboard focus on
      Continue; Continue drops into the world map exactly where the newest
      save left off; Load Game still opens the save list.
- [ ] Manual verify (debug KO): with the debug Overlord selected, target a
      full-HP enemy with Debug KO -> the preview reads `--- ACC 100%` (no damage
      number, accuracy shown because the KO is a rider that still rolls); the hit
      lands even against evasion 99 / defense 99;
      the skull is small and centred on the unit, fades in, holds, then rises and
      fades; the unit dies as the skull dissolves, with NO damage number floating;
      a splash/second target also dies; the battle result overlay waits for the
      effect and still resolves victory/defeat.
      _Files: `debug_oneshot.json`, `skull.png`, `CombatVfxLibrary`,
      `AttackResolutionController`, `BattleSession::applyKo`_
- [ ] Manual verify (preview is DMG/ACC only): a damage skill reads `DMG 12`;
      a heal/buff/debuff/KO reads `---` with the ACC bar and percentage still
      shown; the box sits just below the vertical centre of the screen and does
      not cover a unit standing in the middle of the view; the ACC bar is filled
      proportionally (green >=70%, amber >=40%, red below) and its percentage
      matches the bar; the preview box is the ONLY place those numbers appear.
- [ ] Manual verify (two-step, popup-free cast): choosing a skill opens the
      reachable tiles and shows NO damage/accuracy while the cursor moves;
      Accept locks the tile and the preview box appears; the cursor snaps onto
      the focused unit and the splash highlight does not move; Q/E cycle the
      preview (and the cursor) through every unit in the splash including allies
      and the caster; Esc returns the cursor to the blast centre;
      Accept casts; Esc unlocks the tile, hides the preview and lets a different
      tile be chosen; a Tab skill description (top text at y=72) does not
      collide with the preview box; shop/tavern/quest confirms still show their
      normal centered prompt box.
- [ ] Manual verify (refused-input feedback): every rejected input stays in its
      current phase and plays a short **red border vignette** + small shake
      instead of silently ignoring the press — an unreachable move tile, an
      attack/skill tile out of range, a supportive cast aimed at a hostile, a
      hostile cast aimed at an ally, and a single-target/AoE cast that catches
      no legal unit. A held Accept must not re-fire after a refusal. The red
      hugs the screen edge and fades to nothing about 5% of the short edge
      inward (no hard banding steps); the middle of the screen is untouched; it
      lasts ~0.18s at a deliberately faint peak alpha (~0.30) with only a ~1.5px
      shake — a hint, not a jolt.
- [ ] Manual verify (no turn banner): nothing is drawn at the top-center of the
      battle screen naming the acting unit (or a round number); the top area is
      free for the Tab skill description.
- [ ] Manual verify (tutorial dialog does not leak its Accept press): with the
      tutorial on, choosing Wait and dismissing the tip leaves the facing
      picker untouched - the player's own Accept is required to end the turn.
      Dismissing the ActionMenu tip does not also select a menu row.
- [ ] Manual verify (Tab details): in the Skills submenu, Tab shows the
      highlighted skill's description centered at the top; Tab again hides it;
      moving to another row swaps the text to that skill (no stale description);
      Esc back into the submenu does not leave a description behind; rows with
      stat/status riders show them appended; the `Tab: Details` footer hint shows
      only when at least one row has a description.
- [ ] Manual verify (tutorial tip is solo): with the first tutorial tip open, no
      other UI is visible behind it (in particular the unit-detail portrait if it
      was opened during deployment); story dialogue still shows the scene.
- [ ] Manual verify (basic attack is a skill): the action menu's "Attack"
      button opens normal target selection; the damage preview and the rolled
      damage match each other; it costs no MP and the unit's turn ends as an
      Attack; Esc from targeting returns to the action menu (not the Skills
      submenu) while Esc from a learned skill still returns to Skills; the
      basic attack never appears in the Skills submenu; a jump-2 unit can hit
      a tile one elevation up/down and a jump-1 unit cannot; an `atkRange > 1`
      unit reaches its full range from the human side too.
      _Files: `basic_attack.json`, `SkillLoader`, `BattleMenuController`,
      `AttackResolutionController`, `EnemyAI`, `DamagePreview`_

## Verification & Regression

- [ ] **Full smoke test** (uninterrupted run): Boot → Main Menu → World Map → Deployment → Combat → Attack → Victory → Return
- [ ] **Camera regression test**: windowed, borderless, deployment, combat, cursor at all map corners, clamping
- [ ] **Button menu verification**: marker positioning, width sizing, alignment across action/skill/system/inspect menus

## Render Pass Integration (refactor/renderer-passes branch)

- [x] Update `BattleState::render()` to use `beginLogicalPass()` / `endLogicalPass()`
- [x] Split world/native effects passes: `DamagePreview` -> native pass, `CombatAnimationSystem::render` + `DebugRenderer::flush` -> world pass
- [ ] Confirm no visual regressions (text crisp, correct coordinate spaces)

## Combat Visuals (engine animation wiring)

- [x] `CombatAnimationSystem` effect expiry driven by a per-effect engine `Animator` (`TimedState` "effect" → exit-time "done"); placeholder expanding-ring render on the target tile / scene center
- [x] `FloatingTextSystem` renders damage/MISS with an eased `Tween` rise + fade over the target tile
- [x] Wire per-target `hit_impact`/`dodge` and arena-wide `*_cast_once` into `AttackResolutionController`
- [ ] Replace placeholder rings/balloons with real sprite/sheet VFX

## Gameplay & Content

- [ ] **DialogSystem** - [x] Wrapper/controller implemented (`DialogueController`) with active-state API - [x] Input blocking while active (via UIManager blocking windows and `DialogWindow`) - [ ] Add sample pre-battle scripted dialogue sequence usage
- [x] **SkillLoader** - [x] Loader implemented (`load`, `loadAll`, duplicate ID checks) - [x] At least 2 skill JSONs present (`slash.json`, `fireball.json`) - [x] Selected skill is wired into combat targeting/resolution flow
- [x] **JSON schema documentation** - [x] Document map, unit, and skill JSON structures

## Polish

- [ ] Sprites and animations for units
- [ ] Menu / combat SFX and background music
- [ ] Impact effects (screen shake, highlights)

## Technical Goals

- [ ] Keep one map fully playable at all times
- [ ] Maintain zero-warning build where practical
- [ ] Compile after every architectural change
