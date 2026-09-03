#pragma once

#include "ui/UIKeyRepeat.h"
#include "ui/UIWindow.h"
#include "inventory/EquipRules.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"

#include <memory>
#include <vector>

class Font;
class GearCatalog;
class Input;
class Renderer;
class RosterSystem;
class Unit;
class Inventory;
class Gear;
struct EquipmentLoadout;
struct RosterUnit;
class FocusGroup;
class IFocusable;

class UnitDetailWindow final : public UIWindow
{
public:
    // Constructor for equipment-enabled interactive detail window
    UnitDetailWindow(WindowId id,
                     const RosterUnit &rosterUnit,
                     RosterSystem &roster,
                     Inventory &inventory,
                     const GearCatalog &catalog);

    // Constructor for read-only inspection (roster unit)
    UnitDetailWindow(WindowId id, const RosterUnit &rosterUnit, const GearCatalog &catalog);

    // Constructor for live unit inspection (battle inspection)
    UnitDetailWindow(WindowId id, const Unit &unit, const GearCatalog &catalog);

    void setFont(const Font *font) { m_font = font; }

    void handleInput(const Input &input) override;
    void update(float dt) override;
    void render(Renderer *renderer) const override;

    int rosterInstanceId() const { return m_rosterInstanceId; }

private:
    enum class UIState
    {
        Details,    // Show unit info and action menu (Equip, Abilities, Dismiss)
        SlotSelect, // Select which gear slot to modify
        ItemSelect, // Select item for the slot with live stat preview
    };

    struct SlotEntry
    {
        GearSlot slot = GearSlot::Weapon;
        int accessoryIndex = -1;
        std::string label;
    };

    struct ItemCandidate
    {
        ItemId itemId = 0;
        const Gear *gear = nullptr;
        std::uint32_t quantity = 0;
    };

    // State management
    void setState(UIState newState);
    void enterSlotSelect();
    void exitSlotSelect();
    void enterItemSelect();
    void exitItemSelect();
    void confirmItemSelection();
    void unequipCurrentSlot();

    // Item filtering and preview
    void rebuildCandidatesList();
    bool isSlotEligible(GearSlot slot) const;

    // Data access
    const Gear *equippedGearAt(const SlotEntry &slot) const;
    void setEquippedGearAt(const SlotEntry &slot, const Gear *gear);
    EquipmentLoadout loadoutWithoutSelectedSlot() const;

    // Rendering helpers
    void renderIdentityHeader(Renderer *renderer, Vec2f contentPos, float contentW) const;
    void renderStatsColumn(Renderer *renderer, Vec2f colPos, float columnW, const Unit *previewUnit = nullptr) const;
    void renderDetailsPanel(Renderer *renderer) const;
    void renderSlotSelectPanel(Renderer *renderer) const;
    void renderItemSelectPanel(Renderer *renderer) const;
    void renderActionMenu(Renderer *renderer) const;
    // Shared read-only description card used by both SlotSelect (currently
    // equipped gear) and ItemSelect (candidate gear) — keeps the look and
    // dismiss affordance identical across both panels (Part I).
    void renderItemDescriptionCard(Renderer *renderer,
                                   const Rectf &rightColumn,
                                   const SlotEntry &slotForLabel,
                                   const Gear *gear,
                                   const char *closeHint) const;

    // Mode flag: true if interactive (has roster/inventory), false if read-only
    bool m_isInteractive = false;
    bool m_isPlayerOwned = true;
    bool m_showItemDescription = false;

    // Persistent data for interactive mode
    RosterSystem *m_roster = nullptr;
    Inventory *m_inventory = nullptr;
    const GearCatalog *m_catalog = nullptr;

    // Display data
    int m_rosterInstanceId = -1;
    const Font *m_font = nullptr;
    std::unique_ptr<Unit> m_unit;
    std::unique_ptr<Unit> m_previewUnit;
    std::unique_ptr<EquipmentLoadout> m_loadout;
    std::unique_ptr<EquipmentLoadout> m_previewLoadout;
    std::string m_className;
    int m_exp = 0;

    // Interactive UI state
    UIState m_state = UIState::Details;
    int m_selectedActionIndex = 0;

    // Slot selection state
    std::vector<SlotEntry> m_slots;
    int m_selectedSlotIndex = 0;
    // Slot index to restore on the next SlotSelect entry. Set when
    // transitioning into ItemSelect (Part H), consumed in enterSlotSelect.
    int m_lastSelectedSlotIndex = -1;
    std::unique_ptr<FocusGroup> m_slotFocus;

    // Item selection state
    std::vector<ItemCandidate> m_candidates;
    int m_selectedItemIndex = 0;
    int m_candidateScroll = 0;
    std::unique_ptr<FocusGroup> m_itemFocus;

    // Hold-to-repeat trackers (Part K) for SlotSelect and ItemSelect
    // Up/Down navigation. A/D page jumps stay single-step.
    UIKeyRepeat m_slotUpRepeat;
    UIKeyRepeat m_slotDownRepeat;
    UIKeyRepeat m_slotLeftRepeat;
    UIKeyRepeat m_slotRightRepeat;
    UIKeyRepeat m_itemUpRepeat;
    UIKeyRepeat m_itemDownRepeat;
    UIKeyRepeat m_itemLeftRepeat;
    UIKeyRepeat m_itemRightRepeat;
    // Most recent update() dt, cached so handleInput() can drive the
    // repeat trackers (the engine dispatches handleInput before update
    // on the same frame).
    float m_lastDt = 0.f;
};