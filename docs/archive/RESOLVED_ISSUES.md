# Resolved issues — root-cause records

Archive: these entries are kept for the reasoning, not as a to-do list. Open
work is in `../TODO.md`, and what shipped when is in `../CHANGELOG.md`.

---

# Floating damage number "flickers once" at the end of its animation

**Status:** RESOLVED — root-caused, fixed, visually confirmed by user.
All debug scaffolding (Tab FX-mode cycler, banner, probe PNGs, per-frame
logs) has been removed.
**Affected:** `game_1` battle combat VFX (placeholder ring + floating text systems)
**Severity:** Low–Medium (visual artifact, deterministic)

## Symptom

When battle feedback plays — a floating damage number rising from a tile, with
or without the expanding ring, with or without the center cast ring — the
number seems to "flicker once" right as it finishes: it is fading/moving to a
stop, then appears to **go off and appear again very shortly**, at or near its
final position.

Reproduces in **every** debug FX isolation mode (see Repro), including **mode 2
which draws ONLY the floating number** (no ring at all). Therefore the ring is
NOT the cause.

A second, separate artifact seen earlier: the placeholder red hit-ring (drawn
as an integer-scanline disc) showed solid "red line" bars at its top/bottom
poles (~π and 3π/2) — rows near the pole collapse into short horizontal
segments that read as a line. This was replaced with two sampled circle
outlines; **confirm it is gone in the current build** (leave this file open if
it persists).

## Repro

Originally isolated in debug builds via a **Tab**-cycled 4-mode FX tool
(center cast ring + hit ring + number / hit ring + number / number only /
ring only). All four modes flickered once near the end of the animation, and
a real player attack on an enemy reproduced the same behavior. The Tab tool
was removed after the fix.

## Observed (logged) data — CPU side is CORRECT

Per-frame capture (tag `FloatText` T / `Render` TEXT / `CombatAnim` T lines in
the debug run log — the log itself was deleted with the repo cleanup, 2026-10-19):

- A single floating-text entry is enqueued per press/attack.
- Rise uses `easeOut` tween over ttl 1.0 s; fade driven by **linear** t over
  `[0.65, 1.0]` so alpha reaches 0 exactly the frame the entry is erased.
- Exactly ONE `renderText` draw per frame for the number.
- Alpha monotonically `255 → … → 12 → 0`; y monotonically decreasing
  (`357.7 → 274.8`); no blip up, no duplicate draw, no second enqueue.
- Ring likewise monotonic (alpha 255→9, outer 30→90.7, RIP on done).
- Erase happens at the alpha-0 frame; no renders after.

=> The effect math is provably smooth. The flicker is NOT a double-draw or an
alpha-computation bug. It must be somewhere in the render → present path or in
what is drawn at the tile AFTER the effect erases.

## Root cause (confirmed)

`TTF_RenderText_Blended(font, text, fg)` builds the glyph surface's per-pixel
alpha **purely from anti-aliasing coverage** — it never multiplies by `fg.a`.
The requested color alpha is silently discarded (long-standing SDL_ttf
behavior, libsdl-org/SDL_ttf#70, treated as "working as designed"). So in
`Renderer::renderText`:

- `FloatingTextSystem` computed a correct, smoothly decreasing `color.a` and
  logged it faithfully — that's why the frame capture looked perfect.
- But that alpha never reached the pixels. The number rendered **fully opaque
  for its whole lifetime**, then the entry was erased the frame alpha hit 0:
  solid number → next frame, gone. That hard pop is the "flicker once at the
  end." It reproduced in every FX mode (including number-only) because it is a
  property of `renderText`, unrelated to the ring.

### Fix (applied)

`engine/src/engine/renderer/Renderer.cpp`, `renderText()`:

1. Rasterize at full opacity: `sdlColor.a = 255` (keep full AA coverage).
2. Apply the requested fade as a texture alpha mod, which SDL honors:
   `SDL_SetTextureAlphaMod(texture, color.a)` right after
   `SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND)`.
3. Early-out `if (color.a == 0) return;` before rasterizing (no invisible
   texture on the erase frame).

Note: do not mix `SDL_SetTextureAlphaMod` (Uint8 0–255) with the engine's
`Renderer::setTextureAlphaMod(Texture*, float)` (0.0–1.0 float) ranges.

## Suspects / next steps

1. **Texture alpha actually applied?** The number fade relies on per-pixel
   alpha from `TTF_RenderText_Blended` + per-texture
   `SDL_SetTextureBlendMode(TEXTURE, BLEND)`. If that blend is not applied on
   the active SDL renderer, the number stays fully opaque through the "fade"
   window and hard-cuts at the erase frame — a single visible "flicker".
   → Probe: dump the logical target to PNG at alpha≈128 and alpha≈0
   (`fx_mid.png` / `fx_end.png`) and verify the glyphs actually dim.
2. **Sub-pixel draw + scale:** number draws at fractional dst
   (e.g. `(557.2, 274.8)`) into the 1280×720 logical target. The tail of the
   rise moves <1px/frame; whether that reads as a "flip" depends on window
   scale (1:1 vs letterboxed) and the logical→native scale mode
   (`ScaleMode::Nearest` default).
3. **Present/VSync behavior on the erase frame.**

## Workarounds considered (not yet applied)

- Skip drawing entirely when `alpha == 0` (avoid the trailing invisible draw).
- Integer-snap the float-text position only when alpha is low (end phase).
- Different end behavior (shrink-to-point instead of alpha fade) as a
  placeholder-agnostic fallback.

## Files

- `game_1\game\src\ui\FloatingTextSystem.cpp` — rise/fade/render.
- `game_1\game\src\battle\systems\CombatAnimationSystem.cpp` — ring render,
  `drawRing` (scanline → sampled outlines).
- `engine\src\engine\renderer\Renderer.cpp` — logical pass,
  `SDL_BLENDMODE_BLEND` + `SDL_SetTextureAlphaMod` on text textures, present.