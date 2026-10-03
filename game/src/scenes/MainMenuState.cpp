#include "engine/core/App.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Rect.h"
#include "engine/math/MathUtils.h"
#include "engine/renderer/Aligment.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/FontManager.h"
#include "engine/renderer/Color.h"
#include "engine/renderer/Renderer.h"
#include "engine/statemachine/StateMachine.h"
#include "config/GameConstants.h"
#include "data/SaveManager.h"
#include "scenes/MainMenuState.h"
#include "settings/SettingsState.h"
#include "ui/windows/ButtonMenuWindow.h"
#include "ui/ConfirmDialogs.h"
#include "ui/ActionId.h"
#include "ui/WindowId.h"
#include "scenes/IntroState.h"
#include "scenes/WorldMapState.h"
#include "systems/PartyContext.h"

MainMenuState::MainMenuState(StateMachine<Scene> &sm, Renderer *renderer, bool fadeInOnEnter)
    : m_stateMachine(sm),
      m_renderer(renderer),
      m_fadeInOnEnter(fadeInOnEnter)
{
}

void MainMenuState::onEnter()
{
    m_transitioning = false;
    m_saveListWindow = nullptr;
    m_saveListSlotIndexes.clear();
    m_pendingDeleteSlotIndex = -1;
    m_uiManager.clear();
    SaveManager::instance().loadIndex();

    m_mainMenuWindow = m_uiManager.push<ButtonMenuWindow>(WindowId::MainMenu);
    m_mainMenuWindow->setFont(FontManager::instance().get(FontRole::Heading));

    // Center horizontally (box width now follows the font); only fix the
    // vertical anchor below the title.
    m_mainMenuWindow->setPanelPosition(Vec2f{0.0f, GameConstants::VIEW_CY - 40.0f});
    m_mainMenuWindow->centerHorizontally(true);
    refreshMainMenuItems();

    if (m_fadeInOnEnter)
    {
        m_transition.start({
            .transition = ScreenTransitions::FadeIn,
            .duration = 0.5f,
            .easing = easeInOut,
        });
    }
}

void MainMenuState::onExit()
{
    m_uiManager.clear();
}

void MainMenuState::handleInput()
{
    const Input &input = Input::instance();

    // 1. GUARD FIRST: Block all actions if we are currently mid-transition
    if (m_transition.isActive() || m_transitioning)
        return;

    m_uiManager.handleInput(input);
    processUIEvents();
}

void MainMenuState::update(float dt)
{

    // This is purely for background visuals, fading animations, or UI transitions.
    if (m_transition.isActive())
    {
        m_transition.update(dt);
    }

    m_uiManager.update(dt);
}

void MainMenuState::render(float alpha)
{
    (void)alpha;
    m_renderer->clear(Color{18, 21, 29, 255});

    const std::string title = "TRPG";
    m_renderer->renderTextInRect(FontManager::instance().get(FontRole::Title),
                                 title,
                                 Rectf{0.0f, 130.0f, GameConstants::VIEW_W, 64.0f},
                                 Color{235, 240, 250, 255},
                                 HorizontalAlign::Center,
                                 VerticalAlign::Middle,
                                 false,
                                 false,
                                 false);

    m_uiManager.render(m_renderer);

    m_transition.render(m_renderer, GameConstants::VIEW_W, GameConstants::VIEW_H);
}

void MainMenuState::processUIEvents()
{
    auto events = m_uiManager.drainEvents();
    for (const UIEvent &event : events)
    {
        if (event.windowId == WindowId::SaveListDeleteConfirm && event.type == UIEventType::ConfirmResult)
        {
            m_uiManager.popById(WindowId::SaveListDeleteConfirm);
            if (event.confirmed && m_pendingDeleteSlotIndex >= 0)
            {
                SaveManager::instance().deleteSave(m_pendingDeleteSlotIndex);
                m_pendingDeleteSlotIndex = -1;
                if (m_saveListWindow)
                    refreshSaveList();
            }
            continue;
        }

        if (event.windowId == WindowId::SaveList)
        {
            if (event.type == UIEventType::ActionCanceled)
            {
                closeSaveList();
            }
            else if (event.type == UIEventType::ActionSelected)
            {
                const int idx = event.index;
                if (idx < 0 || idx >= static_cast<int>(m_saveListSlotIndexes.size()))
                    continue;

                if (event.actionId == ActionId::LoadGame)
                {
                    m_transitioning = true;
                    CampaignSave save;
                    const int slotIndex = m_saveListSlotIndexes[static_cast<std::size_t>(idx)];
                    if (SaveManager::instance().loadCampaign(slotIndex, save))
                    {
                        PartyContext &ctx = PartyContext::instance();
                        ctx.ensureInitialized();
                        if (ctx.applyCampaign(save))
                        {
                            m_stateMachine.replace(std::make_unique<WorldMapState>(m_stateMachine, m_renderer, save.worldNodeId));
                            return;
                        }
                    }
                    m_transitioning = false;
                }
                else if (event.actionId == ActionId::DeleteSave)
                {
                        m_pendingDeleteSlotIndex = m_saveListSlotIndexes[static_cast<std::size_t>(idx)];
                        pushDeleteSaveConfirm(m_uiManager);

                }
            }
            continue;
        }

        if (event.windowId != WindowId::MainMenu || event.type != UIEventType::ActionSelected)
            continue;

        m_transitioning = true;

        switch (event.actionId)
        {
        case ActionId::StartGame:
        {
            PartyContext &ctx = PartyContext::instance();
            ctx.ensureInitialized();
            // A New Game must never inherit a previous campaign's gold/roster/
            // party/inventory — reset the singleton to a fresh state first
            // (gold persists across New Game was the bug this fixes).
            ctx.resetForNewGame();
            m_stateMachine.replace(std::make_unique<IntroState>(m_stateMachine, m_renderer));
            return;
        }
        case ActionId::ContinueGame:
        {
            // Resume straight into the most recent campaign save — no save
            // list. Mirrors the load-save path below (loadLatestCampaign is
            // SaveManager's newest-campaign helper).
            CampaignSave save;
            PartyContext &ctx = PartyContext::instance();
            ctx.ensureInitialized();
            if (SaveManager::instance().loadLatestCampaign(save) && ctx.applyCampaign(save))
            {
                m_stateMachine.replace(std::make_unique<WorldMapState>(m_stateMachine, m_renderer, save.worldNodeId));
                return;
            }
            m_transitioning = false;
            break;
        }
        case ActionId::LoadGame:
        {
            openSaveList();
            m_transitioning = false;
            break;
        }
        case ActionId::OpenSettings:
        {
            m_stateMachine.push(std::make_unique<SettingsState>(m_stateMachine, m_renderer));
            m_transitioning = false;
            break;
        }
        case ActionId::QuitGame:
        {
            App::requestQuit();
            return;
        }

        default:
        {
            m_transitioning = false;
            break;
        }
        }
    }
}

void MainMenuState::openSaveList()
{
    SaveManager::instance().loadIndex();

    std::vector<ButtonMenuWindow::Item> items;
    std::vector<int> slotIndexes;
    for (const SaveSlotInfo &info : SaveManager::instance().slots())
    {
        if (info.kind != SaveKind::Campaign)
            continue;
        items.push_back(ButtonMenuWindow::Item{.id = ActionId::LoadGame, .label = info.label, .enabled = true});
        slotIndexes.push_back(info.slotIndex);
    }
    if (items.empty())
        return;

    m_saveListSlotIndexes = std::move(slotIndexes);
    if (m_mainMenuWindow)
        m_mainMenuWindow->setVisible(false);

    m_saveListWindow = m_uiManager.push<ButtonMenuWindow>(WindowId::SaveList);
    m_saveListWindow->setFont(FontManager::instance().get(FontRole::Heading));
    m_saveListWindow->setPanelPosition(Vec2f{0.0f, GameConstants::VIEW_CY - 40.0f});
    m_saveListWindow->centerHorizontally(true);
    m_saveListWindow->setItems(std::move(items));
    m_saveListWindow->setDeleteMode(ActionId::DeleteSave, "X: Delete save file");
}

void MainMenuState::refreshSaveList()
{
    if (!m_saveListWindow)
        return;

    std::vector<ButtonMenuWindow::Item> items;
    std::vector<int> slotIndexes;
    for (const SaveSlotInfo &info : SaveManager::instance().slots())
    {
        if (info.kind != SaveKind::Campaign)
            continue;
        items.push_back(ButtonMenuWindow::Item{.id = ActionId::LoadGame, .label = info.label, .enabled = true});
        slotIndexes.push_back(info.slotIndex);
    }
    m_saveListSlotIndexes = std::move(slotIndexes);
    if (items.empty())
    {
        closeSaveList();
        return;
    }
    m_saveListWindow->setItems(std::move(items));
}

void MainMenuState::closeSaveList()
{
    m_uiManager.popById(WindowId::SaveList);
    m_saveListWindow = nullptr;
    m_saveListSlotIndexes.clear();
    m_pendingDeleteSlotIndex = -1;
    if (m_mainMenuWindow)
    {
        m_mainMenuWindow->setVisible(true);
        refreshMainMenuItems();
    }
}

void MainMenuState::refreshMainMenuItems()
{
    if (!m_mainMenuWindow)
        return;

    // Continue sits at the very top whenever any save exists — being the
    // first enabled row, FocusGroup lands the cursor on it automatically
    // (auto-focus for free). It stays grayed out until a campaign save
    // (battle-save-only states can't be resumed from here).
    std::vector<ButtonMenuWindow::Item> items;
    if (SaveManager::instance().hasSave())
        items.push_back(ButtonMenuWindow::Item{.id = ActionId::ContinueGame, .label = "Continue", .enabled = SaveManager::instance().latestCampaignSlot() >= 0});
    items.push_back(ButtonMenuWindow::Item{.id = ActionId::StartGame, .label = "New Game", .enabled = true});
    items.push_back(ButtonMenuWindow::Item{.id = ActionId::LoadGame, .label = "Load Game", .enabled = SaveManager::instance().latestCampaignSlot() >= 0});
    items.push_back(ButtonMenuWindow::Item{.id = ActionId::OpenSettings, .label = "Options", .enabled = true});
    items.push_back(ButtonMenuWindow::Item{.id = ActionId::QuitGame, .label = "Quit", .enabled = true});
    m_mainMenuWindow->setItems(std::move(items));
}
