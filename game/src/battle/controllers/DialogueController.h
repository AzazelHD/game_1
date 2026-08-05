#pragma once

#include "engine/math/Vec2.h"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

class BattleState;
class Unit;
class DialogWindow;

// One line of a scripted story sequence: display name + text (fed straight
// into DialogWindow::Line), plus an optional speaking Unit for camera
// retargeting. speaker == nullptr means "no camera move for this line"
// (narrator text, or a line whose speaker isn't a map unit).
struct DialogueBeat
{
    std::string speakerName;
    std::string text;
    Unit *speaker = nullptr; // non-owning
};

// Drives a DialogWindow through a multi-beat sequence and, for beats with a
// speaker, exposes that speaker's position each frame so BattleState can
// feed it into Camera::trackTarget() in place of the cursor.
//
// DialogWindow owns rendering/typewriter/advance-on-input; this owns
// sequencing + the battle-specific "who's talking" meaning DialogWindow
// deliberately doesn't know about (kept generic for reuse outside battle,
// e.g. town/NPC dialogue).
class DialogueController
{
public:
    explicit DialogueController(BattleState &owner) : m_owner(owner) {}

    // window must already be pushed on BattleState's UIManager and stay
    // alive for the whole sequence (same non-owning pattern
    // DeploymentPhaseController uses for DeploymentWindow).
    void begin(DialogWindow *window, std::vector<DialogueBeat> beats,
               std::function<void()> onComplete);

    // Call every frame this controller is active, before deciding what to
    // feed Camera::trackTarget(). Returns the current beat's speaker, or
    // nullptr if the current beat has no speaker (caller should leave the
    // camera target unchanged, not snap anywhere).
    [[nodiscard]] Unit *currentSpeaker() const;

    // Call once per frame; detects when DialogWindow has finished the whole
    // sequence (via its own DialogFinished event, forwarded by BattleState)
    // and fires onComplete().
    void notifyDialogFinished();

    [[nodiscard]] bool isActive() const { return m_window != nullptr; }

private:
    BattleState &m_owner;
    DialogWindow *m_window = nullptr; // non-owning
    std::vector<DialogueBeat> m_beats;
    std::function<void()> m_onComplete;
};