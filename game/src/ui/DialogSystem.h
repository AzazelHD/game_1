#pragma once

#include <functional>
#include <string>
#include <vector>

#include "ui/UIManager.h"

class Renderer;
class Font;
class DialogWindow;
class Input;

// DialogLine: a single line (or paragraph) of dialog with optional metadata.
struct DialogLine
{
    std::string speakerName; // displayed above the box — empty = no speaker label
    std::string text;        // the actual dialog content
    std::string portrait;    // path to portrait sprite — empty = no portrait (Phase 12)
};

// DialogSystem manages sequences of DialogLines and drives the game's DialogBox.
// It handles advancing lines, calling callbacks when dialog ends, and integrating
// with keyboard input (DialogAdvance key).
//
// Usage in BattleState (example):
//   m_dialog.start({ {"General", "Halt! Your advance ends here."}, {"", "..."} },
//                  [this]{ /* called when all lines finish */ transitionToPlayerTurn(); });
//
// TODO: Declare the class with:
//   - start(std::vector<DialogLine> lines, std::function<void()> onFinish)
//       Load a sequence of lines. Show the first one immediately in the DialogBox.
//       Store onFinish — call it after the last line is dismissed.
//
//   - update(float dt, const Input& input)
//       Forward dt to m_box.update(dt).
//       If DialogAdvance is pressed:
//         - If box is not finished: call m_box.skipToEnd() (show full line instantly).
//         - If box is finished: advance to next line, or call onFinish and close if done.
//
//   - render()
//       Forward to m_box.render(), plus draw the speaker name label above the box
//       and the portrait on the side (placeholder rect until Phase 12).
//
//   - isActive() const → bool
//       True while a dialog sequence is running. BattleState should block player
//       input and turn advancement while this is true.
//
// Design note: both DialogSystem and DialogBox live in the game layer because
// reveal pacing, portrait placement, and narrative presentation are game UX decisions.
class DialogSystem
{
public:
    void start(std::vector<DialogLine> lines, std::function<void()> onFinish, const Font *font);
    void update(float dt, const Input &input);
    void render(Renderer *renderer) const;

    bool isActive() const { return m_active; }

private:
    UIManager m_uiManager;
    DialogWindow *m_window = nullptr;
    std::vector<DialogLine> m_lines;
    bool m_active = false;
    std::function<void()> m_onFinish;
};
