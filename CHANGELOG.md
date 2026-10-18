# Changelog

All notable completed milestones for the TRPG game project.

Each entry is keyed to its **commit**, ordered **newest first** by commit date.
`git show <hash>` for details.

---

## `957114b` — 2026-10-18 — "Camera bounds count the map's drawn art (raised tiles)"

- **The camera no longer cuts off raised map rows, and centres a fitting map on
  what is actually drawn.** The camera's bounds are built from tile *anchors*,
  but a tile is drawn with its surface half a tile height above its anchor and
  every height layer (`h2`/`h3`/`h4`, `offsety = -height * 12`) lifts it further
  — `1v1_test`/`boss_arena` draw up to 48 world units (~130px at 720p) above the
  top anchor row, `test_map` 36 (~94px). Two visible symptoms: on the large map
  an elevated top row sat outside the play field (the clamp believed the anchor
  box was the whole map — walk the cursor up there and it was simply gone), and
  a fitting map was centred on its anchor box, so the art looked pushed up with
  more empty space below than above. `BattleLoader::computeMapTopExtra()` now
  measures the art reach above the top anchors from the tile layers (gids +
  layer Y offsets + the renderer's sprite-cell top) and returns it as
  `BattleLoadResult::mapTopExtra`; `BattleState` hands it to the new engine
  `Camera::setMapBoundsTopExtra()` (world units, zoom-independent) and clamps
  with the new `Camera::SmallMapFit::Frame`. `Frame` frames a map that fits
  *on its own* centred, ignoring the sprite margin — a fitting axis has no slack
  to pan, so honoring the margin there would only let the clamp slide the art out
  of the view. `BattleCameraController::boardFit()` adds the same reach to its
  vertical extent, so the controller's "fits → hold still" gate and the engine
  agree. Measured at 720p: `1v1_test`/`boss_arena` are now 133/133px and
  135/135px above/below (were 68/198 and asymmetric), and node 7's whole map —
  top row included, nothing cut — sits inside the play field (276 world units of
  art against a 277-unit play field, at every resolution, so its vertical axis
  has nothing left to scroll). _Files: `scenes/BattleLoader`,
  `scenes/BattleState`, `battle/systems/BattleCameraController`; engine
  `renderer/Camera` (`01a623b`)._

## `5fcaa7c` — 2026-10-18 — "Constant tile size, a still camera on fitting boards, wrapping world-map menu, Title Screen"

- **Tiles now keep the same size relative to the window at every resolution.**
  `artScale` is `floor(outH / 360)`, so at 1600×900 (not a multiple of 360) the
  world was drawn at the 720p ratio over a 25% taller window: tiles came out ~20%
  smaller *relative to the window* and 25% more map was visible, so the
  battlefield quietly changed size between resolutions. `BattleLayout` now also
  computes `worldScale = outH / 360` (the exact ratio), and
  `BattleState::applyWorldZoom()` folds `worldScale / artScale` into the battle
  camera's zoom. A 900p window draws each tile 1.25× a 720p one — matching its
  1.25× larger window — and the visible slice of map is now constant across
  720p/900p/1080p/1440p. `artScale` stays integral, so the world buffer and the
  sizes baked into it (tile quads, sprite boxes) are unchanged and crisp; the
  compensation is re-derived on window resize. _Files: `renderer/BattleLayout`
  (`worldScale`), `scenes/BattleState` (`m_worldZoom`, `applyWorldZoom`)._
- **The camera holds perfectly still on a board that fits the play field.**
  `BattleCameraController::update()` used to be able to drift a small board around
  (the comfort box was capped to 70% of the board precisely so a fitting board
  would still leave it) and, with `SmallMapFit::Slide`, the clamp allowed it. But a
  board that fits is entirely visible at once — every tile, the cursor, and every
  unit standing on it — so nothing the cursor does can take anything off screen.
  `update()` now returns before *any* camera motion when both board axes fit (no
  focus glide, no resolution cut, no edge scroll), `focusImmediately()` leaves the
  scene's centred clamp alone so the map fades in composed, and
  `scrollCursorIntoBand()` gates each axis independently: only an axis the board
  **overflows** is ever panned. The 70%-of-board band cap is gone with it. Side
  effect worth knowing: on a board that overflows horizontally but fits vertically
  (the 10×10 at 720p/900p/1080p) the camera now pans horizontally only — the
  vertical axis has nothing that can leave the view. _File:
  `battle/systems/BattleCameraController`._
- **The world-map menu wraps around again.** The in-game Back menu had
  `setWrappable(false)`, so focus stopped dead at the first/last row; it now keeps
  `ButtonMenuWindow`'s default wrapping (Up on the first row → the last row). It
  still only disables scrolling (`setScrollable(false)`), so the panel grows to fit
  every row. _File: `scenes/WorldMapState`; docs: `AGENTS.md` §3._
- **Quitting the application moved to the title screen.** The in-game Back menu's
  "Quit" killed the app outright, with no way back to the title from a running
  campaign. It is now **"Title Screen"** (new `ActionId::ReturnToTitle`): it leaves
  the campaign and replaces the scene with a fade-in `MainMenuState`, so quitting
  is only ever done from the title screen. Unsaved progress is abandoned, exactly
  as the old in-game Quit did — `MainMenuState`'s own Quit is unchanged. _Files:
  `ui/ActionId`, `scenes/WorldMapState`, `scenes/MainMenuState` (unchanged)._

## `1e97afc` — 2026-10-18 — "Battle camera: framed on load, eased pan, and first-time tips that stay gone"

- **The battle no longer fades in on one framing and then jumps to another.**
  The camera drive lives in `BattleState::update()`, and `update()` returns early
  while the fade-in transition plays, so the opening framing only happened once
  control started: the scene faded in on the clamped map origin and *then* moved
  to the cursor. On the big maps the two positions are far apart, which is why it
  read as a blink. `BattleState::onEnter()` now calls a new
  `BattleCameraController::focusImmediately()` (a hard cut that also drops any
  pending glide) right before starting the transition, framing the same tile the
  free-control glide would pick — the acting unit when there is one, the cursor
  otherwise — and neutralises the between-frame render interpolation for that
  frame. The scene now fades in already centred on the cursor. _Files:
  `scenes/BattleState`, `battle/systems/BattleCameraController`._
- **The edge-scroll eases instead of cutting.** `scrollCursorIntoBand()` used to
  apply the whole fixing delta in one frame, so walking the cursor across the
  board produced a jump per tile step. It now eases towards the fixing position
  (τ = 0.15s, ≈90% of the way in 0.35s) with the delta recomputed from the live
  offset every frame, so the camera is a smooth pan that still settles exactly on
  the box edge — no overshoot, no drift past the box, and the world-pass pixel
  snap keeps the tiles crisp mid-pan. _File:
  `battle/systems/BattleCameraController` (`scrollCursorIntoBand`)._
- **First-time tips are part of the campaign now, so a load cannot replay them.**
  `PartyContext::controlsTutorialShown` / `battleControlsTutorialShown` were
  explicitly "reset per new game; not persisted to saves", which meant loading a
  campaign re-showed the world-map controls dialog (on *every* load, since the
  world map is where a load lands) and the first-combat controls block.
  `SaveManager` now writes `controlsTutorialShown` and
  `battleControlsTutorialShown` in both the campaign and battle payloads (still
  `formatVersion` 4 — the keys are additive) and
  `PartyContext::applyCampaign()` restores them. Absent keys read as `true`: a
  save written before the keys existed is a campaign in progress, and replaying
  the tips is the bug being fixed. `resetForNewGame()` still seeds `false`, so a
  fresh game teaches the controls exactly once. _Files: `data/SaveManager`
  (`saveCampaign`, `saveBattle`, `campaignFromJson`, `battleFromJson`),
  `systems/PartyContext`, `scenes/WorldMapState` (save call site);
  docs: `DATA_SCHEMAS.md` §6 ("Campaign ownership")._

## `ea57868` — 2026-10-18 — "Battle camera: a cursor comfort box that works on every board (and no 900p zoom)"

- **No more zoom-in at non-720p-multiple heights.** `BattleLayout` is back to
  `floor(outH / 360)` for the art scale: rounding to the *nearest* multiple made
  1600×900 jump from 2× to 3×, which magnified the battlefield 50% and read as a
  zoom-in rather than a bigger viewport. Tiles are now never drawn larger than
  the 720p baseline ratio; a 900p window simply shows more world (the camera's
  small-map slide covers that, below). _File: `renderer/BattleLayout`._
- **The camera now pans on a board that fits the play field instead of being
  pinned.** This is what made the vertical axis look dead at every resolution
  where `clampToBounds()` was able to hold the whole board (10×10 at 1600×900
  included): centring a fitting map leaves the camera literally nothing to move
  through. New engine knob `Camera::SmallMapFit::Slide` (and the existing
  `clampToBounds()` keeps `Center` as its default) lets a map that fits the
  viewport sit *anywhere* inside it — the hard limit becomes the unpadded map
  extent, so the map itself still never leaves the play field, while the camera
  gains the slack to scroll across. The initial framing and window-resize clamp
  still centre. _Files (engine): `renderer/Camera`._
- **The edge band became a real comfort box, measured against the cursor's whole
  tile.** Three fixes in one place:
  1. The test now uses the tile's on-screen **footprint** — `tileToScreen()` is
     the diamond's **top** corner, and in iso the tile's height *is* the y offset
     (a tile reaches a full tile height below its anchor and half a tile width
     either side), so comparing the anchor alone let the bottom half of the tile
     hang outside the box before the camera reacted.
  2. The box is capped to **70% of the board**, so a board that would otherwise
     sit entirely inside it still has board left for the cursor to leave — a
     board that fits inside a viewport-only box never scrolls, which is exactly
     the "deployment doesn't move the camera" report on the 5×5 tutorial.
  3. The band still resolves to `max(15% of the viewport, map-bounds margin +
     64px)` so it can never be shorter than the margin the clamp enforces. Net
     travel on the boards in the game, at 900p: 1v1 194×97px, 10×10 448×187px,
     6×5 198×99px — all four directions, deployment and combat alike, with the
     board never leaving the play field. Kept intentionally modest: the drift is
     bounded by `(1 - cover) × board`, so a small board glides a little rather
     than following the cursor around. _Files:
     `battle/systems/BattleCameraController` (`scrollCursorIntoBand`,
     `edgeBand`), `scenes/BattleState` (context carries the board size);
     engine `renderer/Camera`._

## `880cd6e` — 2026-10-18 — "Battle camera: driven during deployment, and 900p back at the intended world scale"

- **Deployment had no camera driver at all.** `BattleState::update()` returns
  early for the deployment phase, *above* the camera drive, so
  `BattleCameraController` only ever ran in combat. That was masked until the
  previous change by `DeploymentPhaseController`'s `trackTarget()` follow; with
  that gone, nothing moved the camera during placement, so on any map larger
  than the play field the cursor could walk off screen completely. The camera
  phase machine + drive now live in `BattleState::updateCamera()`, called from
  the deployment branch *and* the combat path — one camera authority, both
  phases, and the camera keeps the cursor in the edge band during deployment
  exactly as it does in combat. _File: `scenes/BattleState`._
- **900p was rendered at the wrong world scale.** `BattleLayout` derived the
  integer art scale as `floor(outH / 360)`, which does not match the documented
  intent (the world should never be *smaller* than the 720p baseline scale):
  floor drops to 1× for any window 360–719px tall, and at 900px it gives 2×
  where the exact ratio is 2.5. At 1600×900 that meant 333 world-px of vertical
  coverage against a 323px-tall 10×10 board — the whole board fitted, so the
  camera correctly held still and the vertical axis looked dead. With
  `lround`, 720p and 1080p are unchanged and 1600×900 renders at 3× (the same
  on-screen scale 1080p already had), giving 222px of coverage so the 10×10
  board pans vertically with 296 device-px of travel. _File:
  `renderer/BattleLayout`._
  *Reverted in the next entry:* at 900p that reads as a 50% zoom-in on the
  battlefield, and the vertical axis is now served by the camera's small-map
  slide instead — the art scale is back to the largest whole multiple *below*
  the baseline ratio.

## `c620ca9` — 2026-10-17 — "Battle camera: edge-scroll in both phases, and an edge band that can be reached"

- **Deployment no longer follows the cursor.** `DeploymentPhaseController`
  drove the camera with `Camera::trackTarget()` every frame, so deployment
  smoothly chased the cursor while combat edge-scrolled the play field — the
  same scene behaved two different ways. Removed: `BattleState` already runs
  `BattleCameraController` every frame, and its free-control pass now serves
  both phases. _File: `battle/controllers/DeploymentPhaseController`._
- **The edge band was narrower than the map-bounds margin, so the vertical axis
  could never scroll.** `clampToBounds()` never lets the cursor closer to the
  viewport edge than `setMapBoundsMargin(112)` (unit-sprite overhang), but the
  band was `0.15 × viewport` = **108px of height at 720p** — *inside* the
  margin, so the cursor could never reach it and the camera silently stopped
  scrolling vertically. Horizontally the band (123px) cleared the margin by only
  11px, which is why X worked and felt thin. The band is now
  `max(0.15 × viewport, margin + 64px)`, read from the camera itself via the new
  `Camera::getMapBoundsMargin()`, so the two can no longer fight: the cursor
  always has 64px of band to be pulled back out of, and the central free-roam
  area stays ~50–66% of the viewport. When the whole board fits an axis the clamp
  still centres it (nothing to reveal), which is correct. _Files:
  `battle/systems/BattleCameraController`; engine `renderer/Camera` (getter)._

## `bb28dd7` — 2026-10-17 — "Themed battle backdrop: continuous sky + rail frame (framed layout, Tier 0/1)"

- **The rails are no longer opaque panels — the map's sky now continues behind
  them.** The battlefield's background gradient is drawn across the **whole
  window** and the world composites on top, so a fight reads as an island
  floating on the themed backdrop (FFTA-style) instead of a panel clamped
  between two bars. The rails are now a **vignette + hairline** over that
  continuous sky: the sky darkens toward the screen edges (with a softer
  top/bottom falloff) and a hairline tinted from the map's lower sky marks the
  play-field boundary. Everything derives from
  `BattleDefinition::backgroundTop/backgroundBottom`, so there are no per-map
  colours to author and no magic strings. _Files: `BattleState`
  (`renderBackdrop`, `renderRails`)._
- **Engine: backdrop seam in the world pass.** `endLogicalPass()` is split into
  `endWorldPass()` (return to the native output and clear it to the letterbox
  colour) and `blitWorldBuffer()` (composite the world buffer 1:1, centred,
  honouring its alpha), so a state can draw a full-window backdrop between the
  two. The world buffer is now composited with blending rather than opaque, as
  the pass already clears it to transparent — and because the sky is drawn
  once, full-window, a semi-transparent tile blends against the same colour it
  did before. `endLogicalPass()` remains as the
  `endWorldPass() + blitWorldBuffer()` convenience. This is the seam Tier 2
  parallax (drifting clouds/embers/snow) drops into. _Files (engine):
  `renderer/Renderer`._
- Docs: `FRAMED_LAYOUT_PLAN.md` (Tier 0/1 shipped), `TODO.md`,
  `ARCHITECTURE.md`, `AGENTS.md`.

## `9f91dcd` — 2026-10-17 — "Framed battle layout: persistent rails + crisp play field (M1)"

- **The battle screen is now a framed composition.** A permanent rail (stub
  panel for now) brackets each side of the battlefield. The rails are a
  *designed* layout element, not leftover letterbox space, so they exist at
  every resolution — there is no "rails only show on some monitors" behaviour.
  `BattleLayout` derives the rail width (~18% of the window, clamped
  200–340px) and the centred play rect from the native output size, and
  `BattleState` rebuilds the layout live when the window is resized. _Files:
  `renderer/BattleLayout`, `BattleState` (`renderRails`, `applyBattleLayout`)._
- **The world now renders at native resolution at an integer art scale, so it
  is crisp at every window size.** Root cause of the long-standing shimmer: the
  world was drawn into the fixed 1280×720 canvas and NEAREST-upscaled by the
  window ratio (1.25× at 1600×900), so texels landed on uneven screen pixels
  while the camera moved. The world-pass buffer is now sized to the play rect
  and blitted **1:1**, with the camera at an integer device-px-per-texel scale
  (`artScale`), which removes the uneven sizing entirely; the earlier
  offset snap now guarantees whole *device* pixels. This closes the
  "known-soft resolutions" backlog item. _Files: `BattleState`,
  `renderer/BattleRenderer` (viewport-driven culling + backdrop),
  `scenes/BattleLoader` (viewport-sized map origin)._
- **Engine: native-sized world buffer + native overlay drawing.** `Renderer`
  gains `getOutputSize()`, `setWorldBufferSize()` (sizes the world-pass target
  independently of the logical/design size; `endLogicalPass()` blits it 1:1,
  centred) and `begin/endNativeOverlay()` (draw in raw output pixels for
  content outside the design play field — the rails). `Camera` gains
  `getViewportSize()`. The UI/design transform (1280×720 → window) is
  unchanged, so no UI code moved; migrating the UI to a native, edge-anchored
  layout is a follow-up (M5). _Files (engine): `renderer/Renderer`,
  `renderer/Camera`._
- Docs: `FRAMED_LAYOUT_PLAN.md` (M1 marked implemented), `TODO.md`,
  `ARCHITECTURE.md`.

## `5e0de28` — 2026-10-17 — "Turn-start menu, undoable Wait, camera framing, UTF-8 dialog"

- **A player unit's turn now opens its action menu immediately.** The unit
  enters its turn with the cursor already on it and the Move/Attack/Defend/
  Wait menu up, so acting no longer needs a redundant "Enter on your own
  unit" step. `Esc` from that fresh menu still drops to the free cursor for
  inspecting, and `Enter` on the unit reopens the menu. `Accept` is consumed
  on entry so a press held from the previous turn can't fire a row instantly.
  _Files: `BattleState` (human-turn entry)._
- **Wait is now undoable with `Esc` while the unit can still act.** Picking
  Wait records the unit and its entry facing and opens the facing picker
  (arrows are preview-only). Pressing `Esc` rolls back only the facing preview
  and reopens the action menu — the move already made is **kept** (the unit
  stays on its tile) — whenever the unit has a move left or its action unused,
  e.g. move-then-Wait-then-`Esc` leaves you free to attack, use a skill, or move
  again. An exhausted turn (max-moved **and** acted) instead auto-fires Wait
  with no one-item menu, and `Esc` there confirms rather than cancels, since
  there is nothing to return to. The facing-picker `Back` case lives in
  `BattleState`'s own Back block (next to the Move/AttackTarget cancels) so the
  edge can't be swallowed upstream. _Files: `BattleMenuController` (`selectWait`
  / `cancelWaitToMenu`), `HumanTurnController`, `BattleState`._
- **Camera no longer pins before edge/corner tiles (units stay framed).**
  Root cause was engine-side: `Camera::setMapBounds` built its clamp box from
  tile *origins*, but each iso diamond spans half a tile beyond its origin
  sideways and a full tile below, so the viewport clamped early and a unit on
  an edge or corner tile sat half off-screen. Bounds now cover the true diamond
  extents, and `setMapBoundsMargin` is finally honoured in **screen pixels**
  (symmetric padding never moves small-map centering) for sprite overhang —
  battle sets a 112px margin (sprites stand ~94px above their tile at battle
  zoom). _Files (engine): `renderer/Camera`; `BattleState` sets the margin._
- **Battle camera is now FFTA-style edge-scroll, not a follow-cam.** Free
  control no longer re-centres on the active unit or cursor every frame. The
  camera stays put while the cursor roams the central area and scrolls **only**
  when the cursor reaches a thin band (15%) at the viewport edge, shifting just
  far enough to keep the cursor inside that band — and it stops immediately
  once the cursor leaves the edge region. Movement stays clamped to the map
  bounds. The lone exception is a single focus glide onto the acting unit at
  the start of each turn (and on returning from a cast/track resolution), so an
  enemy/far unit's turn is still framed without a continuous follow. _Files:
  `battle/systems/BattleCameraController`, `BattleState` (`m_turnSerial`)._
- **World render is snapped to the pixel grid (fixes "breaking" tiles/sprite
  jitter while the camera moves).** The battlefield is drawn into the fixed
  logical canvas and sampled with NEAREST, but the camera offset handed to the
  render pass was fractional (it had been continuously eased every frame), so
  every tile and sprite edge landed *between* pixels — visible as seams through
  the iso tile grid and shimmering sprites whenever the camera moved.
  `BattleState::buildInterpolatedCamera` now snaps `offset * z` to whole pixels,
  so every anchor maps to a whole logical pixel (`screen = (iso - offset) * z`
  with integer `iso`). _Files: `BattleState`._ Note: a residual known-soft
  spot remains at non-integer window ratios (1366×768, 1600×900, 1920×1080)
  because NEAREST upscaling of the 1280×720 canvas is then uneven; 1280×720,
  2560×1440 and 3840×2160 are exact.
- **Turn queue: `forceNext` replaces `moveToFront`.** The tutorial opener and
  future extra-turn effects now use a pure reorder primitive that lifts a unit
  to the current front time without touching anyone's clock (no `+1.0`
  head-start artifact). _Files: `TurnQueue`, `BattleSession`,
  `DeploymentPhaseController`._
- **Enemy move-only turns no longer pay a MajorAction.** `EnemyAI::resolveAttack`
  marks `MajorAction::Attack` only when a strike actually lands; a null/dead/
  out-of-range target leaves the action unspent, so a move-only turn costs
  move-only and alternation stays correct at equal speeds. _Files: `EnemyAI`._
- **Deployment cursor regression + UX.** The cursor no longer snaps when the
  selection is cycled (Q/E leaves it on the tile under it) and no longer
  duplicates a held unit; placing a unit selects the next **unplaced** one, and
  cycling while holding skips already-placed units. _Files:
  `DeploymentPhaseController`, `DeploymentSystem`._
- **Tutorial unit panels.** The battle unit panels stay hidden while a tutorial
  dialog is pending; the enemy walk shows only the mover, and the duel panel
  appears only after a strike lands. _Files: `BattleState`
  (`syncUnitPanelWindow`)._
- **Dialog typewriter no longer splits multi-byte UTF-8 characters.**
  `DialogWindow` steps whole code points instead of bytes, killing the
  mid-character blink on accented text. _Files: `DialogWindow`._
- **Window-mode transitions.** `Window::restore()` (engine passthrough to
  `SDL_RestoreWindow`) lets `SettingsManager::applyGraphics` leave a maximized
  state before resizing: Windowed+native maximizes, Borderless+native uses true
  fullscreen, and returning to a smaller floating size restores first so the
  resize lands. _Files (engine): `core/Window`; `data/SettingsManager`._

## `(uncommitted)` — 2026-10-09 — "FFTA deployment cycling, exp-driven leveling, QA string fixes"

- **Deployment cycling reworked to the FFTA model.** The unit portrait is now
  driven purely by roster cycling (Q/E) or by the held unit — hovering the
  cursor never changes it, so cycling reads clearly even when the cursor sits
  on an already-placed unit. `Enter` acts on the portrait's unit: an unplaced
  unit is grabbed into the hand (following the cursor as a pulsing ghost), a
  placed unit is picked back up; `Esc` drops the held unit back to the roster
  pool; placing auto-selects the next unplaced unit. Details (not Enter) is the
  inspect key. _Files: `DeploymentPhaseController`, `BattleState`._
- **Leveling is now exp-driven.** Each victory adds exp; crossing the linear
  threshold (`100 * level`, `expNeededForLevel`) grants +1 level and +1 skill
  point with the surplus carried over, replacing the flat "+1 level per win".
  `Unit::expToNextLevel` shares the same curve. _Files: `UnitProgression`,
  `Unit`, `BattleState::applyVictoryRewards`, `RosterSystem`._
- **EXP / level display.** The portrait gained a thin green EXP bar under the
  HP/MP lines; the unit-detail identity header shows an `EXP` label + bar
  instead of raw `EXP 123` text.
- **Fixed two "LV 1" display bugs.** The deployment preview and the
  `UnitDetailWindow` roster constructors built units from the template without
  applying the roster level — they now `growToLevel()` and
  `importRosterProgression()` like battle units do.
- **Fixed mojibake in `WorldMapState`.** An earlier ASCII-encoded write had
  flattened UTF-8 em dashes to `???` (visible in the quest-complete toast);
  restored.
- `ConfirmWindow` gained a separate optional button font so the
  "Start Battle?" prompt can use the heading font while its buttons stay
  body-sized. The deployment "Start Battle?" confirm now uses
  heading-prompt + body-buttons to match the world-map confirm.
- **Portraits hidden behind dialogs/tips.** The battle unit panel clears while
  a `BattleDialog` is open (tutorial tips, controls block, scripted dialogue),
  so stray unit portraits no longer flash over tutorial messages.
- **Place-on-occupied refused.** Placing a held unit onto an occupied tile now
  flashes the red invalid vignette instead of silently swapping (swap left as
  a future-redesign comment in the controller).
- **Per-skill `effectPerTarget` pacing flag.** Default `true` — the skill's
  effect animation plays on each target interleaved with that target's
  consequence. Set `"effectPerTarget": false` for a once-bloom over the threat
  tile (fireball-style), with per-target consequences after. _Files:
  `SkillLoader`, `AttackResolutionController`, `DATA_SCHEMAS.md`._
- **`BattleCameraController` (FFTA camera).** Free control holds the active
  unit centred and only pans when the cursor exits the inner ~75% viewport box
  (edge-scroll); entering a resolution glides to the caster for the cast beat,
  then SNAPS target-to-target as each hit's effect plays, and returns to the
  active unit when the resolve closes. Target-all casts stay on the caster;
  scripted dialogue always wins. Snaps hard-cut the render interpolation.
  _Files: `battle/systems/BattleCameraController`, `BattleState`,
  `AttackResolutionController` accessors._

## `(uncommitted)` — 2026-09-27 — "Fresh-game tutorial duel with contextual tips"

- A **New Game** now jumps straight into a tutorial duel right after the
  narrator: `IntroState::finish()` replaces the world-map transition with a
  `BattleState` whose `BattleRequest.tutorial` is set (reusing
  `assets/maps/1v1_test.tmj`). Winning — or skipping via the system-menu
  Quit — continues to the world map at the start node; a tutorial defeat
  retries the duel in place so a fresh game can't strand the player at a loss.
- Tutorial duels replace the one-time static controls block with **contextual
  one-line tips** that fire as the player acts: welcome, first action-menu
  open, and first Move/Attack/Skills/Defend/Wait selection. Each tip is
  one-shot per duel. Non-tutorial battles keep the existing static controls
  dialog as the fallback.
- The tutorial duel now **starts already deployed**: the party is auto-placed
  on its spawn tile facing the nearest enemy and combat begins immediately.
  Previously deployment ran with its facing picker — arrows silently rotate
  the dial and Esc is repurposed to undo the placement — before facing was
  ever explained (the only facing tip is the later combat `Wait` one, several
  dialogs later). Real battles keep the full deployment phase. The tutorial's
  deployed unit **defaults to the Debug Overlord** so the onboarding fight is a
  comfortable clear while it is being built; it falls back to the normal
  first-in-roster pick if the Overlord is not in the party.
- The tutorial duel is now **impossible to abandon**: the system menu (Esc)
  drops its **Quit** row during the tutorial, leaving only Resume. Quitting used
  to count as a defeat and dump the player back on the world map with the
  tutorial half-finished. Real battles keep Quit. (`tools\clear_saves.bat` is the
  tester's escape hatch.)
- The tutorial **enemy now opens the duel**. `DeploymentPhaseController::startCombatPhase`
  takes an `enemyOpens` flag; when set it calls the new `TurnQueue::moveToFront`
  (pass-through on `BattleSession`) to lift each enemy to the head of the queue.
  The move happens immediately after `BattleSession::init()` and *before*
  `processCurrentTurn()` — that call is what reads the front of the queue to pick
  human-vs-enemy, so reordering after `startCombatPhase()` returns leaves the
  human turn already running against a queue front that is no longer the player
  (cursor live, Enter dead, enemy frozen).
  Watching the other unit take its turn teaches the player which one is theirs far
  faster than a dialog naming it. `HumanTurnController` already snaps the cursor
  onto their unit when the turn arrives, so the only thing left to discover is that
  Enter opens the action menu — which is why the welcome tip now points at the
  arrow keys and deliberately stops short of saying "press Enter on your unit".
- **The reorder is a move to the head, not a delay and not a swap.** The first
  attempt added a flat `delayInitialTurn(unit, delay)` and was wrong twice over:
  `TurnQueue::advance()` rewrites a unit's `timeUntilTurn` to whatever it just
  spent on its last action (`TurnQueue.cpp:110`), so a unit pushed back by `delay`
  stays behind for `delay / cost` turns — a 200 delay against a 100-cost turn
  handed the enemy **three** turns before the player's first, five if it Waits.
  The second attempt swapped the first ally's and first enemy's times, which
  leaves every unit *between* them where it was and so reorders them. `moveToFront`
  lifts the unit out and re-inserts it at the head: everyone else keeps its own
  time and its relative order and simply slides down a slot. The enemy acts once,
  then the normal cost table resumes. `delayInitialTurn` and `swapTurns` have both
  been removed rather than left as unused API.
- **Turn-cost consolidation (internal).** Every "cost ÷ speed" division in the
  battle code now goes through two new `TurnQueue` statics instead of being
  open-coded at each site: `scaledCost(baseCost, speed)` and
  `turnCostFor(unit)` (which picks wait/move/action/both from what the unit
  actually did). This collapsed seven separate divisions across `TurnQueue`,
  `BattleSession` and `BattleState` into one place, and it fixes a real
  inconsistency: `rebuildPreview()` was costing each predicted turn at
  `BASE_MOVE_COST + BASE_ACTION_COST` (125) while the live timeline used
  `BASE_MOVE_AND_ACTION_COST` (100) — despite a comment in `init()` explicitly
  stating the constants are dedicated and must not be summed. The preview's
  spacing was therefore ~25% wider than the timeline it predicts. No gameplay
  impact (nothing reads `getPreview()` yet), but it would have shown up the moment
  a turn-order gauge is wired up. The `speed > 0` assertion that only `init()`
  had now guards every path.
- Main menu gains a **Continue** button at the very top whenever any save
  exists, giving the order Continue / New Game / Load Game / Options / Quit.
  It instantly resumes the newest campaign save (`SaveManager::loadLatestCampaign`);
  being the first enabled row, keyboard focus starts on it automatically.
  **Load Game** keeps its purpose (browse the save list, pick older saves,
  delete with X).
- Fixed unit draw order during movement: a walking unit was drawn in a final
  always-on-top pass, so a mover cutting around behind an enemy wrongly
  appeared in front of it. It now renders inside the tile pass at its current
  visual tile, so units closer to the camera correctly occlude it.
- Fixed battles not ending when a unit died to **DoT on its own turn** (e.g. a
  burn stack ticking down the enemy): the result was cached but never acted
  on, so the duel kept rolling. DoT deaths at the start of a turn now trigger
  the victory/defeat overlay exactly like an attack kill does.
- DoT floating damage now reads by status: **Burn** ticks in fire orange with
  an orange ring, **Poison** in a sickly dark green — no more green "-3" that
  looks like healing.
- Tutorial tips now render as the **only** thing on screen. `DialogWindow` blocked
  input but not rendering, so a unit-detail window left open from deployment
  drew its portrait (e.g. Aria) behind the first tip. Added an opt-in
  `setBlocksRenderBelow` and enabled it only for tutorial tips — story dialogue
  still shows the scene behind it.
- New **skill animation + KO framework**. A skill is a list of effects, and any
  of them can be backed by a named animation that plays _before_ the effect
  lands. This replaces the one-off `deathVfx`/`instantKill` debug flags with
  something general:
  - New effect type **`"ko"`** (`SkillEffectType::Ko`) - kills the target
    outright with **no damage**: no roll, no defense, no damage number. It goes
    through the new `Unit::kill()` and `BattleSession::applyKo()`, which keeps
    the single result-evaluation funnel intact, so a KO can still end the
    battle exactly like a killing blow.
  - New optional **`visualOnHit`** skill field (`{"id": "skull", "duration":
1.1}`, or a bare string for the default duration). The animation plays on the
    target and the skill's consequence is only applied once it finishes, so the
    unit stays on screen and alive for the effect. Each target is a two-phase
    playback driven by one `visualPlayed` flag on the pending hit, so the next
    animated skill costs zero new resolution code.
  - The **damage preview is now damage and accuracy only**, and nothing else:
    - damage skills read `DMG 12 ACC 92%`,
    - any skill without a `damage` effect reads `--- ACC 92%` - heal, buff,
      debuff, and KO alike, since a KO is a lethal rider and rolls its accuracy
      like a stun does. (`neverMiss` still reports 100%.)
    Effect text that used to be jammed into that readout - `HEAL +14`,
    `ATK +3 (3T)`, `BRN2x3(60%)` - has moved out of the preview entirely.
  - **Casting is now a two-step, popup-free flow.** There is no confirm window
    for battle casts at all:
    1. pick the skill, then the reachable tiles open up and the cursor moves over
       them. **No damage/accuracy is computed during this step** - it would be
       recomputed every frame for a target that may not be the one cast;
    2. Accept locks the tile. The splash is built around it and **the preview box
       appears** - this is the confirmation;
    3. **Q/E cycle the preview through every unit caught in the splash**
       (allies and the caster included), Accept casts, Esc unlocks the tile and
       goes back to step 1.

    The cursor snaps onto the focused unit once the tile is locked (the blast
    centre can be empty or adjacent to the real target) and follows Q/E as it
    walks the splash; it is frozen against arrow input for the duration, so the
    splash itself does not move. Esc puts it back on the blast centre. Accepting
    the tile is therefore a separate, undoable press from casting - a stray Enter
    can no longer spend a decisive turn, but a deliberate one still takes only
    two presses. `ConfirmWindow` is untouched for shop/tavern/quest, where a
    question genuinely needs asking; `setCascadeMode()`/`renderCascade()` and
    `WindowId::BattleActionConfirm` are gone.
  - **The preview box moved and gained a hit gauge.** It now sits just **below
    the vertical centre of the screen** (was the top-right corner) so it never
    covers a focused unit standing in the middle of the view. Alongside the
    `DMG n` / `---` line it shows an **ACC bar filled to the hit chance plus the
    percentage**, so the roll is readable at a glance instead of by arithmetic.
    The bar is green ≥70%, amber ≥40%, red below. Damage and hit chance are
    stored as numbers now (rather than one preformatted string) so the gauge and
    the text can never disagree.
  - **A refused press now says so, everywhere.** The red border vignette fires
    for every rejected input, not just an empty cast: accepting an unreachable
    move tile, attacking a tile out of range, a supportive cast aimed at a
    hostile, a hostile cast aimed at an ally, and a single-target/AoE cast that
    catches no legal unit will flash it and swallow the
    Accept so a held key cannot re-fire. Previously every one of those paths
    returned early before the flash check, so the vignette never appeared. It
    stays in the current phase with the cursor unchanged. The vignette is built
    in code — one gradient band per screen border with per-vertex alpha
    (`band = 5%` of the screen's short edge, squared falloff via 3 strips per
    border), drawn through `Renderer::drawGeometry` in a single call. Bands
    overlap in the corners and blend there, so corners come out slightly darker.
    It is a deliberately faint hint (peak alpha 0.30, ~0.18s), enough to register
    without reading as damage.
    `BattleState::flashInvalidTarget()` triggers it; the geometry lives in
    `renderInvalidFlashVignette()`.
  - **Removed the top turn banner** (`"<Name>'s Turn"`, and `Round N -` when a
    round was shown). The unit panel already shows who is acting. Note this
    also removes the only always-on status-tag readout - status abbreviations
    are still on the unit-detail window and in floaters.
  - **The same numbers are no longer printed twice.** Targeting used to fill the
    top battle text with the preview string while the preview box showed it too,
    so confirming an attack looked like two disagreeing readouts. The preview
    box is now the only place damage/accuracy appears.
  - **Tab (Details) explains the highlighted skill.** In the Skills submenu,
    pressing Tab shows the selected skill's description in the top battle text;
    pressing it again dismisses it, and moving to another row re-targets the
    text so it always describes the highlighted skill. Rows whose description
    has riders appended (stat changes, `Causes: ...`) say so in full, because
    the preview has no room for them. A `Tab: Details` footer hint appears when
    the menu has anything to explain. Plumbing:
    `BattleMenuItem::detail`, `ButtonMenuWindow::Item::detail`,
    `hasDetails()` / `detailAt()`, a new `UIEventType::ActionChanged` row-move
    event, and `ButtonMenuWindow::setHintLine()`. The formatting lives in
    `skill_effects::describeSkillDetail()` / `describeSkillRiders()`, so the
    one place that knows how to phrase a skill's riders stays `SkillEffects`.
  - New **`CombatVfxLibrary`** owns the built-in VFX textures and registers them
    from a single table. Adding an animation is now: drop the texture in
    `assets/sprites/vfx/` and add one row - neither `BattleState` nor the
    resolution controller names an effect id any more.
  - `CombatAnimationSystem` gained sprite support (`setVisual` + `SpriteMotion`),
    so an effect can draw a real texture instead of the expanding-ring
    placeholder; unregistered ids are unchanged.
  - `assets/sprites/vfx/skull.png` is the first sprite-backed effect (the engine
    loads PNG via SDL3_image and has no SVG decoder). It fades in over the
    unit's torso, holds, then drifts upward and dissolves.
- New **debug KO skill** (`debug_oneshot`, on the debug Overlord): a guaranteed
  hit that applies the skull as a lethal debuff. The target plays the skull and
  dies as it dissolves, and no damage number is shown at any point.
- The **basic attack is now a real skill** instead of a special case woven
  through the combat code. It lives in the skill database as
  `basic_attack` (`assets/skills/basic_attack.json`, also seeded in code by
  `SkillData::makeBasicAttack()` so the button always resolves even if the
  assets directory is unreadable), and is picked through the action menu's
  "Attack" button the same way any skill is. Consequences:
  - Targeting, splash preview, hit chance, damage, MP, and the enemy AI now all
    run through the one existing skill pipeline. `CombatSystem::hitContextFor()`
    is the single skill → `HitContext` builder shared by the player
    (`BattleState::makeHitContext`), the AI (`EnemyAI::resolveAttack`, which now
    takes the basic skill) and `DamagePreview`, replacing three copies of the
    stat-scaling and flanking rules.
  - Reach still comes from the unit, not the JSON: `SkillData::toRangeRuleFor()`
    overrides range with `atkRange` and height tolerance with `jump - 1`
    (clamped at 0) for the basic attack only, so a jump-2 unit strikes one level
    up or down and a jump-3 unit two levels. This also fixes a pre-existing bug:
    a human-controlled unit with `atkRange > 1` could only ever reach 1 tile,
    while the AI used its real range.
  - `CombatSystem::basicAttackContext()` and the temporary
    `AttackRange::basicAttackRule()` are gone; the first now delegates to the
    shared builder so it stays correct by construction.
  - It keeps basic-attack semantics rather than becoming a generic skill: zero
    base power (the unit's own attack stat is the whole base), physical, neutral
    element, 95% accuracy, no riders, no MP, and it records
    `MajorAction::Attack` rather than `MajorAction::Skill`. It is also excluded
    from a unit's Skills submenu, which is why the action menu keeps its own
    "Attack" button.
  - The "was a skill selected" test used for the Esc behaviour is now an
    explicit `isBasicAttack` check, so backing out of an aimed basic attack
    returns to the action menu while backing out of a learned skill still
    returns to the Skills submenu.
- Battle win/loss is now evaluated from exactly two triggers instead of six
  scattered call sites: `BattleSession::applyDamage()` (the single funnel every
  HP drop passes through, so player attacks, enemy attacks and AoE all resolve
  deaths) and the start of each turn (which resolves turn-based objectives such
  as `SurviveTurns` with no deaths at all). New
  `BattleState::evaluateBattleResult()` is the single funnel for turning a
  result into the end overlay. DoT and reflected damage no longer bypass the
  check.
- The victory/defeat panel now closes the moment you press Enter, and only then
  does the fade-out start — previously a bright VICTORY box sat on top of a
  darkening screen for the whole transition. The battle fade layer also moved
  to render last, so it covers the end overlay too.
- `WorldMapState` now has a `ScreenTransition`: it fades in over 0.5s on enter
  and fades out over 0.5s before handing off to a battle, closing all windows
  first. Every scene change in the game is now a ~1s out-and-in transition
  (intro, world map, battle). `IntroState`'s fade was retimed 0.8s -> 0.5s to
  match.
- In tutorial battles the action-menu tip is now shown _before_ the action menu
  opens, instead of on top of it. `maybeShowTutorialTip()` returns whether it
  pushed a dialog; `openBattleMenu()` holds the menu back and `processUIEvents()`
  opens it once the tip is dismissed.
- The victory/defeat overlay now waits for the finishing blow's VFX to play
  out. The battle result is detected immediately (no gameplay delay), but the
  overlay is deferred until no damage numbers or hit rings remain in flight,
  so the killing blow's number and impact ring are no longer cut off by the
  result screen. The turn machine is frozen during the wait.
- Entering and leaving every battle now always uses a screen transition:
  ~0.5 s fade-in on battle entry (already present) plus a new ~0.5 s
  fade-out on battle exit, driven from the victory/defeat overlay's confirm
  (the tutorial-defeat auto-retry path and the Quit path go through the same
  fade). Total ~1 s per battle hand-off. Durations are per-call
  `ScreenTransition::Config::duration` values and easy to retune.
- Cleanup: the three near-identical "advance turn → resolve victory/defeat"
  tails in `processCurrentTurn()` collapsed into one `endCurrentUnitTurn()`
  helper; DoT tick colors (floating number + VFX ring) are single-sourced
  through `statusTickVisual()` in the status registry; two dead team→color
  switches were removed from `BattleRenderer` (the placeholder sprite tints
  itself via `Unit::getTeam()` already).

---

## `(uncommitted)` — 2026-09-24 — "Battle controls tutorial + world-map menu flags + numbered nodes"

- `ButtonMenuWindow` gained `setScrollable(false)` (panel grows to fit every
  row instead of scrolling) and `setWrappable(false)` (focus stops at the ends);
  the shared geometry now drives both render and selection-scroll so lists no
  longer roll when everything fits. The world-map main menu uses both flags.
- Engine `FocusGroup` gained a wrap toggle (`setWrap(false)`).
- World-map nodes are no longer tinted red for battle/unlocked; every node shows
  a phone-keypad number (top row 1-2-3, middle 4-5-6, bottom 7-8-9) so quest
  text can point at a node by number.
- First battle of a new game now opens with a one-time controls tutorial dialog.

---

## `(uncommitted)` — 2026-09-16 — "M5 combat depth (slices 3–4): skill power scaling + boss battle & scripted waves"

### Skill power scaling per level

- `SkillData` gained `powerPerLevel` (int, default `0`), parsed in
  `SkillLoader`; `BattleState::makeHitContext` scales **damage/heal** skills
  by the caster's learned level from `SkillProgression::levelOf`:
  `basePower += powerPerLevel * (level - 1)`. Enemies have no progression and
  always fight at authored `basePower`. Because the scaling lives in
  `makeHitContext`, the battle-menu damage preview, AI and real hits agree.
- Real damage/heal skills now level to 3 (point costs 1/1/2/level):
  slash (`powerPerLevel` 3), fireball (4), heal (4), venom_strike (2),
  thunder_jolt (2).

### Boss battle + scripted reinforcement waves

- `EnemyDefinition.level` is now honoured: `UnitSpawn.level` flows through
  `BattleParticipantsBuilder` and `BattleSession::makeUnit`, which calls
  `Unit::growToLevel` for template spawns above level 1.
- `BattleEvent` gained `minTurn` (int, default `0`) + `once` (bool, default
  `false`) gating; `BattleEventSystem::setTurn(int)` is fed from
  `BattleSession::m_turnsElapsed` by `BattleState` before turn-gated emits,
  so events can be staged as **one-shot waves** that fire only after N unit
  turns.
- New content:
  - `assets/maps/boss_arena.tmj` — 6x5 arena (2 player / 3 enemy spawn
    points) reusing the 1v1 tileset.
  - `assets/units/bandit_chief.json` — the Bandit Chief boss (Soldier
    archetype, 85 HP, slash/sunder/venom_strike/fireball).
  - `assets/units/raider.json` — Raider reinforcement (Duelist archetype,
    26 HP).
  - Third battle definition `battle_boss_arena` (red combat backdrop,
    `battle_theme_02`, `KillBoss` victory rule, 350 XP / 450 gold /
    starter_boots + starter_ring): a raider spawns on the first enemy turn,
    and if the fight drags on a second one-shot wave of 2 raiders arrives at
    turn 4 — killing the chief wins regardless of standing reinforcements.
  - World-map node 8 now leads to the boss arena.

---

## `(uncommitted)` — 2026-09-16 — "M5 combat depth (slice 2): buffs & debuffs as battle statuses"

### Stat buffs / debuffs (barrier / attack up / armor down / attack down)

- `StatusEffect.h` grew four status types: `Barrier` (BRR), `AttackUp`
  (ATK+), `ArmorDown` (ARM-), `AttackDown` (ATK-) — label registry entries
  with harmful flag, non-stacking, no turn skip; JSON strings `barrier`,
  `attack_up` (`attackup`), `armor_down` (`armordown`), `attack_down`
  (`attackdown`).
- **Math** (`Unit::statusStatContribution` in `Unit.cpp`, folded into
  `tempMod()` so every stat getter / hit context / preview / AI reflects
  them): buffs grant **+25% of the BASE stat** (data + race/gender, rounded
  half up); curses take **−10% of the TOTAL stat** (base + gear, excluding
  temp/status mods so the basis stays stable). Barrier raises Defense and
  MagicDefense; AttackUp/AttackDown move Attack; ArmorDown shaves Defense
  and MagicDefense. Statuses are transient — they tick off like any status.
- **Content migrations**: `valor` is now an `attack_up` buff (was a flat
  `+3` stat mod), `weaken` an `attack_down` curse (was `-3`). New skills:
  **Barrier** (buff, 3T) and **Sunder** (debuff armor_down, 3T). The old
  `statKey`/`statAmount`/`buffTurns` preview & application paths still work
  for any skill that carries them.
- `describeSkill` prints the status tags (e.g. `BRR3`, `ATK-3`) for
  status-based buff/debuff skills instead of a phantom `ATK +0`.
- Status floaters now color by polarity: curses purple `debuff` burst,
  buffs blue `buff` burst (previously everything was a purple `debuff`).
- `Debug Overlord` gained `barrier` + `sunder` for smoke tests; class
  recipes: barrier → Soldier/Mage, sunder → Soldier/Duelist/Scout.

---

## `(uncommitted)` — 2026-09-15 — "M5 combat depth (slice 1): status effects pipeline + terrain bonuses"

### Status effects (poison / burn / stun / sleep)

- New shared model `battle/unit/StatusEffect.h`: `StatusType` +
  `StatusInfo` label registry (`PSN`/`BRN`/`STN`/`SLP`, harmful flag,
  `stacks`, `skipsTurn`, `breaksOnDamage`), runtime
  `ActiveStatus{type, turnsRemaining, damagePerTick}`, and skill payload
  `StatusEffectApply{type, turns, damagePerTick, chance}`.
- `Unit` gained `m_statuses` + `applyStatus()` (DoT statuses **stack** their
  `damagePerTick` and refresh duration; lock statuses refresh only) and
  `tickStatuses()` — called at the start of the unit's own turn after
  `resetTurn()`. Durations count the target's own turns; expired statuses are
  cleaned up as they expire.
- **DoT** (poison/burn) deals damage each tick (green floater). **Turn
  locks** (stun/sleep) consume the target's turn while present (N turns = N
  skipped turns, amber STUNNED/ASLEEP floater). Sleep additionally **breaks on
  taking real damage** only — implemented in `BattleSession::applyDamage`, not
  on DoT ticks.
- Skills carry statuses via a new optional `statuses` array in their JSON
  (per entry: `type`, `turns`, `damagePerTick`, `chance`), parsed by
  `SkillLoader` into `SkillData::statuses`. `AttackResolutionController`
  applies riders on a **landed hit** for offensive skills and on any
  **supportive cast** target, each roll gated by `CombatSystem::chanceRoll`.
  Skill previews (top text + describe box) append compact suffixes
  (`BRN2x3(60%)`).
- Turn flow hooks in `BattleState::processCurrentTurn`: DoT tick damage +
  death-by-DoT branch + STUNNED/ASLEEP skip branch (both advance the queue and
  re-check defeat/victory, mirroring the dead-unit skip).

### Terrain bonuses

- The `TileClass` `grass_atk` / `grass_mag` / `grass_eva` flags now do something:
  standing on **grass_atk / grass_mag** adds flat `+2` / `+3` attack / magic to
  your attacks; standing on **grass_eva** adds `+5` evasion when defending.
  Implemented as `CombatSystem::applyTileBonuses(HitContext&, const BattleMap&)`
  and threaded through `BattleState::makeHitContext`, `EnemyAI::resolveAttack`,
  and `DamagePreview::show` (the attack-range preview box now predicts terrain).
- The 1v1 test map now actually places these tiles (previously only normal +
  plain grass were used): grass_atk / grass_mag / grass_eva patches in the
  lower arena of `1v1_test.{tmx,tmj}`.

### New content

- `fireball` gains a **burn** rider (BRN 3/turn, 2 turns, 60%).
- New skills: `venom_strike` (physical poison DoT — Soldier/Duelist/Scout) and
  `thunder_jolt` (magical stun — Mage/Scout), both added to the
  `SkillProgression` recipe table so they are learnable/equippable through the
  skills panel; `debug_overlord` template learns them for smoke tests.

### UI

- The in-battle turn banner appends the active unit's statuses as compact tags
  (`Soldier's Turn [PSN 3]`); the battle/party unit detail window shows a
  `Status` line under EXP while any are active.

### Data

- No save-format change: statuses are transient battle state (roster remains
  the source of truth for progression). New fields on skill JSON
  (`statuses`) are optional and default empty.

---

## `(uncommitted)` — 2026-09-14 — "M4 progression: ability points, skill unlocks, level-up flow"

### Skill points and the level-up flow

- Each roster unit now earns **1 skill point per level-up** (surviving units
  still gain exactly +1 level per battle victory, so +1 SP per victory).
  Points are spent **permanently** (never re-assignable) to unlock skills.
- The victory summary notes `(level-ups grant +1 skill point)` when any
  unit leveled up. The unit inspect header already shows EXP; level-ups are
  also where the new points come from.
- `RosterUnit` now owns its progression: `baseClass` (derived from the unit
  template), `skillPoints`, and a `SkillProgression` (learned levels +
  equipped sets). Recruited units start with the template's base class and
  zero points.

### Real skill unlocks (replacing the placeholder table)

- `SkillProgression.cpp` no longer ships placeholder skills. The table is now
  **active skills only**, keyed to the real combat catalog, with a class
  recipe:
  - **Soldier**: Slash, Valor, Weaken
  - **Mage**: Fireball, Heal, Weaken
  - **Duelist**: Slash, Weaken
  - **Archer**: Valor, Weaken
  - **Scout**: Slash, Fireball, Valor
- Each skill costs **1 point** and unlocks at Lv.1 (its full combat stats from
  `assets/skills/*.json`). Skill levels/power scaling and passive/reaction
  combat effects are later milestones.

### Skills panel (`SkillProgressionWindow`)

- The **Abilities** action in the unit detail window (opened from the party
  menu) now opens a real **Skills** panel instead of being a placeholder.
  Lists the unit's unlockable skills with name, learned level (`[Lv x/1]`),
  cost and equipped marker; header shows class, level and remaining points.
- Two modes, tab-toggled: **Learn** (Enter spends a point to unlock) and
  **Equip** (Enter toggles the active-skill loadout; a freshly learned skill
  auto-equips while an active slot is free). Esc returns to unit detail.
  Learning is **double-confirmed**: the first Enter on a skill highlights it and
  shows a "Skill points are permanent — cannot be undone" banner; only a second
  Enter spends the point (Esc/navigation/Tab cancels the pending learn).

### Skills in battle

- Battle units are still rebuilt fresh from their template, but on build they
  now `importRosterProgression()` — they copy the roster's points + learned /
  equipped skills, and their **equipped active skills are grafted onto the
  unit's battle skill list**, so unlocked skills actually appear in the
  in-battle Skills menu and resolve with their real stats. Mid-battle state
  continues to persist because the roster is the source of truth.

### Save format v4 (backward-compatible)

- Campaign and battle saves bump to `formatVersion` 4 and persist each unit's
  `baseClass`, `skillPoints`, and `skills` (learned levels + equipped sets).
  v3 saves load fine: progression fields default to the template class, zero
  points, nothing learned.

---

## `(uncommitted)` — 2026-09-13 — "M3 town hub: Riverwatch town node, shop (buy/sell), tavern (quest board, recruiting, lore)"

### Town node on the world map

- New `NodeType::Town` in `WorldMapState`; the default 3×3 world gains a town at
  node 1 ("Riverwatch", rendered in a warm green), reachable straight from the
  start node 4. Arrival opens the **town hub** (`WorldMapNodeMenu`): Shop /
  Tavern / Leave. Both Esc **and** Enter on **Leave** return to the world map.
  The tavern/shop counter **Leave** buttons also exit fully back to the world
  map (Esc still steps back one level to the town hub).
- Reaching a town is a valid `reach` objective target like any other node.

### Currency-aware item values

- `ItemDefinition` gains `goldValue` (authoritative gold worth). Every starter
  and debug gear item is priced. Shop **sell** price = `goldValue`; shop **buy**
  price = `goldValue * 2`.
- New `assets/shop/stock.json` + `ShopStock` loader list the gear keys the shop
  carries (resolved via `GearCatalog::findByKey`; keys that don't resolve in a
  build — e.g. debug gear in Release — are skipped silently).

### Shop UI (`ShopWindow`)

- Entering the shop lands on a **shop counter hub scene** with its own buttons:
  **Buy**, **Sell**, **Trade** (disabled, future work), **Leave** — instead of a
  single window with an A/D Buy/Sell toggle. Each counter opens its own list
  (`ShopBuy` / `ShopSell`); Esc returns to the counter, and **Leave** exits
  all the way back to the world map (no intermediate hub menu).
- Custom list windows with a price column, live gold purse in the header, item
  description panel, and per-line confirm dialogs (`ShopConfirm`) before any
  gold moves. Up/Down navigation **wraps** at the list edges (hold-to-repeat
  included).
- Transactions settle in `WorldMapState`: buy spends gold + adds to inventory,
  sell removes an item + pays gold; the window `refresh()`es after each and the
  shopkeeper toasts the result.
- Debug/test gear (`debug_*` keys) is excluded from the sell list — the shop no
  longer lists items it won't buy.
- The sell list marks **currently equipped** gear with a yellow **"E"** marker
  and will not sell it: those rows keep their `xN` count, Enter is a no-op on
  them, and the confirm handler refuses the transaction (unequip first).

### Identical gear stacks

- Gear is no longer forced non-stackable: buying a second starter ring merges
  into one entry (`x1` → `x2`) instead of a duplicate row. Each equip consumes
  one unit of the stack (two equipped rings on one unit exhaust it, blocking
  another unit from equipping the same item), and unequip returns one unit.
  Save/load, shop sell, inventory, and equip lists all treat stacks by
  quantity.
- In the equip list, owned items that are **fully equipped** (0 free copies —
  all instances on units) still appear, grayed-out (whole row + `x0`), so the
  player can see they own it but nothing is available. Rows are sorted
  **alphabetically** (owned-but-in-use stays in its natural position, not
  shuffled to the end) and stay **focusable** — the cursor can land on them and
  show the highlight, but Enter is a no-op.

### Tavern hub

- The tavern offers three services: **Quest Board**, **Recruit**, **Talk**.
- **Quest board**: quests marked `offeredAtTavern` are no longer auto-accepted —
  they are picked up here. `QuestLog` gained `accept(id)` and
  `availableQuests()`; both boot (`acceptAuto()`) and menu New Game
  (`resetForNewGame()`) leave tavern-offered quests unaccepted so the board
  always has something to offer (main `first_watch` / `break_the_line` still
  auto-accept for the gating spine, side quest `scout_the_east` is
  tavern-offered). The journal only lists active/completed
  quests, matching what the party knows about.
- **Recruit**: `assets/tavern/candidates.json` (2 named candidates, cost +
  flavor) feeds a filtered list that hides anyone already hired (matched by
  template + name). Hiring spends gold, appends the new unit to the roster and
  party (party capped at 6), then toasts.
- **Talk**: `assets/tavern/lore.json` bar-room banter plays as a multi-line
  dialog.

### Data

- New data layer files: `ShopStock.{h,cpp}`, `TavernData.{h,cpp}`, plus
  `assets/shop/stock.json`, `assets/tavern/candidates.json`,
  `assets/tavern/lore.json`. No save-format change — roster, gold, inventory,
  and quest state already persist; recruits/buys/sells round-trip through them.

### List navigation wraps

- Vertical list windows (`InventoryWindow`, `UnitDetailWindow` slot + candidate
  lists, `ShopWindow`) now **wrap** at the top/bottom edge under hold-to-repeat
  instead of stopping — consistent with the menu behaviour.

### Playtest polish

- **Dialog text wrap**: `DialogWindow` wraps long lines (`TextWrap`) inside the
  box and grows the box to fit the line count, so the tavern Talk lore and any
  long quest/dialog lines no longer overflow the panel.
- **Recruited names in battle**: battle units and the deployment banner use the
  roster's `customName` for recruited units — Liandra shows as "Liandra", not
  "elf archer". `Unit::setDisplayName` builds the template and applies the
  custom name at spawn time.
- **Save slot labels**: save list entries are labelled by **date and time**
  (`2026-09-14 09:41`) instead of the world node, so saves made at the same
  node are distinguishable.

---

### Quest catalog (`assets/quests/*.json`)

- New `QuestData` (id, title, brief, description, main/side flag, objective,
  reward, unlocked world nodes) parsed by `QuestLoader` — mirrors the
  `SkillLoader` file-per-definition pattern with duplicate-ID detection.
- Objective types (extensible): `victory` (win the battle at a node) and
  `reach` (arrive at a node). Rewards: gold, XP, gear keys (resolved via
  `GearCatalog`).
- Starter quests: `first_watch` (main — victory at node 2, unlocks node 6),
  `break_the_line` (main — victory at node 6), `scout_the_east` (side — reach
  node 0).

### Quest state (`QuestLog`)

- New `QuestLog` (owned by `PartyContext`) tracks active/completed quests and
  serializes with the campaign save. New Game accepts the full catalog; save
  and load round-trip quest state.
- **Save format bumped to `formatVersion` 3** — campaign and battle payloads
  persist `questLog`. Legacy writes default to an empty object → open catalog.

### Quest-gated world progression

- `WorldMapState` blocks movement into (and across) nodes an incomplete quest
  unlocks (`QuestLog::isNodeUnlocked`); locked nodes render in a locked hue.
- Reaching a node satisfies `reach` goals on arrival; winning the node's battle
  satisfies `victory` goals (tested after the battle resolves on return).

### Quest journal UI

- World-map menu gains a **Quests** entry opening a `ButtonMenuWindow` list —
  main/side quests with `[...]` (active) / `[done]` (completed) prefixes.
- Selecting a quest shows a two-line dialog (objective + rewards); Back closes
  the list.

### Completion hooks

- Centralized in `PartyContext::completeQuestsForNode(nodeId, victorious)`:
  marks quests complete, grants gold + gear drops + party XP, and returns the
  newly-completed definitions so the world map can toast each completion.

---

## `(uncommitted)` — 2026-09-13 — "battle UX bugfixes: cooldown removal, confirm default, ESC stuck states, New Game reset, AOE splash height, deployment auto-facing"

### Skill cooldowns removed

- The cooldown system is gone: `SkillData::cooldown`, `Unit` cooldown state
  (`m_skillCooldowns`, `skillCooldownLeft`/`setSkillCooldown`), the JSON parse,
  and the UI CD counter/label in the skill submenu. Skills are now gated only by
  MP cost. `assets/skills/fireball.json`, `heal.json`, `weaken.json`,
  `valor.json` no longer carry a `cooldown` field.

### Confirm-window default and cancel loop

- The attack/skill confirm keeps **Confirm** as the default-selected button
  (`setConfirmSelectedByDefault`), as intended.
- The real bug was a bounce-back loop: selecting **Cancel** and pressing Enter
  popped the window but the same Accept press was still live, so
  `handleActiveTurn` re-prepared the pending attack and re-opened the confirm
  with Confirm selected — it looked like "I chose Cancel but it confirmed".
  That Accept is now consumed right when the `ConfirmResult` pops the window,
  so the press that chose Cancel can no longer re-trigger it.

### Button-menu hold-and-wrap navigation

- `ButtonMenuWindow` now supports hold-to-repeat Up/Down/W/S (and Left/Right/A/D)
  navigation using the same `UIKeyRepeat` feel as the inventory list — pressing
  once moves, holding past a short delay keeps moving at a steady interval.
- While **held**, movement **stops at the top/bottom** (no wrap). Wrapping
  around only happens on a **fresh key press after release**, matching the
  requested "release then press again to wrap" behavior.

### ESC stuck states

- ESC while in the **Skills submenu** (no unit action possible) now pops the
  skill menu and re-opens the action menu — previously the phase stayed `ActionMenu`
  with no window, soft-locking the turn.
- ESC while **aiming a selected skill** now returns to the skill submenu
  (regret path) instead of jumping to the full action menu. Plain-attack aiming
  still returns to the action menu as before.

### New Game now truly resets campaign state

- `PartyContext::resetForNewGame()` re-seeds gold, roster, party, and inventory
  on every New Game (previously a second New Game kept the previous run's
  inventory, since catalog-seed helpers skipped already-catalogued gear).

### AoE splash height tolerance

- New `splashHeightTolerance` skill field (default `-1` = ignore): splash tiles
  are dropped when |tile height - center height| exceeds the tolerance (center
  exempt). `fireball.json` uses `splashHeightTolerance: 2` with
  `heightTolerance: -1` (cast anywhere, ignore walls, splash limited to +-2).

### Deployment default facing

- A freshly placed unit now auto-faces the nearest non-player unit (arrow keys /
  Enter-Enter-Enter flow needs no manual facing tweak); manual choose-facing is
  still available via Back.

---

## `(uncommitted)` — 2026-09-12 — "gold loop: battle rewards (gold + drops), reward summary overlay, save persistence"

### Currency (gold)

- `PartyContext` now owns the campaign purse (`gold`), seeded to `kStartingGold`
  (300) on New Game. `Map gold()`/`addGold()`/`spendGold()` are the only write
  paths; no other code mutates it directly.
- **Save format bumped to `formatVersion` 2** — the campaign and battle save
  payloads now persist `gold` (`CampaignSave::gold`). Legacy saves (format 1)
  still load with `gold = 0`. Stale `user.dat` files were wiped per
  `AGENTS.md` §9.1.

### Battle rewards (`BattleRewards`)

- `BattleRewards.gold` — once-off currency granted on victory.
- `BattleRewards.items` — gear **keys** granted one copy each to the shared
  inventory on victory, resolved through the new `GearCatalog::findByKey()`
  (avoids hardcoding item IDs in battle data).
- `BattleRewards.xp` is now actually applied (before, it was dead config): total
  victory XP per surviving unit =
  `GameConstants::BATTLE_VICTORY_XP + BattleRewards.xp + event reward XP`.
  The 1v1 test battle's OnVictory GiveReward event was removed (its 100 XP now
  comes from `rewards.xp`, so the total is unchanged).
- Reward application centralized in `BattleState::applyVictoryRewards()`
  (replaces `awardVictoryBonusXp`): gold → purse, drops → inventory (unknown/
  full → LOG_WARN and skip), XP + level-ups → survivors only.

### Victory reward summary overlay

- `renderEndOverlay(true)` now shows a taller panel summarizing the payout:
  `Gold: +N`, `XP: +N each`, one `Lv up: <name>` per leveled survivor, and one
  `Drop: <name> xN` line per distinct dropped item. Defeat overlay is unchanged.

### Gold visibility

- The Party window (world-map roster hub) footer now shows the campaign purse
  (`Gold: N G`) instead of a duplicated title.

---

## `(uncommitted)` — 2026-09-12 — "facing system: choose-facing, deployment facing, front/side/rear bonuses, facing dial UI"

### Facing picker (combat "Wait")

- Choosing **Wait** now enters a choose-facing step (`HumanTurnPhase::ChooseFacing`):
  arrow keys / WASD rotate the unit's facing live, **Accept** commits and ends the
  turn. Deliberately no Back — Wait is committed as soon as it is picked.
- While choosing, the cursor is frozen and hidden so WASD only steers the dots.

### Deployment facing

- After placing a unit in deployment you also pick its facing; **Back/ESC** undoes
  the placement (unit is grabbed back onto the cursor) instead of opening the
  system menu.
- Field-facing survives grab/swap and is carried into battle (placed facing is
  applied to session units when combat starts via `DeploymentSystem::setFacing`).

### Front / side / rear combat bonuses

- A unit's facing now actually matters: `CombatSystem::attackSideFrom` classifies
  an attacker against the **defender's** facing (tile axis NE=(0,-1), SE=(1,0),
  SW=(0,1), NW=(-1,0)).
  - **Front** ×0.90 damage, +0 ACC
  - **Side** ×1.00 damage, +10 ACC
  - **Rear** ×1.10 damage, +20 ACC
- Wired into basic attacks, offensive skills, and damage preview; supportive
  (heal/buff) skills stay `Side` (no flank bonus on heals). Enemy attacks inherit
  it through the shared `basicAttackContext`.

### Facing dial UI (Tactics-Ogre/FFTA style)

- Feet arrows are gone. Units now show a **4-dot isometric dial above the head**:
  solid yellow active dot, dim gray alternatives, rendered **only** during an
  active facing choice (combat or deployment) — never on idle/placed units.
- New engine primitive `Renderer::fillCircle` (32-segment triangle-fan disc)
  replaces every scanline circle fill, so both the unit placeholder sprite and
  the small dial dots render with smooth rims.

### Input parity (arrows = WASD everywhere)

- WASD now mirrors the arrow keys at every direction-using call site
  (`Cursor`, both facing controllers, `SettingsRowWindow`, `InventoryWindow`,
  `UnitDetailWindow`, `ConfirmWindow`, `PartyWindow`, `ButtonMenuWindow`,
  `WorldMapState`) — a deliberate "check both keys" convention, per project
  decision.

---

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
