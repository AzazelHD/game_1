#include "battle/AttackResolutionController.h"

#include "engine/core/Log.h"
#include "engine/input/KeyCode.h"
#include "engine/renderer/Color.h"
#include "battle/Unit.h"
#include "battle/CombatSystem.h"
#include "scenes/BattleState.h"

#include <cstdio>
#include <string>

void AttackResolutionController::prepare(Unit *active, Vec2i targetPos, Unit *directTarget, const SkillData *skill)
{
    BattleState::AttackResolutionContext ctx = m_owner.makeAttackResolutionContext();
    m_pendingAttack.begin(active, targetPos, directTarget, skill, ctx.session.getUnitPtrs());
    updatePreview(active);
}

void AttackResolutionController::cycleFocus(int delta)
{
    m_pendingAttack.cycleFocus(delta);
}

void AttackResolutionController::updatePreview(Unit *active)
{
    BattleState::AttackResolutionContext ctx = m_owner.makeAttackResolutionContext();

    Unit *focus = m_pendingAttack.focusedTarget();
    if (!active || !focus)
    {
        ctx.damagePreview.hide();
        ctx.topBattleText.clear();
        return;
    }

    ctx.hoveredUnit = focus;

    const SkillData *previewSkill = nullptr;
    if (!ctx.selectedSkillId.empty())
    {
        auto it = ctx.skillDB.find(ctx.selectedSkillId);
        if (it != ctx.skillDB.end())
            previewSkill = &it->second;
    }
    ctx.damagePreview.show(*active, *focus, previewSkill);

    HitContext hitCtx = m_owner.makeHitContext(active, focus, previewSkill);

    const CombatResult preview = CombatSystem::preview(hitCtx);
    char textBuf[96];
    std::snprintf(textBuf, sizeof(textBuf), "DMG %d   ACC %d%%", preview.damage, static_cast<int>(preview.hitChance + 0.5f));
    ctx.topBattleText = textBuf;
}

void AttackResolutionController::beginResolution(Unit *active, const SkillData *skill)
{
    if (!active)
    {
        finishResolution();
        return;
    }

    BattleState::AttackResolutionContext ctx = m_owner.makeAttackResolutionContext();

    // Roll every target's hit/damage NOW, up front — deterministic for the
    // rest of the sequence, rather than re-rolling mid-animation later.
    m_pendingHitResults.clear();
    m_pendingHitIndex = 0;
    m_pendingAnyDied = false;

    for (Unit *u : m_pendingAttack.targets())
    {
        if (!u || u->isDead())
            continue;
        HitContext hitCtx = m_owner.makeHitContext(active, u, skill);
        m_pendingHitResults.push_back(PendingHitResult{.target = u, .result = CombatSystem::resolve(hitCtx)});
    }

    if (skill && skill->castOncePerArea)
    {
        // TODO: play the skill's single cast animation once here (e.g. a
        // dragon's breath), covering the whole area — before any individual
        // hit/miss is shown. Something like:
        //   m_combatAnimations.enqueue(skill->id + "_cast_once", castDuration);
    }

    ctx.pendingResolution = BattleState::PendingResolution::ResolvingHits;
    ctx.turnTimer = 0.4f; // TODO: tune per-hit pacing, maybe per-skill
}

void AttackResolutionController::processNextResult()
{
    BattleState::AttackResolutionContext ctx = m_owner.makeAttackResolutionContext();

    if (m_pendingHitIndex >= m_pendingHitResults.size())
    {
        finishResolution();
        return;
    }

    PendingHitResult &entry = m_pendingHitResults[m_pendingHitIndex];
    ctx.session.applyDamage(*entry.target, entry.result);

    const SkillData *skill = nullptr;
    if (!m_pendingSkillId.empty())
    {
        auto it = ctx.skillDB.find(m_pendingSkillId);
        if (it != ctx.skillDB.end())
            skill = &it->second;
    }
    const bool castOnce = skill && skill->castOncePerArea;

    if (entry.result.hit)
    {
        if (entry.target->isDead())
            m_pendingAnyDied = true;

        ctx.floatingText.enqueue(std::to_string(entry.result.damage), entry.target->getPosition(),
                                 Color{255, 90, 90, 255});

        // TODO: play per-target cast animation here if !castOnce (e.g. a
        // mage's fire bolt jumping target to target), plus a hit-impact
        // animation on entry.target regardless of castOnce:
        //   if (!castOnce) m_combatAnimations.enqueue(skill->id + "_cast", ...);
        //   m_combatAnimations.enqueue("hit_impact", ...);

        LOG_INFO("Battle", "Attack hits %s for %d damage!",
                 entry.target->getName().c_str(), entry.result.damage);
    }
    else
    {
        ctx.floatingText.enqueue("MISS", entry.target->getPosition(), Color{200, 200, 200, 255});

        // TODO: play a dodge/evade animation on entry.target:
        //   m_combatAnimations.enqueue("dodge", ...);

        LOG_INFO("Battle", "Attack misses %s", entry.target->getName().c_str());
    }

    ++m_pendingHitIndex;
    ctx.turnTimer = 0.4f; // TODO: tune per-hit pacing; stays in WaitingForAnimation
}

void AttackResolutionController::finishResolution()
{
    BattleState::AttackResolutionContext ctx = m_owner.makeAttackResolutionContext();

    Unit *active = ctx.pendingActor ? ctx.pendingActor : ctx.session.getCurrentUnit();

    const SkillData *skill = nullptr;
    if (!m_pendingSkillId.empty())
    {
        auto it = ctx.skillDB.find(m_pendingSkillId);
        if (it != ctx.skillDB.end())
            skill = &it->second;
    }

    if (active)
    {
        if (skill)
            active->setCurrentMp(active->getCurrentMp() - skill->mpCost);
        active->setMajorAction(skill ? MajorAction::Skill : MajorAction::Attack);
    }

    ctx.canUndoLastMove = false;
    ctx.selectedSkillId.clear();
    m_pendingSkillId.clear();

    ctx.damagePreview.hide();
    m_pendingAttack.clear();
    ctx.topBattleText.clear();
    m_pendingHitResults.clear();
    m_pendingHitIndex = 0;

    ctx.session.checkResult();

    if (m_pendingAnyDied)
        ctx.eventSystem.emit(BattleTriggerType::OnUnitDeath);
    m_pendingAnyDied = false;

    ctx.pendingResolution = BattleState::PendingResolution::None;
    ctx.pendingActor = nullptr;
    ctx.pendingTarget = nullptr;
    ctx.pendingActionLabel.clear();
    ctx.turnState = BattleState::TurnState::ProcessingTurn;

    if (m_owner.checkDefeat())
    {
        m_owner.startBattleEnd(false);
        return;
    }
    if (m_owner.checkVictory())
    {
        m_owner.startBattleEnd(true);
        return;
    }

    ctx.hud.clear();
    ctx.humanTurnPhase = BattleState::HumanTurnPhase::ActionMenu;
    if (active)
        m_owner.openBattleMenu(m_owner.canActiveUnitMove(), false, true, KeyCode::Accept);
}

void AttackResolutionController::cancel()
{
    BattleState::AttackResolutionContext ctx = m_owner.makeAttackResolutionContext();

    m_pendingAttack.clear();
    m_pendingSkillId.clear();
    ctx.damagePreview.hide();
    ctx.topBattleText.clear();
    ctx.hud.clear();
    ctx.humanTurnPhase = BattleState::HumanTurnPhase::AttackTarget;
}
