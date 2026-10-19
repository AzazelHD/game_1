# TRPG Game — TODO

> Milestone records (M2-M5, tutorial duel): `archive/MILESTONES.md`. Shipped-and-dated
> work: `CHANGELOG.md`. Architecture: `ARCHITECTURE.md`. Designs: `design/`.

## State of play (2026-10-19)

- **Build**: Game DEV green (`tools\build_debug.bat`); engine consumed from `../engine/build`.
- **Battle camera**: knobs in `game/src/config/UiTuning.h` - `kDeadZoneFractionX` 0.55,
  `kDeadZoneFractionY` 0.75, `kMarkerHeadroom` = `Cursor::kHoverOffset`. Mapped in `TUNING.md`.
- **Open**: the manual checks under "Verification & Regression" below, the unchecked items in
  `archive/MILESTONES.md`, and the UX backlog. Forward work lives in `ROADMAP.md`.

## Bugs & regressions

_All entries below are fixed and kept for history; add new ones at the top._

- [x] **Camera: cursor lost at top tile / uneven top-bottom gap**: the camera
      reserved `hover + triH` (70) above the floor although the marker only
      reaches `hover` (50), and nothing below. The footprint is now padded by
      `Tuning::Camera::kMarkerHeadroom` above and the tile-art overhang below;
      dead-zone X/Y fractions are independent. All knobs in `config/UiTuning.h`,
      mapped in `TUNING.md`. _Files: `BattleCameraController`, `BattleState`,
      `config/UiTuning.h`, `BattleLayout`._ Retuned 2026-10-19 (X 0.55 / Y 0.75,
      marker headroom tied to `Cursor::kHoverOffset`); still to verify on node 7
      that the top and bottom row gaps look even.

- [x] **Movement cost**  
       Movement points are spent based on Manhattan distance instead of actual path length from pathfinder.  
       _Files: `battle/map/MovementRange`, `battle/map/Pathfinder`_

- [x] **Cursor position after combat**  
       Cursor remains on target tile after attack/spell. Add cursor to `AttackResolutionContext` and reset to active unit when battle menu reopens.  
       _Files: `BattleState`, `AttackResolutionController`_

- [x] **Borderless graphics**  
       Intermittent graphics application when changing WindowMode. Investigate and fix if still reproducible.  
       _Files: `SettingsRowWindow`, `ValueControl`, input timing_

## World Map — Node Interactions

- [x] Node descriptions for battle nodes 2 (watchtower) and 6 (enemy main camp) — no more "Empty Node"
- [x] FFTA-style node event chooser (`WorldMapState::collectNodeEvents/presentNodeEvents/playNodeEvent`): on arrival every active quest whose objective targets the node becomes an event (VictoryAtNode → the node's battle; ReachNode → flavor dialog), plus a generic "Battle" when a battle node has no quest. One event auto-plays (battle confirm/dialog), multiple open a `WorldMapNodeEvents` ButtonMenuWindow
- [ ] Manual verify in-game: buy "Scout the Eastern Reach" at the tavern → reach node 0 → quest dialog plays; add a second victory quest sharing node 6 → arriving at 6 shows a chooser listing both quest battles + generic battle excluded when quests exist
- [x] Quest board "buy" flow: selecting a quest opens a detail window (client / description / objective / cost / reward) before the accept confirmation; accepting a quest with `cost > 0` spends gold (not-enough-gold is refused)
- [x] Town hub order + talk: Tavern, Shop, Talk, Quit — Talk plays `assets/town/talk.json` town gossip
- [x] Quests (journal) reachable from the initial world-map Back menu (`refreshWorldMapMenuItems` now used at open — previously "Quests" was missing there and only appeared after some flows)

## UX Backlog (from manual QA)

- [x] **Fresh turn opens the action menu**: a player unit enters its turn with
      the cursor already on it and its menu open — no Enter-to-select step;
      Esc drops to the free cursor, Enter on the unit reopens it. _Files:
      `BattleState`._
- [x] **Undo Wait (Esc)**: the facing picker backs out to the action menu with
      Esc while the unit can still act (move left or action unused); only the
      facing preview is rolled back — the move already made is kept (the unit
      stays on its tile) — so you're free to attack/skill/move again. An
      exhausted turn auto-fires Wait with no one-item menu and treats Esc as
      confirm. _Files: `BattleMenuController`, `HumanTurnController`,
      `BattleState`._
- [x] **Battle camera — edge-scroll (replaces follow-cam)**: _superseded in
      part by the **dead zone** entry below (box size, test on the walkable
      surface, no focus glide); kept for the fitted-`Slide` and
      drive-from-both-phases history._ The camera stays
      put while the cursor roams the central comfort box; it scrolls only when
      the cursor's **tile** leaves that box, just far enough to bring the whole
      tile back inside, and stops as soon as the box is satisfied. The box is
      `max(15% of the viewport, map-bounds margin + 64px)`, capped to half the
      viewport per side (it was briefly also capped to 70% of the board to force
      a fitting board to move — dropped again by the "holds still on a board that
      fits" entry below): `clampToBounds()` never lets
      the cursor closer to the edge than the margin, so a plain 15% band was
      *unreachable* vertically (108px of height at 720p vs the 112px margin) and
      the vertical axis never scrolled. The test measures the tile's
      whole on-screen **footprint**, not its anchor: `tileToScreen()` returns the
      diamond's *top* corner and a tile reaches a full tile height below it (in
      iso the tile height *is* the y offset), so the anchor-only test let the
      lower half of the tile hang outside the box first. `Camera::SmallMapFit::
      Slide` lets a map that fits the play field sit anywhere inside it instead
      of being pinned centred — that is the slack the camera pans across, and the
      unpadded map bound still keeps the map itself on screen; the initial
      framing and resize clamp keep the default `Center`. It never follows the
      cursor or the active unit continuously; a single focus glide runs at the
      start of each turn (and on returning from a resolution) so the acting unit
      is framed. The camera is driven by `BattleState::updateCamera()` from both
      the deployment branch and the combat path — `update()` returns early for
      deployment, above the combat drive, so deployment has to call it
      explicitly or nothing moves the camera during placement. _Files:
      `scenes/BattleState`, `battle/systems/BattleCameraController`;
      engine `renderer/Camera` (`getMapBoundsMargin`, `SmallMapFit::Slide`)._
      `BattleLayout` picks the art scale with `floor` (the largest whole multiple
      at or below the 720p baseline ratio), so tiles are never drawn *larger*
      than the baseline; rounding to the nearest multiple made 1600×900 jump to
      3× and read as a zoom-in on the battlefield.
      _File: `renderer/BattleLayout`._
- [x] **Battle camera — load framing + eased pan, and one-shot tips that stay
      gone**: three follow-ups reported from play.
      1. The battle faded in on the clamped map origin and *then* jumped to the
         cursor, which read as a blink (worst on a big map, where the two are far
         apart). The camera drive lives in `update()`, which returns early while
         the fade-in transition plays, so the first framing only happened after
         control started. `BattleState::onEnter()` now calls
         `BattleCameraController::focusImmediately()` just before starting the
         transition, hard-cutting to the same tile the free-control glide would
         frame (the acting unit when there is one, the cursor otherwise), so the
         scene fades in already framed.
      2. The edge-scroll cut straight to the fixing position — one jump per tile
         step. `scrollCursorIntoBand()` now *eases* towards it (τ = 0.15s, ≈90%
         of the way in 0.35s) with the delta recomputed from the live offset every
         frame, so the camera settles on the box edge and stops: smooth pan, no
         overshoot, no drift past the box.
      3. The two first-time tip flags lived only in `PartyContext` and were
         deliberately "reset per new game; not persisted to saves", so every load
         replayed the world-map controls dialog and the first-combat controls
         block. `controlsTutorialShown` / `battleControlsTutorialShown` are now
         part of the campaign save (both the campaign and the battle payload) and
         are restored by `PartyContext::applyCampaign()`. Absent keys read as
         `true`: a legacy save is a campaign in progress and the replay is the bug
         being fixed, while `resetForNewGame()` still seeds `false` so a fresh
         game teaches the controls once. _Files: `scenes/BattleState`,
         `battle/systems/BattleCameraController`, `systems/PartyContext`,
         `data/SaveManager`, `scenes/WorldMapState`._
- [x] **Battle camera — holds still on a board that fits the play field**: the
      comfort box used to be capped to 70% of the board specifically so a small
      map would still leave the box and make the camera drift; that is the
      opposite of what a small map wants. A board that fits inside the play field
      is entirely visible at once — every tile, the cursor, every unit on it — so
      `BattleCameraController::update()` now returns before any camera motion when
      both board axes fit: no focus glide, no resolution cut, no edge scroll, and
      `focusImmediately()` leaves the scene's centred clamp alone. Only an axis
      the board **overflows** can carry something off screen, so only such an axis
      is ever panned (`scrollCursorIntoBand` gates each axis independently, which
      also stops the vertical drift on a board that only overflows horizontally).
      The 70%-of-board cap is gone with it. Amended by *map art bounds* below:
      "fits" now measures the map's **drawn** height, not the tile-anchor box, so
      a board whose top rows are drawn above the play field no longer counts as
      fitting. _File: `battle/systems/BattleCameraController`._
- [x] **Battle camera — dead zone (spec rewrite of the comfort box)**: the
      cursor box is now an explicit **camera dead zone**: a centred rectangle
      covering 80% of the play field (10% margin on each side), replacing the
      old `max(15% of the viewport, map-bounds margin + 64px)` band. While the
      cursor stays inside it the camera never moves; the moment the cursor
      leaves, the camera pans by **exactly the minimum displacement** that
      brings it back — one tile over the line moves the camera one tile's worth,
      a diagonal step corrects both axes, a jump far outside corrects the whole
      gap and nothing more. No follow-cam and no automatic recentering: the
      one-time turn-start focus glide is gone (`m_turnSerial`,
      `anchorTile`/`turnSerial` in the controller `Context`, `centeredOn()` and
      the free-focus bookkeeping all removed with it) — the cursor is already
      reset onto the acting unit at turn start, so the dead zone frames it only
      when it actually needs framing. The test now runs on the tile's
      **walkable floor surface**, not its grid anchor: height layers are Y
      offsets, so an elevated tile's floor renders `height * halfTileHeight`
      above its base position, and `floorFootprint()` mirrors the diamond
      `BattleRenderer::drawCursorTicks` draws (anchor lifted by the tile
      height). A raised tile near the top of the dead zone therefore still pulls
      the camera up to it — the correction accounts for the real elevation
      rather than a fixed offset. Per-axis panning is unchanged: an axis the
      board **fits** on never pans (no map content exists beyond the play field
      there), so a board that fits the play field entirely never moves, exactly
      as the spec requires. `focusImmediately()` on scene entry now applies the
      same minimum correction in one step instead of hard-cutting to centre, so
      a comfortable board keeps the scene's centred clamp. Pan is still eased
      (τ = 0.15s) toward a target recomputed every frame from the live cursor
      tile, so the motion is deterministic and settles on the dead-zone edge
      without jitter. _Files: `battle/systems/BattleCameraController`,
      `scenes/BattleState` (`makeCameraContext()` now fills `cursorTileHeight`
      from `BattleMap::at().height`)._
- [x] **Battle camera — restore the pre-beat framing after a scripted beat**:
      a scripted beat (dialogue `glideTo`, cast `glideTo`, hit `snapTo`) takes
      the camera **away** from the tactical framing to centre it on a unit. When
      it ends the cursor usually still sits comfortably inside the dead zone of
      that parked position (it is adjacent to the beat's subject), so the dead
      zone alone left the view **stuck centred on the beat's subject** — the
      reported node-7 "camera is centred on one enemy, somehow". The controller
      now remembers the offset a beat started from (`m_beatOffset`, captured on
      the Free→beat transition) and eases back to it when control returns
      (`m_returningToBeat`), so the camera hands back where the player was
      looking. The target is re-aimed by the dead zone for the **live** cursor
      every frame, so a cursor moved while a return is in flight still lands on
      a valid framing; because the cursor is frozen for the whole duration of a
      beat (dialogue, `ResolvingHits` and `EnemyAction` all block tactical
      input), the target is in practice the pre-beat offset exactly — this is
      undoing the beat's excursion, not a recentering. Chained enemy turns are
      **one** continuous beat (the phase never returns to `Free` between them,
      since `finishEnemyAction()` → `advanceToNextUnit()` → `processCurrentTurn()`
      re-sets `EnemyAction` synchronously), so a whole enemy phase is a single
      excursion with a single return — no back-and-forth between enemies. A
      return in flight interrupted by a new beat keeps the **original** tactical
      offset (the `!m_returningToBeat` capture guard) rather than re-capturing a
      transient mid-ease position. The return settles on arrival (0.5px) **or**
      when the map bounds pin the camera short of the target (detected by the
      clamp rejecting the eased position), so a pinned camera can never lock the
      dead zone out in a no-op easing loop. _File:
      `battle/systems/BattleCameraController`._
- [x] **Enemy turn deadlock on a refused move commit**: `beginUnitWalk()`
      returned **without invoking `onComplete`** when
      `MovementCommitSystem::commitMovement()` was invalid (out-of-grid
      destination, or the re-path came back empty — both reachable from
      `EnemyAI::planTurn()`'s destination, which is only checked against
      occupancy, not re-pathability). For the enemy path `onComplete` is the
      turn state machine's **only** resume point — it calls `resolveEnemyAttack()`
      → `finishEnemyAction()` — and unlike the player path
      (`HumanTurnController.cpp`, which pre-checks `reachableCosts` before
      calling) the enemy path has no guard at all. A failed commit therefore
      parked `TurnState::ProcessingTurn` forever with `m_pendingResolution`
      stuck at `EnemyAction`: the battle deadlocked **and**
      `BattleState::updateCamera()` kept `m_cameraPhase` on
      `ResolutionCast`/`ResolutionTrack`, which glues the camera to that enemy
      permanently — the exact "camera centred on one enemy, somehow" symptom,
      and the harder of the two causes to spot since the camera is a side
      effect. `onComplete` now fires on **both** early-return paths (null unit,
      invalid commit); the unit simply attacks / reopens its menu from where it
      already stands. _Files: `scenes/BattleState` (`beginUnitWalk`)._
- [x] **Battle world — constant size relative to the window**: `artScale` is
      `floor(outH / 360)`, so a window whose height is not a multiple of 360 (e.g.
      1600×900 → artScale 2 over a 25% taller window) showed 25% more world and
      drew tiles 20% smaller *relative to the window* than 720p/1080p do. 
      `BattleLayout` now also computes `worldScale = outH / 360` (the exact ratio)
      and `BattleState::applyWorldZoom()` folds `worldScale / artScale` into the
      battle camera's zoom, so tiles occupy the same fraction of the window at
      every resolution — 900p draws each tile 1.25× a 720p one, matching its 1.25×
      larger window, and the visible slice of map is constant across resolutions.
      artScale stays integral for the world buffer and the sizes baked into it.
      Re-applied on window resize. _Files: `renderer/BattleLayout`,
      `scenes/BattleState` (`m_worldZoom`, `applyWorldZoom`)._
- [x] **World-map menu wraps around**: `focusPrevious/Next` on the in-game Back
      menu now cycles past the ends (first row + Up → last row) instead of
      stopping. The menu only disables scrolling (`setScrollable(false)`, panel
      grows to fit every row) and keeps `ButtonMenuWindow`'s default wrapping.
      _File: `scenes/WorldMapState`._
- [x] **Quit moved to the title screen**: the in-game (world-map) Back menu's
      "Quit" — which killed the application outright — is now **"Title Screen"**
      (`ActionId::ReturnToTitle`): it leaves the campaign and replaces the scene
      with a fade-in `MainMenuState`, so quitting the app is only ever done from
      the title screen (whose own Quit is unchanged). Unsaved progress is
      abandoned, exactly as the old in-game Quit did. _Files: `ui/ActionId`,
      `scenes/WorldMapState`, `scenes/MainMenuState` (unchanged Quit path)._
- [x] **Battle camera — map art bounds (raised tiles)**: the camera's bounds only
      knew the tile *anchors* (the iso grid), but a tile is drawn with its
      surface half a tile height **above** its anchor and each height layer
      (`h2`/`h3`/`h4`, `offsety = -height * 12`) lifts it further:
      `1v1_test`/`boss_arena` draw up to 48 world units (≈130px) above the top
      anchor row, `test_map` 36 (≈94px). Two symptoms: an elevated top row sat
      outside the play field on the large map (the clamp believed the anchor box
      was the whole map), and a fitting map was centred on its anchors, so the
      art looked pushed up with more empty space below than above. The loader now
      reports the art reach above the top anchors (`BattleLoader::computeMapTopExtra`
      → `BattleLoadResult::mapTopExtra`) and the scene hands it to the new
      `Camera::setMapBoundsTopExtra()`, which expands the top bound (world units,
      zoom-independent). The scene clamps with the new `Camera::SmallMapFit::Frame`
      (a map that fits *on its own* is framed centred, ignoring the sprite
      margin, so the clamp can never slide the art out of the view), and
      `BattleCameraController::boardFit()` adds the same reach to the vertical
      extent so the "fits" gate and the engine agree. Result: the tutorial/boss
      maps are now centred on their art (133/133px above/below at 720p instead of
      68/198), and node 7's whole map — top row included — is inside the play
      field (its art is 276 world units tall against a 277-unit play field at
      720p, so it fits with 1px to spare at every resolution; the vertical axis
      has nothing left to scroll). _Files: `scenes/BattleLoader`,
      `scenes/BattleState` (`m_camera.setMapBoundsTopExtra`,
      `SmallMapFit::Frame`), `battle/systems/BattleCameraController`; engine
      `renderer/Camera` (`setMapBoundsTopExtra`, `SmallMapFit::Frame`)._
- [x] **Battle camera — edge/corner framing**: the clamp box was built from
      tile origins only, so an edge/corner unit hung off-screen; bounds now
      cover the true diamond extents and the margin is applied in screen pixels
      for sprite overhang. _Files (engine): `renderer/Camera`; `BattleState`._
- [x] **Pixel-snapped world render**: the render-pass camera offset was
      fractional (continuously eased), landing tiles/sprites between pixels —
      seams through the tile grid and sprite shimmer while the camera moved.
      `BattleState::buildInterpolatedCamera` now snaps `offset * z` to whole
      pixels so every anchor lands on a whole logical pixel. _Files:
      `BattleState`._
- [x] **Crisp world at any resolution / framed layout M1**: the world renders
      at native resolution at an integer art scale inside the play field
      (blitted 1:1), so the NEAREST uneven-texel shimmer at non-integer window
      ratios is gone, and permanent left/right rails bracket the battlefield.
      `BattleLayout::artScale` picks the whole device-px-per-texel zoom from the
      window height; rail width is ~18% of the window, clamped 200–340px.
      _Files: `renderer/BattleLayout`, `BattleState` (`renderRails`,
      `applyBattleLayout`), `renderer/BattleRenderer`, `scenes/BattleLoader`._
- [x] **Framed layout — themed backdrop (Tier 0/1)**: the map's sky now spans
      the whole window and the battlefield floats on it; the rails are a
      vignette + hairline frame over that sky rather than opaque panels. Colors
      derive from `BattleDefinition`, so it is per-map themed with nothing to
      author. The engine world pass exposes a backdrop seam
      (`endWorldPass`/`blitWorldBuffer`) that Tier 2 parallax slots into.
      _Files: `BattleState` (`renderBackdrop`/`renderRails`), engine
      `renderer/Renderer`._
- [ ] **Framed layout — M2 (rail content)**: left rail = actor card (portrait,
      HP/MP, statuses, selected action); right rail = turn queue, swapping to a
      target card while aiming/resolving (predicted DMG/ACC) and reading as a
      brief duel on the strike. Reuse `UnitPanelWindow`
      (`setSingle`/`setDuel`) and move the battle unit panels off the window
      corners into the rails. _Files: `battle/ui/*`, `BattleState`
      (`syncUnitPanelWindow`)._
- [ ] **Framed layout — M3 (native overlays)**: keep tile-anchored overlays
      (damage preview, floating text, VFX) aligned with the play field now that
      the world pass no longer shares the design canvas. _Files:
      `ui/DamagePreview`, `battle/systems/CombatAnimationSystem`,
      `ui/FloatingTextSystem`._
- [ ] **Framed layout — M4/M5 (polish + UI migration)**: Tier 2 parallax layers
      (drifting clouds/embers/snow per theme — drop into `renderBackdrop`),
      ornamental frame art, queue→target→duel transitions, rail show/hide with
      dialogs; migrate menus/dialogs to a native, edge-anchored layout and make
      the UI crisp (it still upscales 1280×720 by the window ratio). _See
      `design/FRAMED_LAYOUT.md`._
- [ ] **Manual verify (framed layout M1)**: windowed at 1600×900 — rails
      visible left/right, world crisp while panning (no shimmer), any art
      scale 1×–3×, deployment + attack preview aligned; resize the window and
      confirm the layout rebuilds (rails stay, camera re-clamps, no crash);
      check 1280×720 and 1920×1080.
- [ ] **Journal submenu**: "Quests" should open a tabbed panel (Active /
      Completed tabs for now) instead of one flat list, so more tabs can be
      added later (e.g. Bestiary, Party).
- [x] **Deployment cycling (FFTA model)**: the portrait is driven only by
      Q/E cycling (or by the held unit); hovering never changes it. Enter acts
      on the portrait unit — unplaced → grab it in hand, placed → pick it back
      up; the held unit follows the cursor with a pulsing ghost; Esc drops it
      back to the roster pool; placing auto-selects the next unplaced unit.
- [x] **Exp-driven leveling + XP display**: `RosterUnit::exp` now drives
      levels on a linear curve (`expNeededForLevel`, 100 * level) instead of a
      flat +1/win; crossing the threshold levels up and grants +1 SP with
      surplus carried. Level badge + thin green EXP bar in the portrait; the
      unit-detail identity header shows an EXP label + bar. (Wipes stale saves.)
- [x] **LVL1 display bug**: deployment preview and `UnitDetailWindow` roster
      constructors now `growToLevel()` + `importRosterProgression()` so they
      show the real roster level/stats instead of a template-fresh unit.
- [x] **Battle camera — edge-scroll**: the camera stays stable during normal
      cursor navigation and scrolls only when the cursor nears the viewport
      edge (see UX Backlog), replacing the earlier centre-on-active-unit follow
      request. A one-time glide frames the acting unit at each turn start.
- [ ] **Battle camera — target framing**: on attack/skill confirm, center the
      target unit; for AoE, center the midpoint of all target units.
- [ ] **Battle camera — cast vs effect**: cast animation centers the caster,
      effect animation centers the target(s); per-skill flag decides whether
      the effect animation repeats once per target (e.g. heal) or plays once on
      the blast tile with per-target floating damage (e.g. fire).

## Verification & Regression

- [x] **Full smoke test** (uninterrupted run, verified 2026-10-15): Boot → Main Menu → World Map → Deployment → Combat → Attack → Victory → Return
- [ ] **Turn-start menu verify**: each fresh player turn opens the action menu with
      the cursor on the unit; Esc drops to the free cursor; Enter on the unit
      reopens the menu; a key held from the previous turn does not auto-fire a row.
- [ ] **Undo Wait verify**: move → Wait → picker → Esc reopens the menu and the
      unit **stays on its moved tile** (move kept) with attack/skill/move still
      available; the facing preview is rolled back; max-move + attack → menu
      skips straight to the facing picker and Esc there ends the turn.
- [ ] **Camera dead-zone verify (large map, node 7)**: in windowed and
      borderless, during deployment and combat, the whole map must sit inside the
      play field with no raised top row cut off — the vertical extent counts the
      art above the tile anchors, and node 7's art is as tall as the play field at
      its default zoom, so expect it framed **centred and still** with the top and
      bottom rows 1–2px from the rails; only the horizontal axis overflows and
      pans. Walking the cursor across the central tiles (inside the central 80%
      dead zone — 10% margin per side) leaves the camera perfectly still. Past the
      left/right dead-zone edge it pans by **exactly** the minimum needed: one tile
      over the line moves it one tile's worth, a diagonal step corrects both axes,
      a multi-tile jump corrects the whole gap and no more; it stops the instant
      the cursor is back inside and never re-centres on its own. Small maps
      (`1v1_test`, `boss_arena`) must be perfectly symmetric — an equal gap above
      the tallest drawn tile and below the lowest — and never move at all. The
      test must use the cursor tile's **walkable floor surface**: an elevated tile
      (h2/h3/h4) renders its floor `height * halfTileHeight` above the grid
      anchor, so a raised tile just above the dead zone must still pull the camera
      up to it (a base-anchor-only test misses this). It never reveals space
      outside the map; deployment behaves exactly like combat and its cursor never
      leaves the play field (deployment used to have no camera driver at all);
      the turn-start focus glide is gone — a new turn frames the acting unit only
      if the dead zone requires it. Also confirm the world scale did **not**
      jump at 1600×900 (tiles must be the same size per source texel as 720p,
      not 1.5× bigger).
- [ ] **Camera beat-return verify (node 7)**: after an enemy attacks — and after
      the player's own attack resolves — the camera must **ease back to the
      framing it was using before** that attack's cast/hit beat, not stay parked
      centred on the enemy it just snapped to (that was the reported node-7
      bug). During a whole enemy phase the camera stays on the acting enemies
      for the entire chain and returns **once**, at the end — no
      back-and-forth between consecutive enemies. An enemy whose move commit is
      refused must still close its turn out (attack from where it stood, then
      the next unit) instead of deadlocking the battle with the camera glued to
      it. Small maps (tutorial, boss arena) never move at all — the beat-return
      is gated by the same per-axis `boardFit()` as the dead zone.
- [ ] **Board framing at a glance**: if node 7 (10×10) reads as too tight
      vertically now that its full art is counted (rows flush with the rails, and
      a unit standing on the top/bottom row would have its sprite clipped by the
      edge), the lever is the per-battle `defaultZoom` in `config/BattleCatalog`
      — 1.30 → ~1.10 leaves ~55px of headroom above and below; lowering it also
      shows more of the map horizontally.
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
