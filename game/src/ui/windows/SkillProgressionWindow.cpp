#include "ui/windows/SkillProgressionWindow.h"

#include "config/GameConstants.h"
#include "data/UnitLoader.h"
#include "engine/core/Log.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Rect.h"
#include "engine/renderer/Aligment.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/FontManager.h"
#include "engine/renderer/Renderer.h"
#include "engine/ui/Insets.h"
#include "systems/RosterSystem.h"
#include "ui/ActionId.h"
#include "ui/UITheme.h"

#include <algorithm>

namespace
{
    constexpr float kPanelW = GameConstants::VIEW_W - 80.0f;
    constexpr float kPanelH = GameConstants::VIEW_H - 64.0f;
    constexpr float kLineH = 30.0f;

    const Color kLearnedColor{0, 255, 0, 255};
    const Color kEquippedColor{255, 200, 60, 255};
    const Color kDisabledColor{120, 120, 120, 255};
}

SkillProgressionWindow::SkillProgressionWindow(WindowId id, const RosterUnit &unit, RosterSystem &roster)
    : UIWindow(id, true, false),
      m_roster(&roster),
      m_instanceId(unit.instanceId),
      m_baseClass(unit.baseClass),
      m_promotion(unit.skills.getPromotion())
{
    m_name = unit.customName.empty() ? loadUnitDisplayName(unit.templatePath) : unit.customName;
    if (m_name.empty())
        m_name = "Unit";
    rebuildRows();
}

void SkillProgressionWindow::rebuildRows()
{
    m_skills = availableProgressionSkills(m_baseClass, m_promotion);

    m_rows.clear();
    m_rows.reserve(m_skills.size());
    std::vector<IFocusable *> pointers;
    pointers.reserve(m_skills.size());

    for (std::size_t i = 0; i < m_skills.size(); ++i)
    {
        auto row = std::make_unique<Button>(Rectf{0.0f, 0.0f, 0.0f, 0.0f}, "");
        row->setOnClick([this]()
                        { actOnSelection(); });
        pointers.push_back(row.get());
        m_rows.push_back(std::move(row));
    }
    m_focus.resetFromPointers(std::move(pointers));
}

void SkillProgressionWindow::syncSkillsClass()
{
    RosterUnit *unit = m_roster->findById(m_instanceId);
    if (!unit)
        return;
    if (unit->skills.getBaseClass() != m_baseClass ||
        unit->skills.getPromotion() != m_promotion)
    {
        unit->skills.setClass(m_baseClass, m_promotion);
    }
}

void SkillProgressionWindow::actOnSelection()
{
    const int index = m_focus.getSelectedIndex();
    if (index < 0 || index >= static_cast<int>(m_skills.size()))
        return;

    RosterUnit *unit = m_roster->findById(m_instanceId);
    if (!unit)
        return;
    syncSkillsClass();

    const std::string &skillId = m_skills[static_cast<std::size_t>(index)]->id;
    if (m_mode == Mode::Learn)
    {
        if (!unit->skills.canLearnOrUpgrade(skillId, unit->skillPoints))
        {
            cancelPending();
            return;
        }
        // Skill points are permanent: require a second Enter on the same row
        // before anything is spent, so the player explicitly confirms.
        if (m_pending == PendingMode::LearnConfirm && m_pendingSkillIndex == index)
        {
            commitLearn(index);
            cancelPending();
        }
        else
        {
            m_pending = PendingMode::LearnConfirm;
            m_pendingSkillIndex = index;
        }
        return;
    }

    if (!unit->skills.isLearned(skillId))
        return;
    if (unit->skills.isEquipped(skillId))
        unit->skills.unequip(skillId);
    else if (static_cast<int>(unit->skills.equippedActive().size()) < unit->skills.activeEquipLimit())
        unit->skills.equip(skillId);
}

void SkillProgressionWindow::commitLearn(int index)
{
    if (index < 0 || index >= static_cast<int>(m_skills.size()))
        return;
    RosterUnit *unit = m_roster->findById(m_instanceId);
    if (!unit)
        return;
    syncSkillsClass();
    const std::string &skillId = m_skills[static_cast<std::size_t>(index)]->id;
    if (unit->skills.learnOrUpgrade(skillId, unit->skillPoints))
    {
        // Auto-equip the skill if there's a free active slot so the
        // purchase has immediate value; the player can rebalance later.
        if (!unit->skills.isEquipped(skillId) &&
            static_cast<int>(unit->skills.equippedActive().size()) < unit->skills.activeEquipLimit())
        {
            unit->skills.equip(skillId);
        }
    }
}

void SkillProgressionWindow::cancelPending()
{
    m_pending = PendingMode::None;
    m_pendingSkillIndex = -1;
}

void SkillProgressionWindow::handleInput(const Input &input)
{
    if (input.isKeyPressed(KeyCode::Up, false) || input.isKeyPressed(KeyCode::W, false))
    {
        cancelPending();
        m_focus.focusPrevious();
    }
    else if (input.isKeyPressed(KeyCode::Down, false) || input.isKeyPressed(KeyCode::S, false))
    {
        cancelPending();
        m_focus.focusNext();
    }
    else if (input.isKeyPressed(KeyCode::Details, false))
    {
        cancelPending();
        m_mode = (m_mode == Mode::Learn) ? Mode::Equip : Mode::Learn;
    }
    else if (input.isKeyPressed(KeyCode::Accept, false))
    {
        (void)m_focus.activateSelected();
    }
    else if (input.isKeyPressed(KeyCode::Back, false))
    {
        if (m_pending != PendingMode::None)
        {
            cancelPending();
        }
        else
        {
            emit(UIEvent{.type = UIEventType::ActionCanceled, .windowId = id(), .actionId = ActionId::Close});
        }
    }
}

void SkillProgressionWindow::update(float dt)
{
    (void)dt;
}

void SkillProgressionWindow::render(Renderer *renderer) const
{
    if (!renderer || !m_font)
        return;

    const float panelX = (GameConstants::VIEW_W - kPanelW) * 0.5f;
    const float panelY = (GameConstants::VIEW_H - kPanelH) * 0.5f;
    const Insets outer{18.0f, 24.0f, 18.0f, 24.0f};
    const float contentX = panelX + outer.left;
    const float contentY = panelY + outer.top;
    const float contentW = kPanelW - outer.left - outer.right;

    renderer->setBlendMode(Renderer::BlendMode::Blend);
    renderer->setDrawColor(UITheme::PopupBG);
    renderer->fillRect(Rectf{panelX, panelY, kPanelW, kPanelH});
    renderer->setDrawColor(UITheme::PopupBorder);
    renderer->drawRect(Rectf{panelX, panelY, kPanelW, kPanelH});

    const Font *headingFont = FontManager::instance().get(FontRole::Heading);
    if (!headingFont)
        headingFont = m_font;

    renderer->renderTextInRect(headingFont, "Skills — " + m_name,
                               Rectf{contentX, contentY, contentW, 34.0f},
                               UITheme::SelectedText, HorizontalAlign::Left, VerticalAlign::Middle, false, false, false);

    const RosterUnit *unit = m_roster->findById(m_instanceId);
    const int level = unit ? unit->level : 1;
    const int points = unit ? unit->skillPoints : 0;
    const char *modeText = (m_mode == Mode::Learn) ? "Learn" : "Equip";

    renderer->renderTextInRect(m_font,
                               toString(m_baseClass) + std::string("   Lv ") + std::to_string(level) +
                                   "   " + std::to_string(points) + " skill point(s)   Mode: " + modeText,
                               Rectf{contentX, contentY + 40.0f, contentW, 26.0f},
                               UITheme::Info, HorizontalAlign::Left, VerticalAlign::Middle, false, false, false);

    float rowY = contentY + 82.0f;
    for (std::size_t index = 0; index < m_skills.size(); ++index)
    {
        const ProgressionSkillDefinition &skill = *m_skills[index];
        const bool selected = (static_cast<int>(index) == m_focus.getSelectedIndex());
        const bool rowPending = (m_pending == PendingMode::LearnConfirm &&
                                 static_cast<int>(index) == m_pendingSkillIndex);
        const int learned = unit ? unit->skills.levelOf(skill.id) : 0;
        const bool maxed = learned >= skill.maxLevel;
        const bool equipped = unit && unit->skills.isEquipped(skill.id);

        if (selected || rowPending)
        {
            renderer->setDrawColor(selected ? Color{80, 96, 122, 140} : Color{110, 110, 60, 150});
            renderer->fillRect(Rectf{contentX, rowY - 2.0f, contentW, kLineH});
        }
        if (rowPending)
        {
            renderer->setDrawColor(UITheme::Gold);
            renderer->drawRect(Rectf{contentX, rowY - 2.0f, contentW, kLineH});
        }

        const bool usable = (m_mode == Mode::Equip) ? learned > 0 : (learned < skill.maxLevel);
        const Color nameColor = maxed               ? kLearnedColor
                                : rowPending        ? UITheme::Gold
                                : usable            ? (selected ? UITheme::SelectedText : UITheme::Text)
                                                    : kDisabledColor;

        renderer->renderTextInRect(m_font, skill.name,
                                   Rectf{contentX + 12.0f, rowY, contentW * 0.42f, kLineH},
                                   nameColor, HorizontalAlign::Left, VerticalAlign::Middle, false, false, false);

        renderer->renderTextInRect(m_font, "[Lv " + std::to_string(learned) + "/" + std::to_string(skill.maxLevel) + "]",
                                   Rectf{contentX + contentW * 0.42f, rowY, contentW * 0.18f, kLineH},
                                   maxed ? kLearnedColor : UITheme::Info,
                                   HorizontalAlign::Left, VerticalAlign::Middle, false, false, false);

        std::string rightText;
        if (m_mode == Mode::Equip)
        {
            if (equipped)
            {
                rightText = "Equipped";
            }
            else if (learned > 0)
            {
                rightText = "Equip";
            }
        }
        else if (rowPending)
        {
            rightText = "Enter to confirm";
        }
        else if (maxed)
        {
            rightText = "Learned";
        }
        else
        {
            const std::size_t costIndex = static_cast<std::size_t>(learned);
            const int cost = costIndex < skill.pointCosts.size() ? skill.pointCosts[costIndex] : 0;
            rightText = (cost > 0 ? std::to_string(cost) + " pt" : "Learn");
        }

        if (!rightText.empty())
        {
            const bool canAfford = unit && unit->skills.canLearnOrUpgrade(skill.id, unit->skillPoints);
            const Color rightColor = equipped           ? kEquippedColor
                                     : maxed            ? kLearnedColor
                                     : rowPending       ? UITheme::Gold
                                     : (m_mode == Mode::Equip) ? UITheme::Text
                                     : canAfford         ? UITheme::Text
                                                         : kDisabledColor;
            renderer->renderTextInRect(m_font, rightText,
                                       Rectf{contentX + contentW - 130.0f, rowY, 118.0f, kLineH},
                                       rightColor, HorizontalAlign::Right, VerticalAlign::Middle, false, false, false);
        }

        rowY += kLineH + 2.0f;
    }

    const float bannerY = panelY + kPanelH - outer.bottom - 82.0f;
    if (m_pending == PendingMode::LearnConfirm &&
        m_pendingSkillIndex >= 0 && m_pendingSkillIndex < static_cast<int>(m_skills.size()))
    {
        const ProgressionSkillDefinition &pendingSkill = *m_skills[static_cast<std::size_t>(m_pendingSkillIndex)];
        const int pendingLearned = unit ? unit->skills.levelOf(pendingSkill.id) : 0;
        const std::size_t pendingCostIndex = static_cast<std::size_t>(pendingLearned);
        const int pendingCost = pendingCostIndex < pendingSkill.pointCosts.size()
                                    ? pendingSkill.pointCosts[pendingCostIndex]
                                    : 0;

        renderer->setDrawColor(UITheme::Panel);
        renderer->fillRect(Rectf{contentX, bannerY - 8.0f, contentW, 52.0f});
        renderer->setDrawColor(UITheme::Gold);
        renderer->drawRect(Rectf{contentX, bannerY - 8.0f, contentW, 52.0f});

        const std::string confirmLine = "Learn " + pendingSkill.name +
                                        (pendingCost > 0 ? " for " + std::to_string(pendingCost) + " point?" : "?") +
                                        "  Skill points are permanent — this cannot be undone.";
        renderer->renderTextInRect(m_font, confirmLine,
                                   Rectf{contentX + 12.0f, bannerY, contentW - 24.0f, 34.0f},
                                   UITheme::SelectedText, HorizontalAlign::Left, VerticalAlign::Middle, false, false, false);
    }

    const std::string help = (m_pending == PendingMode::LearnConfirm)
                             ? "Enter: Confirm learn (permanent)   Esc: Cancel   Tab: Mode"
                             : "W/S: Navigate | Tab: Mode | Enter: " +
                                   std::string(m_mode == Mode::Learn ? "Learn skill" : "Toggle equip") +
                                   " | Esc: Back";
    renderer->renderTextInRect(m_font, help,
                               Rectf{contentX, panelY + kPanelH - outer.bottom - 22.0f, contentW, 22.0f},
                               UITheme::Info, HorizontalAlign::Center, VerticalAlign::Middle, false, false, false);
}