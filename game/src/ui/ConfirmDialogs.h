#pragma once

#include <string>

class UIManager;
class ConfirmWindow;

// One factory per logical confirm dialog, so its WindowId, wording and button
// labels are written in exactly one place instead of being copy-pasted at every
// site that raises the same question.
//
// Deliberately NOT a ConfirmWindow subclass hierarchy. Every confirm in the
// game handles input, emits the event and renders identically, and differs only
// in content — so a subclass per appearance would override nothing while each
// call site had to know which class to pick. Promote one to a subclass only when
// a variant needs genuinely different *behaviour* (extra buttons, a text
// field, an auto-confirm countdown).
//
// Each returns the pushed window; UIManager owns it, so callers that only
// trigger the dialog can ignore the result.

ConfirmWindow *pushDeleteSaveConfirm(UIManager &ui);
ConfirmWindow *pushStartBattleConfirm(UIManager &ui);
ConfirmWindow *pushSettingsExitConfirm(UIManager &ui, const std::string &prompt);
