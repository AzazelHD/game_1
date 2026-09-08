#include "battle/controllers/AttackResolutionController.h"

#include "engine/core/Log.h"
#include "engine/input/KeyCode.h"
#include "engine/renderer/Color.h"
#include "battle/unit/Unit.h"
#include "battle/combat/CombatSystem.h"
#include "battle/combat/SkillEffects.h"
#include "scenes/BattleState.h"

#include <cstdio>
#include <string>

void AttackResolutionController::prepare(Unit *active, Vec2i targetPos, Unit *directTarget, const SkillData *skill)
{
    BattleState::AttackResolutionContext ctx = m_owner.makeAttackResolutionContext();
    const RangeRule rule = skill ? skill->toRangeRule() : RangeRule{};
    m_pendingAttack.begin(active, targetPos, directTarget, skill, rule,
                          ctx.grid, ctx.battleMap, ctx.session.getUnitPtrs());
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

    if (previewSkill)
        ctx.topBattleText = skill_effects::describeSkill(*previewSkill, hitCtx);
    else
    {
        const CombatResult preview = CombatSystem::preview(hitCtx);
        char textBuf[96];
        std::snprintf(textBuf, sizeof(textBuf), "DMG %d   ACC %d%%", preview.damage, static_cast<int>(preview.hitChance + 0.5f));
        ctx.topBattleText = textBuf;
    }
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
        const bool supportive = skill && skill_effects::isSupportive(*skill);
        m_pendingHitResults.push_back(PendingHitResult{
            .target = u,
            .result = supportive ? CombatResult{} : CombatSystem::resolve(hitCtx),
            .hitCtx = hitCtx});
    }

    if (skill && skill->castOncePerArea)
    {
        // Arena-wide cast flash (e.g. a dragon's breath) once, before the
        // per-target hits below.
        ctx.combatAnimations.enqueue(skill->id + "_cast_once", 0.5f);
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

    const SkillData *skill = nullptr;
    if (!m_pendingSkillId.empty())
    {
        auto it = ctx.skillDB.find(m_pendingSkillId);
        if (it != ctx.skillDB.end())
            skill = &it->second;
    }
    const bool castOnce = skill && skill->castOncePerArea;

    // Supportive skills (heal/buff) apply unconditionally — no hit roll.
    if (skill && skill_effects::isSupportive(*skill))
    {
        if (skill_effects::hasEffect(*skill, SkillEffectType::Heal))
        {
            const int heal = skill_effects::computeHealAmount(entry.hitCtx);
            entry.target->heal(heal);
            ctx.floatingText.enqueue("+" + std::to_string(heal), entry.target->getPosition(),
                                     Color{120, 255, 140, 255});
            ctx.combatAnimations.enqueue("heal", entry.target->getPosition());
            LOG_INFO("Battle", "%s heals %s for %d HP",
                     entry.hitCtx.attacker ? entry.hitCtx.attacker->getName().c_str() : "?",
                     entry.target->getName().c_str(), heal);
        }
        if (skill_effects::hasEffect(*skill, SkillEffectType::Buff))
        {
            entry.target->applyStatMod(skill_effects::statKindFromKey(skill->statKey),
                                       skill->statAmount, skill->buffTurns);
            const bool beneficial = skill->statAmount >= 0;
            ctx.floatingText.enqueue(skill_effects::describeStatMod(*skill),
                                     entry.target->getPosition(),
                                     beneficial ? Color{120, 190, 255, 255} : Color{255, 150, 70, 255});
            ctx.combatAnimations.enqueue(beneficial ? "buff" : "debuff", entry.target->getPosition());
            LOG_INFO("Battle", "%s applies %s to %s",
                     entry.hitCtx.attacker ? entry.hitCtx.attacker->getName().c_str() : "?",
                     skill_effects::describeStatMod(*skill).c_str(),
                     entry.target->getName().c_str());
        }
        ++m_pendingHitIndex;
        ctx.turnTimer = 0.4f;
        return;
    }

    ctx.session.applyDamage(*entry.target, entry.result);

    if (skill && (skill_effects::hasEffect(*skill, SkillEffectType::Buff) ||
                  skill_effects::hasEffect(*skill, SkillEffectType::Debuff)) &&
        entry.result.hit)
    {
        // Offensive skill carrying a rider debuff/buff (e.g. drain): apply the
        // stat mod on a landed hit.
        entry.target->applyStatMod(skill_effects::statKindFromKey(skill->statKey),
                                   skill->statAmount, skill->buffTurns);
    }

    if (entry.result.hit)
    {
        if (entry.target->isDead())
            m_pendingAnyDied = true;

        ctx.floatingText.enqueue(std::to_string(entry.result.damage), entry.target->getPosition(),
                                 Color{255, 90, 90, 255});

        ctx.combatAnimations.enqueue("hit_impact", entry.target->getPosition());

        LOG_INFO("Battle", "Attack hits %s for %d damage!",
                 entry.target->getName().c_str(), entry.result.damage);
    }
    else
    {
        ctx.floatingText.enqueue("MISS", entry.target->getPosition(), Color{200, 200, 200, 255});
        ctx.combatAnimations.enqueue("dodge", entry.target->getPosition());

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
        {
            active->setCurrentMp(active->getCurrentMp() - skill->mpCost);
            if (skill->cooldown > 0)
                active->setSkillCooldown(skill->id, skill->cooldown + 1); // turn of use counts
        }
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

    m_owner.battleMenu().closeAllMenus();
    ctx.humanTurnPhase = BattleState::HumanTurnPhase::ActionMenu;
    if (active)
    {
        ctx.cursor.setPosition(active->getPosition());
        m_owner.openBattleMenu(m_owner.canActiveUnitMove(), false, true, KeyCode::Accept);
    }
}

void AttackResolutionController::cancel()
{
    BattleState::AttackResolutionContext ctx = m_owner.makeAttackResolutionContext();

    m_pendingAttack.clear();
    m_pendingSkillId.clear();
    ctx.damagePreview.hide();
    ctx.topBattleText.clear();
    m_owner.battleMenu().closeAllMenus();
    ctx.humanTurnPhase = BattleState::HumanTurnPhase::AttackTarget;
}
