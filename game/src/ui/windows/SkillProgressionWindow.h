#pragma once

#include "ui/UIWindow.h"
#include "engine/math/Vec2.h"
#include "engine/ui/Button.h"
#include "engine/ui/FocusGroup.h"
#include "battle/unit/SkillProgression.h"
#include "battle/unit/UnitData.h"

#include <memory>
#include <string>
#include <vector>

class Font;
class Input;
class Renderer;
class RosterSystem;
struct RosterUnit;

// World-map skill progression panel (M4). Lists the skills the unit's class
// can learn, spends permanent skill points to learn/unlock them (Enter in
// Learn mode) and toggles which unlocked active skills are equipped — the
// equipped set is what shows up in battle (Unit::importRosterProgression).
class SkillProgressionWindow final : public UIWindow
{
public:
    SkillProgressionWindow(WindowId id, const RosterUnit &unit, RosterSystem &roster);

    void setFont(const Font *font) { m_font = font; }

    void handleInput(const Input &input) override;
    void update(float dt) override;
    void render(Renderer *renderer) const override;

private:
    enum class Mode
    {
        Learn,
        Equip,
    };

    enum class PendingMode
    {
        None,
        LearnConfirm,
    };

    void rebuildRows();
    void actOnSelection();
    void commitLearn(int index);
    void cancelPending();
    void syncSkillsClass();

    const Font *m_font = nullptr;
    RosterSystem *m_roster = nullptr;
    int m_instanceId = -1;
    BaseClass m_baseClass = BaseClass::Soldier;
    PromotionClass m_promotion = PromotionClass::None;
    std::string m_name;
    Mode m_mode = Mode::Learn;
    PendingMode m_pending = PendingMode::None;
    int m_pendingSkillIndex = -1;

    std::vector<const ProgressionSkillDefinition *> m_skills;
    std::vector<std::unique_ptr<Button>> m_rows;
    FocusGroup m_focus;
};