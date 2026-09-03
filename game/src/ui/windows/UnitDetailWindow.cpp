#include "ui/windows/UnitDetailWindow.h"

#include "battle/unit/Unit.h"
#include "battle/unit/UnitProgression.h"
#include "config/GameConstants.h"
#include "data/UnitLoader.h"
#include "engine/core/Log.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/renderer/Aligment.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/FontManager.h"
#include "engine/renderer/Renderer.h"
#include "engine/ui/FocusGroup.h"
#include "engine/ui/HorizontalLayout.h"
#include "engine/ui/Insets.h"
#include "engine/ui/TextWrap.h"
#include "engine/ui/VerticalLayout.h"
#include "inventory/GearCatalog.h"
#include "inventory/Inventory.h"
#include "systems/RosterSystem.h"
#include "ui/ActionId.h"
#include "ui/UITheme.h"
#include "ui/UnitPortrait.h"

#include <algorithm>
#include <array>
#include <string>
#include <utility>

namespace
{
    constexpr float kPanelW = GameConstants::VIEW_W - 80.0f;
    constexpr float kPanelH = GameConstants::VIEW_H - 64.0f;
    constexpr float kHeaderH = 134.0f;
    constexpr float kLineH = 26.0f;
    constexpr float kColumnGap = 48.0f;

    class CloseFocusable final : public IFocusable
    {
    public:
        bool activate() const override { return true; }
        void setSelected(bool selected) override { m_selected = selected; }
        bool isEnabled() const override { return true; }
        bool selected() const { return m_selected; }

    private:
        bool m_selected = false;
    };

    std::string itemName(const Gear *gear)
    {
        return gear ? gear->displayName : "Empty";
    }
}

// Read-only constructor (for roster unit inspection)
UnitDetailWindow::UnitDetailWindow(WindowId id, const RosterUnit &rosterUnit, const GearCatalog &catalog)
    : UIWindow(id, true, false),
      m_isInteractive(false),
      m_isPlayerOwned(true),
      m_rosterInstanceId(rosterUnit.instanceId),
      m_catalog(&catalog),
      m_loadout(std::make_unique<EquipmentLoadout>(catalog.resolve(rosterUnit.equippedGear))),
      m_exp(rosterUnit.exp)
{
    try
    {
        UnitData data = UnitLoader::load(rosterUnit.templatePath);
        const RaceData &raceData = getRaceData(data.race);
        const GenderData &genderData = getGenderData(data.gender);
        m_unit = std::make_unique<Unit>(data, raceData, genderData, Vec2i{0, 0});
        m_unit->bindEquipmentLoadout(m_loadout.get());
        m_className = (data.className.empty() || data.className == "Unknown") ? toString(data.baseClass) : data.className;
    }
    catch (...)
    {
        m_className = "Unknown";
    }

    m_slots = {
        SlotEntry{.slot = GearSlot::Weapon, .accessoryIndex = -1, .label = "Weapon"},
        SlotEntry{.slot = GearSlot::Offhand, .accessoryIndex = -1, .label = "Offhand"},
        SlotEntry{.slot = GearSlot::Head, .accessoryIndex = -1, .label = "Head"},
        SlotEntry{.slot = GearSlot::Body, .accessoryIndex = -1, .label = "Body"},
        SlotEntry{.slot = GearSlot::Accessory, .accessoryIndex = 0, .label = "Accessory 1"},
        SlotEntry{.slot = GearSlot::Accessory, .accessoryIndex = 1, .label = "Accessory 2"},
    };

    m_state = UIState::Details;
}

// Live unit constructor (for in-battle unit inspection)
UnitDetailWindow::UnitDetailWindow(WindowId id, const Unit &unit, const GearCatalog &catalog)
    : UIWindow(id, true, false),
      m_isInteractive(false),
      m_isPlayerOwned(unit.getTeam() == 0),
      m_rosterInstanceId(-1),
      m_catalog(&catalog),
      m_loadout(std::make_unique<EquipmentLoadout>(unit.equipmentLoadout() ? *unit.equipmentLoadout() : EquipmentLoadout{})),
      m_exp(unit.getExp())
{
    const UnitData &data = unit.getData();
    const RaceData &raceData = getRaceData(unit.getRace());
    const GenderData &genderData = getGenderData(unit.getGender());
    m_unit = std::make_unique<Unit>(data, raceData, genderData, unit.getPosition());
    m_unit->bindEquipmentLoadout(m_loadout.get());
    m_className = (data.className.empty() || data.className == "Unknown") ? toString(data.baseClass) : data.className;

    m_slots = {
        SlotEntry{.slot = GearSlot::Weapon, .accessoryIndex = -1, .label = "Weapon"},
        SlotEntry{.slot = GearSlot::Offhand, .accessoryIndex = -1, .label = "Offhand"},
        SlotEntry{.slot = GearSlot::Head, .accessoryIndex = -1, .label = "Head"},
        SlotEntry{.slot = GearSlot::Body, .accessoryIndex = -1, .label = "Body"},
        SlotEntry{.slot = GearSlot::Accessory, .accessoryIndex = 0, .label = "Accessory 1"},
        SlotEntry{.slot = GearSlot::Accessory, .accessoryIndex = 1, .label = "Accessory 2"},
    };

    m_state = UIState::Details;
}

// Interactive constructor (for equipment management)
UnitDetailWindow::UnitDetailWindow(WindowId id,
                                   const RosterUnit &rosterUnit,
                                   RosterSystem &roster,
                                   Inventory &inventory,
                                   const GearCatalog &catalog)
    : UIWindow(id, true, false),
      m_isInteractive(true),
      m_isPlayerOwned(true),
      m_roster(&roster),
      m_inventory(&inventory),
      m_catalog(&catalog),
      m_rosterInstanceId(rosterUnit.instanceId),
      m_loadout(std::make_unique<EquipmentLoadout>(catalog.resolve(rosterUnit.equippedGear))),
      m_exp(rosterUnit.exp),
      m_slotFocus(std::make_unique<FocusGroup>()),
      m_itemFocus(std::make_unique<FocusGroup>())
{
    try
    {
        UnitData data = UnitLoader::load(rosterUnit.templatePath);
        const RaceData &raceData = getRaceData(data.race);
        const GenderData &genderData = getGenderData(data.gender);
        m_unit = std::make_unique<Unit>(data, raceData, genderData, Vec2i{0, 0});
        m_unit->bindEquipmentLoadout(m_loadout.get());
        m_previewUnit = std::make_unique<Unit>(data, raceData, genderData, Vec2i{0, 0});
        m_previewLoadout = std::make_unique<EquipmentLoadout>(*m_loadout);
        m_previewUnit->bindEquipmentLoadout(m_previewLoadout.get());
        m_className = (data.className.empty() || data.className == "Unknown") ? toString(data.baseClass) : data.className;
    }
    catch (...)
    {
        m_className = "Unknown";
    }

    // Build slot list for interactive mode
    m_slots = {
        SlotEntry{.slot = GearSlot::Weapon, .accessoryIndex = -1, .label = "Weapon"},
        SlotEntry{.slot = GearSlot::Offhand, .accessoryIndex = -1, .label = "Offhand"},
        SlotEntry{.slot = GearSlot::Head, .accessoryIndex = -1, .label = "Head"},
        SlotEntry{.slot = GearSlot::Body, .accessoryIndex = -1, .label = "Body"},
        SlotEntry{.slot = GearSlot::Accessory, .accessoryIndex = 0, .label = "Accessory 1"},
        SlotEntry{.slot = GearSlot::Accessory, .accessoryIndex = 1, .label = "Accessory 2"},
    };

    m_state = UIState::Details;
}

void UnitDetailWindow::setState(UIState newState)
{
    if (m_state == UIState::ItemSelect)
        exitItemSelect();
    if (m_state == UIState::SlotSelect)
        exitSlotSelect();

    m_state = newState;

    if (m_state == UIState::SlotSelect)
        enterSlotSelect();
    if (m_state == UIState::ItemSelect)
        enterItemSelect();
}

void UnitDetailWindow::enterSlotSelect()
{
    // Restore the slot the user was on before entering ItemSelect, so
    // equipping into Helmet returns to the Helmet row rather than
    // jumping back to Weapon (the bug Part H reports). The remembered
    // index is cleared after the first restore so a future direct
    // navigation to SlotSelect (Details → SlotSelect) starts at Weapon.
    if (m_lastSelectedSlotIndex >= 0 && m_lastSelectedSlotIndex < static_cast<int>(m_slots.size()))
    {
        m_selectedSlotIndex = m_lastSelectedSlotIndex;
        m_lastSelectedSlotIndex = -1;
    }
    else
    {
        m_selectedSlotIndex = 0;
    }
    m_showItemDescription = false;
}

void UnitDetailWindow::exitSlotSelect()
{
}

void UnitDetailWindow::enterItemSelect()
{
    rebuildCandidatesList();
    m_selectedItemIndex = 0;
    m_candidateScroll = 0;
    m_lastSelectedSlotIndex = m_selectedSlotIndex;
    m_showItemDescription = false;
}

void UnitDetailWindow::exitItemSelect()
{
}

const Gear *UnitDetailWindow::equippedGearAt(const SlotEntry &slot) const
{
    if (slot.slot == GearSlot::Accessory)
    {
        return slot.accessoryIndex >= 0 && slot.accessoryIndex < static_cast<int>(m_loadout->accessories.size())
                   ? m_loadout->accessories[static_cast<std::size_t>(slot.accessoryIndex)]
                   : nullptr;
    }

    const std::size_t index = static_cast<std::size_t>(slot.slot);
    return index < m_loadout->slots.size() ? m_loadout->slots[index] : nullptr;
}

void UnitDetailWindow::setEquippedGearAt(const SlotEntry &slot, const Gear *gear)
{
    if (slot.slot == GearSlot::Accessory)
    {
        if (slot.accessoryIndex >= 0 && slot.accessoryIndex < static_cast<int>(m_loadout->accessories.size()))
            m_loadout->accessories[static_cast<std::size_t>(slot.accessoryIndex)] = gear;
        return;
    }

    const std::size_t index = static_cast<std::size_t>(slot.slot);
    if (index < m_loadout->slots.size())
        m_loadout->slots[index] = gear;
}

EquipmentLoadout UnitDetailWindow::loadoutWithoutSelectedSlot() const
{
    EquipmentLoadout candidateLoadout = *m_loadout;
    if (m_selectedSlotIndex < 0 || m_selectedSlotIndex >= static_cast<int>(m_slots.size()))
        return candidateLoadout;

    const SlotEntry &slot = m_slots[static_cast<std::size_t>(m_selectedSlotIndex)];
    if (slot.slot == GearSlot::Accessory)
    {
        if (slot.accessoryIndex >= 0 && slot.accessoryIndex < static_cast<int>(candidateLoadout.accessories.size()))
            candidateLoadout.accessories[static_cast<std::size_t>(slot.accessoryIndex)] = nullptr;
    }
    else
    {
        const std::size_t index = static_cast<std::size_t>(slot.slot);
        if (index < candidateLoadout.slots.size())
            candidateLoadout.slots[index] = nullptr;
    }
    return candidateLoadout;
}

bool UnitDetailWindow::isSlotEligible(GearSlot slot) const
{
    if (!m_unit)
        return false;

    if (!EquipRules::isSlotEnabled(m_unit->getRace(), slot))
        return false;

    // Offhand has special eligibility based on main-hand weapon
    if (slot == GearSlot::Offhand)
    {
        const Gear *weapon = m_loadout->slots[static_cast<std::size_t>(GearSlot::Weapon)];
        if (!weapon)
            return true; // No main-hand means Offhand is available (but can't equip non-shield/non-weapon there)
        // Offhand is eligible only if main-hand is one-handed non-ranged
        return weapon->weaponHandedness() == WeaponHandedness::OneHanded && !weapon->isRanged();
    }

    return true;
}

void UnitDetailWindow::rebuildCandidatesList()
{
    m_candidates.clear();
    if (!m_inventory || m_selectedSlotIndex < 0 || m_selectedSlotIndex >= static_cast<int>(m_slots.size()))
        return;

    std::unordered_set<ItemId> seen;
    const SlotEntry &slot = m_slots[static_cast<std::size_t>(m_selectedSlotIndex)];
    const EquipmentLoadout candidateLoadout = loadoutWithoutSelectedSlot();

    for (const ItemStack &stack : m_inventory->stacks())
    {
        if (!seen.insert(stack.itemId).second)
            continue;

        const Gear *gear = m_catalog->find(stack.itemId);
        if (!gear)
            continue;

        // Slot matching: either the gear's tagged slot matches the
        // selected slot, or (for the Offhand case) the gear is a valid
        // off-hand candidate under the current main-hand state. Without
        // the second clause dual-wield weapons (Sword/Mace, slot==Weapon)
        // would never appear in the Offhand item list, which is the bug
        // Part J reports.
        const bool slotMatches = (slot.slot == gear->slot());
        const bool offhandCandidate = (slot.slot == GearSlot::Offhand &&
                                       EquipRules::isOffhandEligibleGear(*gear, candidateLoadout));
        if (!slotMatches && !offhandCandidate)
            continue;

        if (!EquipRules::canEquip(m_unit->getRace(), *gear, candidateLoadout))
            continue;

        m_candidates.push_back(ItemCandidate{
            .itemId = stack.itemId,
            .gear = gear,
            .quantity = stack.quantity,
        });
    }

    if (m_selectedItemIndex >= static_cast<int>(m_candidates.size()))
        m_selectedItemIndex = std::max(0, static_cast<int>(m_candidates.size()) - 1);
}

void UnitDetailWindow::unequipCurrentSlot()
{
    if (!m_isInteractive || !m_roster || !m_inventory || m_selectedSlotIndex < 0 ||
        m_selectedSlotIndex >= static_cast<int>(m_slots.size()))
        return;

    const SlotEntry &slot = m_slots[static_cast<std::size_t>(m_selectedSlotIndex)];
    if (m_roster->unequip(m_rosterInstanceId, slot.slot, slot.accessoryIndex, *m_inventory, *m_catalog))
    {
        if (const RosterUnit *unit = m_roster->findById(m_rosterInstanceId))
            *m_loadout = m_roster->resolveLoadout(*unit, *m_catalog);
        if (m_unit)
            m_unit->bindEquipmentLoadout(m_loadout.get());

        if (m_state == UIState::ItemSelect)
        {
            rebuildCandidatesList();
        }
    }
}

void UnitDetailWindow::confirmItemSelection()
{
    if (!m_isInteractive || !m_roster || !m_inventory || m_selectedSlotIndex < 0 ||
        m_selectedSlotIndex >= static_cast<int>(m_slots.size()))
        return;

    if (m_selectedItemIndex < 0 || m_selectedItemIndex >= static_cast<int>(m_candidates.size()))
        return;

    const SlotEntry &slot = m_slots[static_cast<std::size_t>(m_selectedSlotIndex)];
    const ItemCandidate &candidate = m_candidates[static_cast<std::size_t>(m_selectedItemIndex)];
    const bool changed = m_roster->equip(m_rosterInstanceId, slot.slot, slot.accessoryIndex,
                                         candidate.itemId, *m_inventory, *m_catalog, m_unit->getRace());

    if (changed)
    {
        // Refresh loadout from roster
        if (const RosterUnit *unit = m_roster->findById(m_rosterInstanceId))
            *m_loadout = m_roster->resolveLoadout(*unit, *m_catalog);
        if (m_unit)
            m_unit->bindEquipmentLoadout(m_loadout.get());
        // Return to slot select
        setState(UIState::SlotSelect);
    }
}

void UnitDetailWindow::handleInput(const Input &input)
{
    switch (m_state)
    {
    case UIState::Details:
    {
        const int actionCount = (m_isInteractive && m_isPlayerOwned) ? 3 : 2;

        if (input.isKeyPressed(KeyCode::Up, false) || input.isKeyPressed(KeyCode::W, false))
        {
            if (m_selectedActionIndex > 0)
                --m_selectedActionIndex;
        }
        else if (input.isKeyPressed(KeyCode::Down, false) || input.isKeyPressed(KeyCode::S, false))
        {
            if (m_selectedActionIndex < actionCount - 1)
                ++m_selectedActionIndex;
        }
        else if (input.isKeyPressed(KeyCode::Accept, false))
        {
            if (m_selectedActionIndex == 0) // Equip (player interactive) or Gear (read-only/enemy)
            {
                setState(UIState::SlotSelect);
            }
            // Abilities (1) and Dismiss (2) are reserved for future tasks
        }
        else if (input.isKeyPressed(KeyCode::Back, false))
        {
            LOG_INFO("UnitDetailWindow", "Back pressed in Details state -> Emitting ActionCanceled (Close)");
            emit(UIEvent{.type = UIEventType::ActionCanceled, .windowId = id(), .actionId = ActionId::Close});
        }
        break;
    }

    case UIState::SlotSelect:
    {
        if (m_showItemDescription)
        {
            if (input.isKeyPressed(KeyCode::Details, false) || input.isKeyPressed(KeyCode::Back, false) || input.isKeyPressed(KeyCode::Accept, false))
            {
                m_showItemDescription = false;
            }
            return;
        }

        // Hold-to-repeat (Part K): continuous Up/Down after the initial
        // press. Tab/Accept/Back/X keep their single-press semantics.
        const bool upHit   = m_slotUpRepeat.tick(m_lastDt,   input.isKeyDown(KeyCode::Up)   || input.isKeyDown(KeyCode::W));
        const bool downHit = m_slotDownRepeat.tick(m_lastDt, input.isKeyDown(KeyCode::Down) || input.isKeyDown(KeyCode::S));

        if (upHit)
        {
            if (m_selectedSlotIndex > 0)
                --m_selectedSlotIndex;
        }
        else if (downHit)
        {
            if (m_selectedSlotIndex < static_cast<int>(m_slots.size()) - 1)
                ++m_selectedSlotIndex;
        }
        else if (input.isKeyPressed(KeyCode::Details, false)) // Tab on equipped slot
        {
            if (m_selectedSlotIndex >= 0 && m_selectedSlotIndex < static_cast<int>(m_slots.size()))
            {
                const Gear *gear = equippedGearAt(m_slots[static_cast<std::size_t>(m_selectedSlotIndex)]);
                if (gear)
                {
                    m_showItemDescription = true;
                }
            }
        }
        else if (input.isKeyPressed(KeyCode::Accept, false))
        {
            if (m_isInteractive && m_isPlayerOwned && isSlotEligible(m_slots[m_selectedSlotIndex].slot))
                setState(UIState::ItemSelect);
        }
        else if (input.isKeyPressed(KeyCode::X, false))
        {
            if (m_isInteractive && m_isPlayerOwned)
                unequipCurrentSlot();
        }
        else if (input.isKeyPressed(KeyCode::Back, false))
        {
            LOG_INFO("UnitDetailWindow", "Back pressed in SlotSelect state -> Transitioning to Details state");
            setState(UIState::Details);
        }
        break;
    }

    case UIState::ItemSelect:
    {
        const int count = static_cast<int>(m_candidates.size());

        if (m_showItemDescription)
        {
            if (input.isKeyPressed(KeyCode::Details, false) || input.isKeyPressed(KeyCode::Back, false) || input.isKeyPressed(KeyCode::Accept, false))
            {
                m_showItemDescription = false;
            }
            return;
        }

        // Hold-to-repeat (Part K): Up/Down scroll the candidate list
        // continuously; A/D page-jump (5 rows at a time), Tab details,
        // Enter equip, X unequip, Back to SlotSelect. A/D uses the same
        // initial-delay/interval tuning as W/S for a consistent feel.
        const bool upHit    = m_itemUpRepeat.tick(m_lastDt,    input.isKeyDown(KeyCode::Up)    || input.isKeyDown(KeyCode::W));
        const bool downHit  = m_itemDownRepeat.tick(m_lastDt,  input.isKeyDown(KeyCode::Down)  || input.isKeyDown(KeyCode::S));
        const bool leftHit  = m_itemLeftRepeat.tick(m_lastDt,  input.isKeyDown(KeyCode::Left)  || input.isKeyDown(KeyCode::A));
        const bool rightHit = m_itemRightRepeat.tick(m_lastDt, input.isKeyDown(KeyCode::Right) || input.isKeyDown(KeyCode::D));

        if (count > 0)
        {
            if (upHit)
            {
                if (m_selectedItemIndex > 0)
                    --m_selectedItemIndex;
            }
            else if (downHit)
            {
                if (m_selectedItemIndex < count - 1)
                    ++m_selectedItemIndex;
            }
            else if (leftHit)
            {
                m_selectedItemIndex = std::max(0, m_selectedItemIndex - 5);
            }
            else if (rightHit)
            {
                m_selectedItemIndex = std::min(count - 1, m_selectedItemIndex + 5);
            }
            else if (input.isKeyPressed(KeyCode::Details, false)) // Tab opens read-only description of the candidate (Part I)
            {
                if (m_selectedItemIndex >= 0 && m_selectedItemIndex < static_cast<int>(m_candidates.size()))
                {
                    const ItemCandidate &cand = m_candidates[static_cast<std::size_t>(m_selectedItemIndex)];
                    if (cand.gear)
                        m_showItemDescription = true;
                }
            }
            else if (input.isKeyPressed(KeyCode::Accept, false))
            {
                confirmItemSelection();
            }

            constexpr int kVisibleCandidates = 6;
            if (m_selectedItemIndex < m_candidateScroll)
                m_candidateScroll = m_selectedItemIndex;
            if (m_selectedItemIndex >= m_candidateScroll + kVisibleCandidates)
                m_candidateScroll = m_selectedItemIndex - kVisibleCandidates + 1;
        }

        if (input.isKeyPressed(KeyCode::X, false))
        {
            unequipCurrentSlot();
        }
        else if (input.isKeyPressed(KeyCode::Back, false))
        {
            LOG_INFO("UnitDetailWindow", "Back pressed in ItemSelect state -> Transitioning to SlotSelect state");
            setState(UIState::SlotSelect);
        }
        break;
    }
    }
}

void UnitDetailWindow::update(float dt)
{
    // Refresh hold-to-repeat parameters (Part K). Each list gets its own
    // set of trackers so navigation in SlotSelect doesn't leak into
    // ItemSelect. W/S and A/D use the same tuning for a consistent feel.
    m_lastDt = dt;
    m_slotUpRepeat.start(0.35f, 0.09f);
    m_slotDownRepeat.start(0.35f, 0.09f);
    m_slotLeftRepeat.start(0.35f, 0.09f);
    m_slotRightRepeat.start(0.35f, 0.09f);
    m_itemUpRepeat.start(0.35f, 0.09f);
    m_itemDownRepeat.start(0.35f, 0.09f);
    m_itemLeftRepeat.start(0.35f, 0.09f);
    m_itemRightRepeat.start(0.35f, 0.09f);
}

void UnitDetailWindow::render(Renderer *renderer) const
{
    if (!renderer || !m_font || !m_unit)
        return;

    const float panelX = (GameConstants::VIEW_W - kPanelW) * 0.5f;
    const float panelY = (GameConstants::VIEW_H - kPanelH) * 0.5f;

    renderer->setBlendMode(Renderer::BlendMode::Blend);
    renderer->setDrawColor(UITheme::PopupBG);
    renderer->fillRect(Rectf{panelX, panelY, kPanelW, kPanelH});
    renderer->setDrawColor(UITheme::PopupBorder);
    renderer->drawRect(Rectf{panelX, panelY, kPanelW, kPanelH});

    switch (m_state)
    {
    case UIState::Details:
        renderDetailsPanel(renderer);
        break;
    case UIState::SlotSelect:
        renderSlotSelectPanel(renderer);
        break;
    case UIState::ItemSelect:
        renderItemSelectPanel(renderer);
        break;
    }
}

void UnitDetailWindow::renderIdentityHeader(Renderer *renderer, Vec2f contentPos, float contentW) const
{
    const HorizontalLayout::Container portraitColumn{
        .items = {{250.0f, kHeaderH, Insets{}}},
    };
    const HorizontalLayout::Container identityColumn{
        .items = {{contentW - 270.0f, kHeaderH, Insets{}}},
    };
    const auto header = HorizontalLayout::layoutContainers(
        {portraitColumn, identityColumn}, contentPos, 20.0f);

    UnitPortrait::render(renderer, m_font, *m_unit,
                         Vec2f{header[0].itemRects[0].x, header[0].itemRects[0].y},
                         UnitPortrait::PortraitStyle{.team = m_unit->getTeam()});

    const Rectf identityRect = header[1].itemRects[0];
    const Font *headingFont = FontManager::instance().get(FontRole::Heading);
    if (!headingFont)
        headingFont = m_font;

    renderer->renderTextInRect(headingFont, m_className,
                               Rectf{identityRect.x, identityRect.y + 12.0f, identityRect.w, 32.0f},
                               UITheme::SelectedText, HorizontalAlign::Left, VerticalAlign::Middle, false, false, false);
    renderer->renderTextInRect(m_font, "EXP  " + std::to_string(m_exp),
                               Rectf{identityRect.x, identityRect.y + 54.0f, identityRect.w, 24.0f},
                               UITheme::Info, HorizontalAlign::Left, VerticalAlign::Middle, false, false, false);
}

void UnitDetailWindow::renderStatsColumn(Renderer *renderer, Vec2f colPos, float columnW, const Unit *previewUnit) const
{
    const std::array<std::pair<const char *, int>, 10> stats = {{
        {"HP", m_unit->getMaxHp()},
        {"MP", m_unit->getMaxMp()},
        {"Attack", m_unit->getAttack()},
        {"Defense", m_unit->getDefense()},
        {"Magic", m_unit->getMagic()},
        {"Magic Def", m_unit->getMagicDefense()},
        {"Speed", m_unit->getSpeed()},
        {"Move", m_unit->getMoveRange()},
        {"Jump", m_unit->getJump()},
        {"Evasion", m_unit->getEvasion()},
    }};
    std::vector<VerticalLayout::Item> statItems(stats.size(), VerticalLayout::Item{.width = columnW, .height = kLineH});
    const auto statRows = VerticalLayout::layoutColumn(statItems, Vec2f{colPos.x, colPos.y + 30.0f});
    renderer->renderTextInRect(m_font, "Stats", Rectf{colPos.x, colPos.y, columnW, kLineH},
                               UITheme::SelectedText, HorizontalAlign::Left, VerticalAlign::Middle, false, false, false);

    const float padX = 10.0f;
    const float usableW = columnW - (2.0f * padX);
    const float labelW = std::floor(usableW * 0.44f);
    const float baseValW = std::floor(usableW * 0.26f);
    const float kDeltaGap = 6.0f;
    const float deltaW = usableW - labelW - baseValW;

    for (std::size_t index = 0; index < stats.size(); ++index)
    {
        const Rectf &row = statRows[index];
        const char *label = stats[index].first;
        const int baseVal = stats[index].second;

        const Rectf labelRect{row.x + padX, row.y, labelW, row.h};
        const Rectf baseValRect{row.x + padX + labelW, row.y, baseValW, row.h};
        // Column edge stays at the same x so rows without a delta keep
        // their right-alignment; the gap only narrows where the delta
        // text actually renders.
        const Rectf deltaRect{row.x + padX + labelW + baseValW + kDeltaGap, row.y, deltaW - kDeltaGap, row.h};

        // 1. Column 1: Label (left-aligned, fixed width)
        renderer->renderTextInRect(m_font, label, labelRect,
                                   UITheme::Text, HorizontalAlign::Left, VerticalAlign::Middle, false, false, false);

        // 2. Column 2: Base Value (right-aligned, fixed width)
        renderer->renderTextInRect(m_font, std::to_string(baseVal), baseValRect,
                                   UITheme::Text, HorizontalAlign::Right, VerticalAlign::Middle, false, false, false);

        // 3. Column 3: Delta (right-aligned, fixed width, reserved space even when empty)
        if (previewUnit)
        {
            const int previewVal = (label == std::string("HP")) ? previewUnit->getMaxHp() : (label == std::string("MP"))      ? previewUnit->getMaxMp()
                                                                                        : (label == std::string("Attack"))    ? previewUnit->getAttack()
                                                                                        : (label == std::string("Defense"))   ? previewUnit->getDefense()
                                                                                        : (label == std::string("Magic"))     ? previewUnit->getMagic()
                                                                                        : (label == std::string("Magic Def")) ? previewUnit->getMagicDefense()
                                                                                        : (label == std::string("Speed"))     ? previewUnit->getSpeed()
                                                                                        : (label == std::string("Move"))      ? previewUnit->getMoveRange()
                                                                                        : (label == std::string("Jump"))      ? previewUnit->getJump()
                                                                                                                              : previewUnit->getEvasion();

            const int delta = previewVal - baseVal;
            if (delta != 0)
            {
                const char *sign = delta > 0 ? "+" : "";
                const std::string deltaStr = "(" + std::string(sign) + std::to_string(delta) + ")";
                const Color deltaColor = delta > 0 ? Color{0, 255, 0, 255} : Color{255, 80, 80, 255};

                renderer->renderTextInRect(m_font, deltaStr, deltaRect,
                                           deltaColor, HorizontalAlign::Right, VerticalAlign::Middle, false, false, false);
            }
        }
    }
}

void UnitDetailWindow::renderDetailsPanel(Renderer *renderer) const
{
    const float panelX = (GameConstants::VIEW_W - kPanelW) * 0.5f;
    const float panelY = (GameConstants::VIEW_H - kPanelH) * 0.5f;
    const Insets outer{18.0f, 24.0f, 18.0f, 24.0f};
    const float contentX = panelX + outer.left;
    const float contentY = panelY + outer.top;
    const float contentW = kPanelW - outer.left - outer.right;

    renderIdentityHeader(renderer, Vec2f{contentX, contentY}, contentW);

    const float lowerY = contentY + kHeaderH + 18.0f;
    const float lowerH = kPanelH - outer.top - outer.bottom - kHeaderH - 58.0f;
    const float columnW = (contentW - kColumnGap) * 0.5f;

    renderStatsColumn(renderer, Vec2f{contentX, lowerY}, columnW);

    const float rightColX = contentX + columnW + kColumnGap;

    const bool showEquipActions = m_isPlayerOwned && m_isInteractive;
    const std::vector<const char *> actions = showEquipActions
                                                  ? std::vector<const char *>{"Equip", "Abilities", "Dismiss"}
                                                  : std::vector<const char *>{"Gear", "Abilities"};

    constexpr float kActionRowH = 36.0f;
    std::vector<VerticalLayout::Item> actionItems(actions.size(), VerticalLayout::Item{.width = columnW, .height = kActionRowH});
    const auto actionRows = VerticalLayout::layoutColumn(actionItems, Vec2f{rightColX, lowerY + 30.0f});

    const Font *actionFont = FontManager::instance().get(FontRole::Heading);
    if (!actionFont)
        actionFont = m_font;

    for (std::size_t index = 0; index < actions.size(); ++index)
    {
        const Rectf &row = actionRows[index];
        const bool isSelected = (static_cast<int>(index) == m_selectedActionIndex);
        const bool isEnabled = (index == 0); // Equip and Gear are active; Abilities & Dismiss are placeholders

        if (isSelected)
        {
            renderer->setDrawColor(Color{80, 96, 122, 140});
            renderer->fillRect(Rectf{row.x, row.y - 2.0f, row.w, row.h});
        }

        const Color textColor = isEnabled ? (isSelected ? UITheme::SelectedText : UITheme::Text)
                                          : Color{120, 120, 120, 255};
        renderer->renderTextInRect(actionFont, actions[index], Rectf{row.x + 14.0f, row.y, row.w - 28.0f, row.h},
                                   textColor,
                                   HorizontalAlign::Left, VerticalAlign::Middle, false, false, false);
    }

    renderer->renderTextInRect(m_font, "W/S: Navigate | Enter: Select | Esc: Back",
                               Rectf{contentX, panelY + kPanelH - outer.bottom - 22.0f, contentW, 22.0f},
                               UITheme::Info, HorizontalAlign::Center, VerticalAlign::Middle, false, false, false);
}

void UnitDetailWindow::renderSlotSelectPanel(Renderer *renderer) const
{
    const float panelX = (GameConstants::VIEW_W - kPanelW) * 0.5f;
    const float panelY = (GameConstants::VIEW_H - kPanelH) * 0.5f;
    const Insets outer{18.0f, 24.0f, 18.0f, 24.0f};
    const float contentX = panelX + outer.left;
    const float contentY = panelY + outer.top;
    const float contentW = kPanelW - outer.left - outer.right;

    renderIdentityHeader(renderer, Vec2f{contentX, contentY}, contentW);

    const float lowerY = contentY + kHeaderH + 18.0f;
    const float lowerH = kPanelH - outer.top - outer.bottom - kHeaderH - 58.0f;
    const float columnW = (contentW - kColumnGap) * 0.5f;

    renderStatsColumn(renderer, Vec2f{contentX, lowerY}, columnW);

    const float rightColX = contentX + columnW + kColumnGap;

    if (m_showItemDescription && m_selectedSlotIndex >= 0 && m_selectedSlotIndex < static_cast<int>(m_slots.size()))
    {
        const SlotEntry &slot = m_slots[static_cast<std::size_t>(m_selectedSlotIndex)];
        const Gear *gear = equippedGearAt(slot);
        if (gear)
        {
            renderItemDescriptionCard(renderer,
                                      Rectf{rightColX, lowerY, columnW, lowerH},
                                      slot, gear, "Tab/Esc: Close Description");
            renderer->renderTextInRect(m_font, "Tab/Esc: Close Description",
                                       Rectf{contentX, panelY + kPanelH - outer.bottom - 22.0f, contentW, 22.0f},
                                       UITheme::Info, HorizontalAlign::Center, VerticalAlign::Middle, false, false, false);
            return;
        }
    }

    renderer->renderTextInRect(m_font, "Slots", Rectf{rightColX, lowerY, columnW, kLineH},
                               UITheme::SelectedText, HorizontalAlign::Left, VerticalAlign::Middle, false, false, false);

    // Render slot list on right
    float slotY = lowerY + 30.0f;
    for (int index = 0; index < static_cast<int>(m_slots.size()); ++index)
    {
        const SlotEntry &slot = m_slots[static_cast<std::size_t>(index)];
        const bool selected = index == m_selectedSlotIndex;
        const bool enabled = isSlotEligible(slot.slot);

        if (selected && enabled)
        {
            renderer->setDrawColor(Color{80, 96, 122, 140});
            renderer->fillRect(Rectf{rightColX, slotY - 3.0f, columnW, 24.0f});
        }

        const Gear *gear = equippedGearAt(slot);
        const std::string itemName_str = itemName(gear);

        renderer->renderTextInRect(m_font, slot.label,
                                   Rectf{rightColX + 12.0f, slotY, 150.0f, 24.0f},
                                   enabled ? UITheme::Text : Color{120, 120, 120, 255},
                                   HorizontalAlign::Left, VerticalAlign::Middle, false, false, false);
        renderer->renderTextInRect(m_font, itemName_str,
                                   Rectf{rightColX + 180.0f, slotY, columnW - 192.0f, 24.0f},
                                   enabled ? UITheme::SelectedText : Color{120, 120, 120, 255},
                                   HorizontalAlign::Right, VerticalAlign::Middle, false, false, false);

        slotY += 26.0f;
    }

    const std::string helpText = (m_isInteractive && m_isPlayerOwned)
                                     ? "W/S: Navigate | Tab: Details | X: Unequip | Enter: Change | Esc: Back"
                                     : "W/S: Navigate | Tab: Details | Esc: Back";
    renderer->renderTextInRect(m_font, helpText,
                               Rectf{contentX, panelY + kPanelH - outer.bottom - 22.0f, contentW, 22.0f},
                               UITheme::Info, HorizontalAlign::Center, VerticalAlign::Middle, false, false, false);
}

void UnitDetailWindow::renderItemSelectPanel(Renderer *renderer) const
{
    if (!m_isInteractive || m_selectedSlotIndex < 0 || m_selectedSlotIndex >= static_cast<int>(m_slots.size()))
        return;

    const float panelX = (GameConstants::VIEW_W - kPanelW) * 0.5f;
    const float panelY = (GameConstants::VIEW_H - kPanelH) * 0.5f;
    const Insets outer{18.0f, 24.0f, 18.0f, 24.0f};
    const float contentX = panelX + outer.left;
    const float contentY = panelY + outer.top;
    const float contentW = kPanelW - outer.left - outer.right;

    // Fixed identity header across all states
    renderIdentityHeader(renderer, Vec2f{contentX, contentY}, contentW);

    const float lowerY = contentY + kHeaderH + 18.0f;
    const float lowerH = kPanelH - outer.top - outer.bottom - kHeaderH - 58.0f;
    const float columnW = (contentW - kColumnGap) * 0.5f;

    // Live preview unit calculation for hover
    const Unit *previewPtr = nullptr;
    if (m_previewLoadout && m_previewUnit)
    {
        *m_previewLoadout = *m_loadout;
        if (m_selectedItemIndex >= 0 && m_selectedItemIndex < static_cast<int>(m_candidates.size()))
        {
            const ItemCandidate &candidate = m_candidates[static_cast<std::size_t>(m_selectedItemIndex)];
            if (candidate.gear)
            {
                const SlotEntry &slot = m_slots[static_cast<std::size_t>(m_selectedSlotIndex)];
                if (slot.slot == GearSlot::Accessory)
                {
                    if (slot.accessoryIndex >= 0 && slot.accessoryIndex < static_cast<int>(m_previewLoadout->accessories.size()))
                        m_previewLoadout->accessories[static_cast<std::size_t>(slot.accessoryIndex)] = candidate.gear;
                }
                else
                {
                    const std::size_t sIdx = static_cast<std::size_t>(slot.slot);
                    if (sIdx < m_previewLoadout->slots.size())
                        m_previewLoadout->slots[sIdx] = candidate.gear;
                }
            }
        }
        m_previewUnit->bindEquipmentLoadout(m_previewLoadout.get());
        previewPtr = m_previewUnit.get();
    }

    // Left side: always-visible stats panel with live deltas!
    renderStatsColumn(renderer, Vec2f{contentX, lowerY}, columnW, previewPtr);

    // Right side: Breadcrumb header + Candidate items list
    const float rightColX = contentX + columnW + kColumnGap;
    const SlotEntry &selectedSlot = m_slots[static_cast<std::size_t>(m_selectedSlotIndex)];

    // Tab-driven read-only description card for the currently highlighted
    // candidate (Part I). Reuses the same card helper as SlotSelect.
    // When the card is open, skip the breadcrumb so its "Editing: X" line
    // doesn't overlap the card's own "Item Details" header.
    if (m_showItemDescription && m_selectedItemIndex >= 0 && m_selectedItemIndex < static_cast<int>(m_candidates.size()))
    {
        const ItemCandidate &cand = m_candidates[static_cast<std::size_t>(m_selectedItemIndex)];
        if (cand.gear)
        {
            renderItemDescriptionCard(renderer,
                                      Rectf{rightColX, lowerY, columnW, lowerH},
                                      selectedSlot, cand.gear, nullptr);
            renderer->renderTextInRect(m_font, "Tab/Esc: Close Description",
                                       Rectf{contentX, panelY + kPanelH - outer.bottom - 22.0f, contentW, 22.0f},
                                       UITheme::Info, HorizontalAlign::Center, VerticalAlign::Middle, false, false, false);
            return;
        }
    }

    const std::string breadcrumb = "Editing: " + selectedSlot.label;
    renderer->renderTextInRect(m_font, breadcrumb, Rectf{rightColX, lowerY, columnW, kLineH},
                               UITheme::SelectedText, HorizontalAlign::Left, VerticalAlign::Middle, false, false, false);

    const int candidateCount = static_cast<int>(m_candidates.size());
    if (candidateCount == 0)
    {
        renderer->renderTextInRect(m_font, "(No compatible items)",
                                   Rectf{rightColX, lowerY + 40.0f, columnW, 24.0f},
                                   UITheme::Info, HorizontalAlign::Center, VerticalAlign::Middle, false, false, false);
    }
    else
    {
        constexpr int kVisibleCandidates = 6;
        constexpr float kCandidateRowH = 26.0f;
        const int start = std::clamp(m_candidateScroll, 0, std::max(0, candidateCount - 1));
        const int end = std::min(candidateCount, start + kVisibleCandidates);

        float itemY = lowerY + 30.0f;
        for (int index = start; index < end; ++index)
        {
            const ItemCandidate &candidate = m_candidates[static_cast<std::size_t>(index)];
            const bool selected = (m_selectedItemIndex == index);

            if (selected)
            {
                renderer->setDrawColor(Color{80, 96, 122, 140});
                renderer->fillRect(Rectf{rightColX, itemY - 2.0f, columnW, kCandidateRowH});
            }

            renderer->renderTextInRect(m_font, candidate.gear->displayName,
                                       Rectf{rightColX + 10.0f, itemY, columnW * 0.7f - 10.0f, kCandidateRowH},
                                       selected ? UITheme::SelectedText : UITheme::Text,
                                       HorizontalAlign::Left, VerticalAlign::Middle, false, false, false);
            renderer->renderTextInRect(m_font, "x" + std::to_string(candidate.quantity),
                                       Rectf{rightColX + columnW * 0.7f, itemY, columnW * 0.3f - 10.0f, kCandidateRowH},
                                       selected ? UITheme::SelectedText : UITheme::Info,
                                       HorizontalAlign::Right, VerticalAlign::Middle, false, false, false);

            itemY += kCandidateRowH + 2.0f;
        }

        // Scroll indicators
        if (start > 0)
        {
            renderer->renderTextInRect(m_font, "^",
                                       Rectf{rightColX + columnW - 20.0f, lowerY + 14.0f, 16.0f, 16.0f},
                                       UITheme::SelectedText, HorizontalAlign::Center, VerticalAlign::Middle, false, false, false);
        }
        if (end < candidateCount)
        {
            renderer->renderTextInRect(m_font, "v",
                                       Rectf{rightColX + columnW - 20.0f, lowerY + 30.0f + static_cast<float>(kVisibleCandidates) * (kCandidateRowH + 2.0f), 16.0f, 16.0f},
                                       UITheme::SelectedText, HorizontalAlign::Center, VerticalAlign::Middle, false, false, false);
        }
    }

    const std::string itemHelp = "W/S: Select | A/D: Page | Tab: Details | X: Unequip | Enter: Equip | Esc: Back";
    renderer->renderTextInRect(m_font, itemHelp,
                               Rectf{contentX, panelY + kPanelH - outer.bottom - 22.0f, contentW, 22.0f},
                               UITheme::Info, HorizontalAlign::Center, VerticalAlign::Middle, false, false, false);
}

void UnitDetailWindow::renderItemDescriptionCard(Renderer *renderer,
                                                 const Rectf &rightColumn,
                                                 const SlotEntry &slotForLabel,
                                                 const Gear *gear,
                                                 const char *closeHint) const
{
    if (!gear)
        return;

    const float cardX = rightColumn.x;
    const float cardY = rightColumn.y;
    const float cardW = rightColumn.w;

    renderer->renderTextInRect(m_font, "Item Details", Rectf{cardX, cardY, cardW, kLineH},
                               UITheme::SelectedText, HorizontalAlign::Left, VerticalAlign::Middle, false, false, false);

    const Font *headingFont = FontManager::instance().get(FontRole::Heading);
    if (!headingFont)
        headingFont = m_font;

    float lineY = cardY + 30.0f;
    // Title
    renderer->renderTextInRect(headingFont, gear->displayName,
                               Rectf{cardX + 8.0f, lineY, cardW - 16.0f, 28.0f},
                               UITheme::SelectedText, HorizontalAlign::Left, VerticalAlign::Middle, false, false, false);
    lineY += 32.0f;

    // Slot / Type label
    std::string typeLabel = slotForLabel.label;
    if (gear->isWeapon())
        typeLabel += " (" + std::string(gear->weaponHandedness() == WeaponHandedness::OneHanded ? "1-Handed" : "2-Handed") + (gear->isRanged() ? ", Ranged" : "") + ")";
    renderer->renderTextInRect(m_font, typeLabel,
                               Rectf{cardX + 8.0f, lineY, cardW - 16.0f, 22.0f},
                               UITheme::Info, HorizontalAlign::Left, VerticalAlign::Middle, false, false, false);
    lineY += 28.0f;

    // Wrapped description
    if (!gear->description.empty())
    {
        const std::vector<std::string> descLines = TextWrap::wrap(renderer, m_font, gear->description, cardW - 20.0f);
        const float lineH = renderer->measureText(m_font, "Ag").y;
        for (const auto &line : descLines)
        {
            renderer->renderTextInRect(m_font, line, Rectf{cardX + 8.0f, lineY, cardW - 20.0f, lineH},
                                       UITheme::Text, HorizontalAlign::Left, VerticalAlign::Middle, false, false, false);
            lineY += lineH + 2.0f;
        }
        lineY += 8.0f;
    }

    // Stat modifiers list
    const auto &mods = gear->statModifiers();
    std::vector<std::string> modStrings;
    if (mods.attack != 0)
        modStrings.push_back((mods.attack > 0 ? "+" : "") + std::to_string(mods.attack) + " Attack");
    if (mods.defense != 0)
        modStrings.push_back((mods.defense > 0 ? "+" : "") + std::to_string(mods.defense) + " Defense");
    if (mods.magic != 0)
        modStrings.push_back((mods.magic > 0 ? "+" : "") + std::to_string(mods.magic) + " Magic");
    if (mods.magicDefense != 0)
        modStrings.push_back((mods.magicDefense > 0 ? "+" : "") + std::to_string(mods.magicDefense) + " Magic Def");
    if (mods.speed != 0)
        modStrings.push_back((mods.speed > 0 ? "+" : "") + std::to_string(mods.speed) + " Speed");
    if (mods.evasion != 0)
        modStrings.push_back((mods.evasion > 0 ? "+" : "") + std::to_string(mods.evasion) + " Evasion");
    if (mods.maxHp != 0)
        modStrings.push_back((mods.maxHp > 0 ? "+" : "") + std::to_string(mods.maxHp) + " Max HP");
    if (mods.maxMp != 0)
        modStrings.push_back((mods.maxMp > 0 ? "+" : "") + std::to_string(mods.maxMp) + " Max MP");
    if (mods.moveRange != 0)
        modStrings.push_back((mods.moveRange > 0 ? "+" : "") + std::to_string(mods.moveRange) + " Move");
    if (mods.jump != 0)
        modStrings.push_back((mods.jump > 0 ? "+" : "") + std::to_string(mods.jump) + " Jump");

    for (const auto &modStr : modStrings)
    {
        renderer->renderTextInRect(m_font, modStr, Rectf{cardX + 8.0f, lineY, cardW - 16.0f, 22.0f},
                                   Color{0, 255, 0, 255}, HorizontalAlign::Left, VerticalAlign::Middle, false, false, false);
        lineY += 24.0f;
    }
    (void)closeHint; // hint text is rendered by the caller along the bottom row
}
