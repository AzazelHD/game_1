# Tuning — where do I change X?

Quick map of the designer-facing knobs. Edit, rebuild (`tools\build_debug.bat`), look.
Most battle knobs live in one file: [`game/src/config/UiTuning.h`](../game/src/config/UiTuning.h).

## Camera (`Tuning::Camera` in `config/UiTuning.h`)

| I want to…                                   | Change                          | Notes |
|----------------------------------------------|---------------------------------|-------|
| Pan earlier/later horizontally               | `kDeadZoneFractionX` (0.55)     | Margin per side = `(1 - f) / 2` of the play-field width, so it scales with the window. Higher = pans later, lower = more board visible beside the cursor. Tiles visible beside the cursor ≈ `1 + margin / (tileW / 2 × z)`. At 720p / zoom 1.35 (play field 820 px, `z` = 2.7): 0.80 → 2.3, 0.70 → 2.9, 0.55 → 3.9, 0.50 → 4.3. |
| Pan earlier/later vertically                 | `kDeadZoneFractionY` (0.75)     | Same as X, independent of it. Symmetric: it moves the top **and** bottom trigger together. Margin per side = `(1 - f) / 2` of the play-field height (720p: 0.80 → 72 px, 0.75 → 90 px, 0.70 → 108 px). Only does anything if the map overflows the play field vertically (`Camera::getPanAxes`). |
| Change top headroom above the cursor tile    | `kMarkerHeadroom` (= `Cursor::kHoverOffset`, 50) | World units reserved above the floor for the floating marker. It is tied to the marker's own reach: reserve less and the marker is clipped the moment the cursor sits on the dead zone's top edge. To shrink the top gap, shrink the marker (`Cursor::kHoverOffset`). Also raises the map's top scroll bound (`BattleState::computeCameraTopExtra`). |
| Change the bottom gap                        | `floorSurfaceAt()` in `battle/systems/BattleCameraController.cpp` | Currently half a tile (the tile art hangs under the diamond). |
| Make panning snappier/floatier               | `kEaseTau` (0.15 s)             | Smaller = snappier. |
| Change cast/dialogue glide speed             | `kGlideSeconds` (0.2 s)         | |
| Change slack at the map border               | `kMapEdgeMarginPx` (112 px)     | Keeps edge unit sprites on screen; also decides whether an axis can pan. |
| Change a battle's zoom                       | `defaultZoom` in `config/BattleCatalog.cpp` | Per battle. |

Cursor height: the dead zone already lifts the floor by `tileHeight * halfTileHeight`
(`floorSurfaceAt`), the same offset the renderer uses. Only the *marker* headroom is a knob.

## Cursor marker (`Tuning::Cursor`)

`kHoverOffset` (distance above the floor), `kTriangleHalfWidth`, `kTriangleHeight`.
Drawn by `BattleRenderer::drawCursorAt` / `drawCursorTriangle`. `kHoverOffset` is
also the camera's top reserve (`Camera::kMarkerHeadroom`), so lowering it tightens
the dead zone's top gap at the same time.

## Framed layout / bars (`Tuning::Layout`)

`kRailFraction`, `kRailMin`, `kRailMax` (side rails), `kBaselineViewH` (world scale:
`artScale = floor(windowH / 360)`). Computed in `renderer/BattleLayout.cpp`.

## Fonts

`engine/src/engine/renderer/FontManager.cpp` — table of `{role, path, size}`
(Body 24, Heading 32, Title 72, Placeholder 40). Requires an engine rebuild.

## UI windows / panels

Per-window sizes and paddings are local constants in the window's own file:
`game/src/battle/ui/` (`UnitPanelWindow`, `DeploymentWindow`, `UnitInspectWindow`),
menus in `battle/controllers/BattleMenuController.cpp`, shared widgets in
`engine/include/engine/ui/` and `game/src/ui/`.

## Other

- Viewport baseline / transition time / turn costs: `config/GameConstants.h`.
- Edge-pan / follow easing defaults: `engine/include/engine/renderer/Camera.h`.
