# Changelog

All notable completed milestones for the TRPG game project.

---

## 2026-09-05 – UI input architecture + mechanical cleanup

### Architecture & UI

- UI navigation refactor (hand-rolled selection → engine primitives) for `ButtonMenuWindow`,
  `InventoryWindow`, and `UnitDetailWindow`: each window now drives selection with an engine
  `FocusGroup` + a set of `Button` carrier rows, while keeping all custom drawing, anchoring,
  scrolling, and viewport math.
- Accept input is routed through `FocusGroup::activateSelected()` → `Button::activate()`, so the
  enabled/`onClick` gating lives in one place per window.
- Removed per-window manual bookkeeping: `m_selectedIndex` / `m_selectedActionIndex` /
  `m_selectedSlotIndex` / `m_selectedItemIndex`, the part-H slot-restore flag, and the never-used
  `CloseFocusable` class.
- Navigation semantics preserved window-by-window: clamp (no wrap-around), hold-to-repeat timing,
  A/D page jumps, scroll-follow, empty-inventory close, and "select a slot → equip → back to the
  same slot".

### Fixed

- Gear HP/MP sync on equip/unequip: roster stores a heap snapshot of current HP/MP before applying
  loadout changes and restores it on unequip, so swapping gear no longer clamps or loses current
  values (`UnitDetailWindow::confirmItemSelection` / `unequipCurrentSlot`).
- Redundant off-screen tile renders culled in the battle renderer (two-level gate: render pass
  eligibility + per-tile batch eligibility).
- Unit panel portrait now syncs to the active unit immediately when a turn starts
  (`BattleState::syncUnitPanelWindow`), instead of only after the first action.
- World map camera clamping: audited, behavior kept as-is.

### Mechanical cleanup

- `Pathfinder::findPath` local coordinate hash removed; pathfinding maps now key via the shared
  `Vec2iHash` (`MovementRange.h`).
- Local `drawCircle` polygon helpers in `WorldMapState` and `PartyWindow` consolidated into a single
  `UIUtils::drawCircle`; the per-file manual `Color` → `FColor` conversions were dropped in favor of
  the implicit conversion.
- `DamagePreview` text centering via `renderTextInRect` (Center/Middle).
- `Pathfinder` heuristic uses engine `MathUtils::manhattanDistance`; world-map interpolation uses
  engine `MathUtils::lerp`.
- `Gear.cpp` one-line stub removed.

### Notes

- `MovementAnimationController::kPi` retained: a `constexpr float` is preferable to the `M_PI`
  macro (float precision, MSVC-safe without `_USE_MATH_DEFINES`).
- World-map `distanceSquared` kept local: the engine exposes no distance helper and the helper is
  used only within a single translation unit.

---

## Notes

- Unit progression / class system design exists in `UNIT_DESIGN.md` but is explicitly deferred.