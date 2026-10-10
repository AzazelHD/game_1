#include "ui/ConfirmDialogs.h"

#include "engine/renderer/FontManager.h"
#include "ui/UIManager.h"
#include "ui/WindowId.h"
#include "ui/windows/ConfirmWindow.h"

namespace
{
    ConfirmWindow *pushConfirm(UIManager &ui, WindowId id, FontRole fontRole)
    {
        auto *confirm = ui.push<ConfirmWindow>(id);
        confirm->setFont(FontManager::instance().get(fontRole));
        return confirm;
    }
}

ConfirmWindow *pushDeleteSaveConfirm(UIManager &ui)
{
    ConfirmWindow *confirm = pushConfirm(ui, WindowId::SaveListDeleteConfirm, FontRole::Body);
    confirm->setPrompt("Delete this save file?");
    confirm->setButtonLabels("Cancel", "Delete");
    return confirm;
}

ConfirmWindow *pushStartBattleConfirm(UIManager &ui)
{
    // Raised from both the deployment action menu and the deployment phase
    // controller, so tear down any half-open one first. Match the world-map
    // "Start Battle?" prompt: heading font, body-sized buttons.
    ui.popById(WindowId::BattleDeploymentConfirm);
    ConfirmWindow *confirm = pushConfirm(ui, WindowId::BattleDeploymentConfirm, FontRole::Heading);
    confirm->setButtonFont(FontManager::instance().get(FontRole::Body));
    confirm->setPrompt("Start Battle?");
    return confirm;
}

ConfirmWindow *pushSettingsExitConfirm(UIManager &ui, const std::string &prompt)
{
    ConfirmWindow *confirm = pushConfirm(ui, WindowId::SettingsExitConfirm, FontRole::Heading);
    confirm->setPrompt(prompt);
    confirm->setButtonLabels("Discard", "Save");
    return confirm;
}
