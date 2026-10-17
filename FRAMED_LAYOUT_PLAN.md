# Framed Battle Layout — Persistent Rails + Crisp Play Field

Status: **M1 implemented** (rails + crisp play field); **themed backdrop shipped (Tier 0/1)**; M2–M5 planned
Scope: battle scene presentation (layout + render pipeline + HUD rails)
Author: opencode (planning session)

---

## 1. Goal

Give the battle screen a deliberate **framed composition**: persistent side rails
carrying game information, and a **crisp** battlefield between them — matching a
"modern old-school TRPG" look (Hades / Slay the Spire / Darkest Dungeon /
tactical-RPG sidebars) while keeping the existing FFTA-style mechanics untouched.

This replaces the current "everything floats over the map" HUD and permanently
removes the letterbox-bar problem (no black bars, no resolution-dependent
decorative rails).

---

## 2. Locked design decisions

1. **Rails are permanent.** They are a designed layout element, not leftover
   space, so they exist at *every* resolution (no "0 rails" cases).
2. **Play field is integer art scale.** The world renders at a whole number of
   device pixels per source texel → crisp, no shimmer, no visible scaling ratio.
3. **No black bars.** The rails occupy the side regions; the world fills the
   middle. Full window usage.
4. **One rule for rail content:** *left = who is acting; right = what is next /
   what is happening.*
   - **Left rail** = **Actor card**: the unit whose turn it is — portrait, name,
     HP/MP, status icons, and the currently selected action/skill.
   - **Right rail** = **Turn queue** (FFTA-style, top = next), which
     **swaps to a Target card** while a skill/attack is aimed/resolving
     (target portrait, HP, predicted DMG/ACC), and can read as a brief
     **duel** (actor ⇄ target) on the strike.
   - Enemy turns use the same rule (left = enemy actor, right = your unit as
     target) — no special-casing.
5. **Rail sizing:** ratio-based, ~**18% of window width** each side, clamped to
   **[200, 340] px**, scaled by the UI scale. Always present; they shrink to the
   floor rather than disappear.

---

## 3. Target layout

```
┌───────────┬──────────────────────────────────┬───────────┐
│  RAIL     │                                  │  RAIL     │
│  ACTOR    │        PLAY FIELD                │  TURN     │
│  card     │   (integer art scale, crisp)     │  QUEUE /  │
│           │                                  │  TARGET   │
└───────────┴──────────────────────────────────┴───────────┘
```

- `playRect` = window minus both rails; this is the **world viewport**.
- Rails are drawn at **native resolution** (crisp, resolution-independent).
- **Shipped (Tier 0/1):** the *map's sky is drawn across the whole window*, so
  the battlefield reads as an island floating on the themed backdrop; the rails
  are a **vignette + hairline** over that sky rather than opaque panels, derived
  from `BattleDefinition`'s background colours. The diagram above is the logical
  zoning, not a literal panel split.
- **Next (Tier 2):** drifting parallax layers (clouds / embers / snow) between
  the sky and the world blit, inside `BattleState::renderBackdrop`.

---

## 4. Current state (starting point)

- Battle world + UI are rendered into a fixed **1280×720 logical canvas**
  (`BattleState::onEnter` → `renderer.setLogicalPresentation(VIEW_W, VIEW_H, Letterbox)`),
  then blitted to the window with **NEAREST** at whatever ratio.
  `GameConstants::VIEW_W/VIEW_H = 1280/720`. World scale `m_scale = 2`
  (2 logical px per source texel).
- **Already landed this session** (prerequisites):
  - `BattleState::buildInterpolatedCamera` snaps the render offset to the pixel
    grid (`round(offset * z) / z`) — removes fractional-position seams/jitter.
  - `BattleCameraController` is FFTA-style **edge-scroll** with a one-time
    turn-start focus glide; engine `Camera` bounds are diamond-exact and honor a
    screen-space margin.
- **Known-soft resolutions** (`TODO.md`): non-integer window ratios
  (1366×768, 1600×900, 1920×1080) still show uneven NEAREST texels while moving.
  **This plan resolves that** by rendering the world at native, integer scale.

---

## 5. Architecture — the crux (coordinate spaces)

The hard part is that today **everything shares one 1280×720 letterbox space**.
The new model introduces distinct spaces; being explicit about them is what keeps
this clean rather than patchwork.

### 5.1 Play-field render (crisp, native)
- Render the world directly to the **native output target**, restricted to
  `playRect`.
- Camera: `viewportSize = playRect.size`, `renderScale = S` where **S is an
  integer** (device px per source texel). Offset snapped to whole device pixels
  (generalizes the existing snap).
- Result: source texels → exactly `S×S` device px; camera translates by whole
  device px. No shimmer at any window size.
- `S` is the **world zoom**: fixed integer (default candidate `S = 2`), possibly
  a later user setting stepped in integers.

### 5.2 Rails (native)
- New rail layer drawn at native resolution, anchored to window edges.
- Actor card (left) and Turn queue / Target card (right).
- Reuse existing unit-panel logic — `UnitPanelWindow` already has
  `setSingle` / `setDuel` / `clearPanels` and `BattleState::syncUnitPanelWindow`
  already drives actor/target/duel states.

### 5.3 Menus / dialogs / tile-anchored overlays
- Screen-anchored menus (action/skill/system/inspect) can remain letterboxed
  initially — flagged as a follow-up migration.
- **Tile-anchored** overlays **must** move into play-field space:
  - `DamagePreview` (currently native pass via `toNativeRect`).
  - `FloatingTextSystem` + `CombatAnimationSystem` (world pass — fine once the
    camera viewport = playRect, but they must project through the play-field
    camera, not the old letterbox).
  - `BattleWorldProjection::tileFeetAnchor` (camera-based → follows automatically
    once the render camera is correct).
  - `DebugRenderer::flush` (camera-based → follows).
- **Audit list** of native-pass code that assumes `VIEW_W/H == output` and must
  use the real output size / play rect: `renderInvalidFlashVignette`,
  `renderEndOverlay`, `renderTopBattleText`, `renderDeploymentHud`,
  `renderUIStack`, screen-transition fade, `drawBackground`.

### 5.4 Recommended implementation approach
- **Approach A (recommended): native world pass + clip/draw into `playRect`.**
  - Engine adds: output-size getter, a clip-rect (or sub-rect target) so the
    world can't bleed under the rails, and `Camera::getViewportSize()`.
  - World renderer culls against the live viewport/play rect instead of
    `GameConstants::VIEW_W/H`.
- **Approach B (alternative): reuse the logical pass** with the canvas sized to
  `playRect` and blitted 1:1 into the play rect (add a target-rect to
  `endLogicalPass`). More reuse, but the "fixed logical resolution" model and the
  new variable play rect fight each other; less clean long-term.

---

## 6. Engine changes (no gameplay logic in engine — API only)

1. `Renderer`: expose **output size** (`SDL_GetRenderOutputSize`) and/or the
   presentation transform, so game code can compute layout.
2. `Renderer`: a **clip rect** (or sub-rect render target) so the world pass is
   confined to `playRect`.
3. `Camera`: `getViewportSize()` (for culling against the live rect).
4. Optional: a "target rect" variant of `endLogicalPass` if Approach B is chosen.
5. Keep the existing **NEAREST** scale mode; the whole point is native rendering
   removes the upscale, so presentation filtering becomes moot for the world.

---

## 7. Game changes

1. **Layout module** (new, e.g. `game/src/renderer/BattleLayout.{h,cpp}` or a small
   struct on `BattleState`): compute `railWidth`, `playRect`, `railRects` from
   output size + UI scale. Recompute on window/resolution change.
2. **`BattleState`**: size the camera to `playRect`, set integer `renderScale`,
   snap offset to device px; drive the world pass into `playRect`.
3. **World renderer (`BattleRenderer`/`BattleRendererContext`)**: cull and draw
   the background against the live play rect; consume the play-field camera.
4. **Tile-anchored overlays**: `DamagePreview`, floating text, VFX → play-field
   space.
5. **Rail layer**: new renderer/window for the Actor card + Turn queue / Target
   card, reusing `UnitPanelWindow` state and `BattleState::syncUnitPanelWindow`.
6. **Native-pass audit**: replace `VIEW_W/H` assumptions with output size /
   play rect where they are not UI-layout constants.
7. **Deployment, tutorial, end overlay, system/inspect menus**: verify they
   coexist with the rails (tutorial currently hides panels during dialogs — the
   rails should follow the same rule).

---

## 8. Milestones

- **M1 — Layout + crisp play field, stub rails.** ✅ *Implemented.* `BattleLayout`
  computes the rails/play rect/integer art scale; the world renders at native
  integer scale inside `playRect` (blitted 1:1); the rails are drawn as solid
  native panels with placeholder headers. *Result: the framed look + no shimmer
  at any size.* Rebuilt live on window resize.
- **M2 — Rail content.** Actor card (left); Turn queue + Target card (right),
  reusing existing panel logic.
- **M3 — Tile-anchored overlay migration.** DamagePreview / floating text / VFX
  into play-field space.
- **M4 — Polish.** Ornamental frame, ambient rail backdrop, queue→target→duel
  transitions, rail show/hide with dialogs.
- **M5 (optional) — Menu/dialog migration** to native edge-anchored layout;
  world-zoom user setting.

---

## 9. Open decisions

- World zoom `S`: fixed (default 2) vs. adaptive-per-resolution vs. user setting.
- Rail width policy confirm (ratio 18%, clamp 200–340px, UI-scaled).
- Whether menus stay letterboxed long-term (M5) or migrate immediately.
- Rail backdrop art vs. plain panels for M1 (recommend plain panels first).
- Minimum supported window size (recommend keeping 1280×720 usable).

---

## 10. Risks & mitigations

- **Coordinate-space regressions** (biggest): tile-anchored overlays or culling
  left in the old space → misaligned previews/effects. *Mitigate:* do M1+M3
  together for anything tile-anchored; keep a per-space audit checklist.
- **UI layout assumptions**: much code assumes `VIEW_W/H`. *Mitigate:* phase the
  menu migration (M5); audit non-layout uses first.
- **Window/resolution changes**: play rect changes → must recompute layout,
  camera viewport, and clip. *Mitigate:* single `recomputeLayout()` path invoked
  on resize and on resolution change (already have `SettingsManager::applyGraphics`).
- **Rail width on small windows**: 200px floor at 1280×720 leaves ~880px play.
  *Mitigate:* floor + optional collapse/cinematic mode later.
- **Engine boundary**: only API additions in `engine/`; all layout/rails logic in
  `game/src/`.

---

## 11. Verification (after implementation)

1. **Build**: engine Debug + game Debug (and Release for the FINAL flow).
2. **Resolutions**: 1280×720, 1366×768, 1600×900, 1920×1080, 2560×1440,
   3840×2160 — world crisp (no shimmer while the camera moves), rails always
   present, no black bars.
3. **Window modes**: windowed, borderless, native; maximize → floating smaller
   resolution still lays out correctly.
4. **Camera**: node 7 large map — edge-scroll, clamping, corners; a unit at each
   corner stays framed inside `playRect` (not under a rail).
5. **Rails**: actor card tracks the active unit; turn queue order correct; target
   card swaps in on aim/resolve; enemy turns show enemy-left / your-unit-right.
6. **Overlays**: damage preview + floating text align with the unit (play-field
   space); invalid-flash vignette and screen transition cover the full window.
7. **Flows**: deployment, tutorial duel (panels hidden during dialogs), end
   overlay, system/inspect menus all coexist with the rails.
8. **Smoke**: Boot → Main Menu → World Map → Deployment → Combat → Attack →
   Victory → Return.
9. Delete stale saves only if a save-affecting change occurs (this plan should
   not).

---

## 12. Files likely touched

Engine:
- `engine/include/engine/renderer/Renderer.h`, `engine/src/engine/renderer/Renderer.cpp`
  (output size, clip/sub-rect, presentation transform access).
- `engine/include/engine/renderer/Camera.h` (+`.cpp` if needed) — `getViewportSize`.

Game:
- `game/src/scenes/BattleState.cpp/.h` — camera setup, world pass, native-pass audit.
- New `game/src/renderer/BattleLayout.{h,cpp}` (or equivalent) — layout math.
- `game/src/renderer/BattleRenderer.cpp/.h`, `BattleRendererContext.h` — viewport culling, background.
- `game/src/renderer/BattleWorldProjection.*` — confirm play-field space.
- `game/src/battle/ui/UnitPanelWindow.*`, `game/src/battle/controllers/BattleMenuController.*`
  — rail content hooks.
- `game/src/ui/DamagePreview.*`, `FloatingTextSystem`, `CombatAnimationSystem` — space alignment.
- `game/src/config/GameConstants.h` — possibly play-field/rail constants.
- `game/CMakeLists.txt` — wire any new source.
- Docs: `CHANGELOG.md`, `TODO.md`, `ARCHITECTURE.md` (ownership/render contract).

---

## 13. Non-goals (this plan)

- No gameplay/mechanic changes (no rules, balance, AI, turn logic).
- No new art assets required for M1 (stub rails are solid panels).
- No audio, no gamepad, no monster units, no promotions (see `AGENTS.md` §10).
- Not switching the world to linear filtering (crisp integer scale is the goal).
