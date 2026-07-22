#pragma once

#include "battle/PendingAttackController.h"
#include "battle/CombatSystem.h"
#include "engine/math/Vec2.h"

#include <cstddef>
#include <string>
#include <vector>

class BattleState;
class Unit;
struct SkillData;

// Owns everything specific to resolving a single confirmed attack/skill use:
// the pending target set (PendingAttackController), the skill id locked in
// for this resolution, and the roll-ahead hit results being played back one
// at a time.
//
// Shared turn-flow state (whose turn it is, BattleState's own
// TurnState/PendingResolution machine, HUD, timers, etc.) stays owned by
// BattleState and is reached through AttackResolutionContext — the same
// pattern HumanTurnController already uses via HumanTurnContext. This class
// never touches BattleState's private members directly.
class AttackResolutionController
{
public:
    explicit AttackResolutionController(BattleState &owner) : m_owner(owner) {}

    // Builds the target/tile set for a just-confirmed move into AttackConfirm
    // (direct target or skill area) and refreshes the damage preview.
    void prepare(Unit *active, Vec2i targetPos, Unit *directTarget, const SkillData *skill);

    // Cycles which target in the pending set is focused (multi-target AoE).
    void cycleFocus(int delta);

    // Refreshes the damage/accuracy preview and hovered-unit highlight for
    // whichever target is currently focused.
    void updatePreview(Unit *active);

    // Rolls every target's hit/damage up front (deterministic for the rest
    // of the sequence) and starts the hit-by-hit playback.
    void beginResolution(Unit *active, const SkillData *skill);

    // Advances the hit-by-hit playback by one target; calls the internal
    // finish step once all targets are processed.
    void processNextResult();

    // Abandons the current attack (e.g. player backs out of AttackConfirm/
    // AttackTarget) without applying anything.
    void cancel();

    [[nodiscard]] PendingAttackController &pendingAttack() { return m_pendingAttack; }
    [[nodiscard]] const PendingAttackController &pendingAttack() const { return m_pendingAttack; }
    [[nodiscard]] std::string &pendingSkillId() { return m_pendingSkillId; }
    [[nodiscard]] const std::string &pendingSkillId() const { return m_pendingSkillId; }

private:
    struct PendingHitResult
    {
        Unit *target = nullptr;
        CombatResult result;
    };

    void finishResolution();

    BattleState &m_owner;

    PendingAttackController m_pendingAttack;
    std::string m_pendingSkillId;

    std::vector<PendingHitResult> m_pendingHitResults;
    std::size_t m_pendingHitIndex = 0;
    bool m_pendingAnyDied = false;
};
