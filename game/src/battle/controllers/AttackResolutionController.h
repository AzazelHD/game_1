#pragma once

#include "battle/combat/PendingAttackController.h"
#include "battle/combat/CombatSystem.h"
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

    // Advances the hit-by-hit playback by one target. Each target is played in
    // two phases - see the comment on PendingHitResult::visualPlayed - so this
    // may be called several times for the same target.
    void processNextResult();

    // Abandons the current attack (e.g. player backs out of AttackConfirm/
    // AttackTarget) without applying anything.
    void cancel();

    [[nodiscard]] PendingAttackController &pendingAttack() { return m_pendingAttack; }
    [[nodiscard]] const PendingAttackController &pendingAttack() const { return m_pendingAttack; }
    [[nodiscard]] std::string &pendingSkillId() { return m_pendingSkillId; }
    [[nodiscard]] const std::string &pendingSkillId() const { return m_pendingSkillId; }

    // Per-step playback readouts for the camera (which hit is showing and its
    // target). Resolution is driven frame-to-frame by the timer, so the caller
    // snaps to currentHitTarget() whenever pendingHitIndex() advances.
    [[nodiscard]] std::size_t pendingHitIndex() const { return m_pendingHitIndex; }
    [[nodiscard]] std::size_t pendingHitCount() const { return m_pendingHitResults.size(); }
    [[nodiscard]] Unit *currentHitTarget() const
    {
        return m_pendingHitIndex < m_pendingHitResults.size() ? m_pendingHitResults[m_pendingHitIndex].target : nullptr;
    }

private:
    // One target's rolled outcome. `result` is fixed when the resolution begins
    // so the whole sequence is deterministic.
    struct PendingHitResult
    {
        Unit *target = nullptr;
        CombatResult result;
        HitContext hitCtx;

        // A skill's `visualOnHit` makes each target a two-phase playback:
        //
        //   phase 1 - play the skill's visual on the target and park the turn
        //             timer for its duration. The index is NOT advanced, so the
        //             target stays on screen, alive and undamaged while the
        //             effect plays.
        //   phase 2 - the timer fires processNextResult() again; visualPlayed is
        //             already true, so this call falls straight through to the
        //             consequence (damage, KO, heal, status).
        //
        // This is what lets any future skill animation gate its own outcome, and
        // it needs no per-skill special-casing.
        bool visualPlayed = false;

        // Second two-phase gate: a skill with effectPerTarget=true (the default)
        // plays its effect on each connected target before that target's
        // consequence, so the whole resolve reads "effect → damage" per unit
        // instead of a burst of numbers at once. Mirrors visualPlayed above.
        bool effectPlayed = false;
    };

    // Phase 2 pieces, one per kind of consequence. Each builds the resolution
    // context itself (a cheap aggregate of references), which keeps this header
    // free of BattleState's nested context type.
    void applySupportiveOutcome(const PendingHitResult &entry, const SkillData &skill);
    void applyOffensiveOutcome(const PendingHitResult &entry, const SkillData *skill);
    void applyStatusRiders(const PendingHitResult &entry, const SkillData &skill);

    // Hands playback to the next target after the standard beat.
    void advanceToNextTarget();

    void finishResolution();

    BattleState &m_owner;

    PendingAttackController m_pendingAttack;
    std::string m_pendingSkillId;

    std::vector<PendingHitResult> m_pendingHitResults;
    std::size_t m_pendingHitIndex = 0;
    bool m_pendingAnyDied = false;
};
