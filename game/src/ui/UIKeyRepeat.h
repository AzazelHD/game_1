#pragma once

// UIKeyRepeat
// -----------
// Lightweight hold-to-repeat tracker for UI list navigation (inventory,
// slot lists, candidate lists, settings rows, …). On the first down-edge
// it fires immediately, then waits an `initialDelay` before the first
// repeat, and continues firing at a fixed `interval` while the key is
// held. On key-up everything resets.
//
// The engine's `Input::isKeyPressed(key, allowRepeat=true)` already
// exposes OS-level key repeat, but that follows the OS rate (usually
// ~30 Hz with a small initial delay) which is rarely the right feel for
// a tightly-tuned menu. UIKeyRepeat gives every UI window the same
// tuning without coupling them to platform-specific repeat settings.
//
// Usage:
//
//   UIKeyRepeat upRepeat, downRepeat;     // members of the window
//
//   // once per frame, from update(dt):
//   upRepeat.start(0.35f, 0.09f);
//   downRepeat.start(0.35f, 0.09f);
//
//   // once per frame, from handleInput(input):
//   const bool upHit   = upRepeat.tick(dt,   input.isKeyDown(KeyCode::Up));
//   const bool downHit = downRepeat.tick(dt, input.isKeyDown(KeyCode::Down));
//   if (upHit)   --m_selected;
//   if (downHit) ++m_selected;
//
// `start()` is idempotent; calling it every frame with the same
// parameters just refreshes the values and is safe.
class UIKeyRepeat
{
public:
    UIKeyRepeat() = default;

    // (Re)configure the initial delay and the steady-state interval.
    // Either value <= 0 disables that phase (only the first edge fires).
    void start(float initialDelaySeconds, float intervalSeconds);

    // Step the timer with `dt` and the current physical key state.
    // Returns true exactly on the frame the key should be considered
    // "pressed" — once on the initial down-edge and once per interval
    // tick while the key remains held past the initial delay.
    bool tick(float dt, bool isCurrentlyDown);

    // Forcibly reset (e.g. when leaving a state). Next down-edge will
    // fire the initial press again.
    void reset();

private:
    bool  m_wasDown = false;
    bool  m_initialFired = false;
    float m_initialDelay = 0.35f;
    float m_interval = 0.09f;
    float m_heldFor = 0.f;
    float m_sinceLastFire = 0.f;
};
