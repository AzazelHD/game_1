#include "battle/controllers/AttackResolutionController.h"

#include "engine/core/Log.h"
#include "engine/input/KeyCode.h"
#include "engine/renderer/Color.h"
#include "battle/unit/Unit.h"
#include "battle/combat/CombatSystem.h"
#include "battle/combat/SkillEffects.h"
#include "engine/math/MathUtils.h"
#include "scenes/BattleState.h"

#include <cstdio>
#include <string>

namespace
{
    // Pause between two hits so the animation and the floating number both read.
    constexpr float kHitPacing = 0.4f;
}

void AttackResolutionController::prepare(Unit *active, Vec2i targetPos, Unit *directTarget, const SkillData *skill)
{
    BattleState::AttackResolutionContext ctx = m_owner.makeAttackResolutionContext();
    // Must match the rule the targeting overlay was built with, otherwise the
    // splash preview and the confirm step would disagree. toRangeRuleFor() is
    // the single source of truth: plain skills use their authored rule, the
    // basic attack uses the acting unit's atkRange/jump.
    const RangeRule rule =
        skill ? skill->toRangeRuleFor(active->getAtkRange(), active->getJump()) : RangeRule{};
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
        return;
    }

    // Park the cursor on whichever target is focused — for an AoE cast on an
    // empty tile the blast centre can be adjacent to the unit actually being
    // previewed, and Q/E moves the focus between targets. The cursor itself is
    // still frozen to arrow input during AttackConfirm (BattleState::update),
    // so only this focus-driven move can relocate it.
    ctx.hoveredUnit = focus;
    ctx.cursor.setPosition(focus->getPosition());

    const SkillData *previewSkill = nullptr;
    if (!ctx.selectedSkillId.empty())
    {
        auto it = ctx.skillDB.find(ctx.selectedSkillId);
        if (it != ctx.skillDB.end())
            previewSkill = &it->second;
    }

    // The preview box owns the readout. What the skill actually does is shown
    // once, in the confirm window's info block.
    ctx.damagePreview.show(*active, *focus, previewSkill, &ctx.battleMap);
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

    // Face whatever we're aiming at (first living target).
    for (Unit *u : m_pendingAttack.targets())
    {
        if (u && !u->isDead())
        {
            const Vec2i delta = u->getPosition() - active->getPosition();
            if (delta.x != 0 || delta.y != 0)
                active->setFacing(isoDirectionForTileDelta(delta));
            break;
        }
    }

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
    ctx.turnTimer = kHitPacing; // TODO: tune per-hit pacing, maybe per-skill
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

    // A supportive skill lands unconditionally; an offensive one only on a hit.
    const bool connects = entry.result.hit || (skill && skill_effects::isSupportive(*skill));

    // ── Phase 1: the skill's visual, once per connected target ──────────────
    // Parking the timer instead of advancing the index is what makes this a
    // gate: the next tick comes straight back here, skips this block (the visual
    // already played) and lands the consequence below.
    if (connects && skill && !skill->visualOnHit.empty() && !entry.visualPlayed)
    {
        entry.visualPlayed = true;
        ctx.combatAnimations.enqueue(skill->visualOnHit.id, entry.target->getPosition(),
                                     skill->visualOnHit.duration);
        ctx.turnTimer = skill->visualOnHit.duration;
        return;
    }

    // ── Phase 2: the consequence ─────────────────────────────────────────────
    if (skill && skill_effects::isSupportive(*skill))
    {
        applySupportiveOutcome(entry, *skill);
        advanceToNextTarget();
        return;
    }

    applyOffensiveOutcome(entry, skill);
    advanceToNextTarget();
}

void AttackResolutionController::applySupportiveOutcome(const PendingHitResult &entry,
                                                       const SkillData &skill)
{
    BattleState::AttackResolutionContext ctx = m_owner.makeAttackResolutionContext();
    const char *casterName = entry.hitCtx.attacker ? entry.hitCtx.attacker->getName().c_str() : "?";

    if (skill_effects::hasEffect(skill, SkillEffectType::Heal))
    {
        const int heal = skill_effects::computeHealAmount(entry.hitCtx);
        entry.target->heal(heal);
        ctx.floatingText.enqueue("+" + std::to_string(heal), entry.target->getPosition(),
                                 Color{120, 255, 140, 255});
        ctx.combatAnimations.enqueue("heal", entry.target->getPosition());
        LOG_INFO("Battle", "%s heals %s for %d HP", casterName, entry.target->getName().c_str(), heal);
    }

    // Flat stat mods (statKey) apply a number; status-based buffs (e.g.
    // Barrier / Attack Up) ride on the `statuses` array below.
    if (skill_effects::hasEffect(skill, SkillEffectType::Buff) && !skill.statKey.empty())
    {
        const bool beneficial = skill.statAmount >= 0;
        entry.target->applyStatMod(skill_effects::statKindFromKey(skill.statKey),
                                   skill.statAmount, skill.buffTurns);
        ctx.floatingText.enqueue(skill_effects::describeStatMod(skill),
                                 entry.target->getPosition(),
                                 beneficial ? Color{120, 190, 255, 255} : Color{255, 150, 70, 255});
        ctx.combatAnimations.enqueue(beneficial ? "buff" : "debuff", entry.target->getPosition());
        LOG_INFO("Battle", "%s applies %s to %s", casterName,
                 skill_effects::describeStatMod(skill).c_str(), entry.target->getName().c_str());
    }

    if (!skill.statuses.empty())
        applyStatusRiders(entry, skill);
}

void AttackResolutionController::applyOffensiveOutcome(const PendingHitResult &entry,
                                                      const SkillData *skill)
{
    BattleState::AttackResolutionContext ctx = m_owner.makeAttackResolutionContext();

    if (!entry.result.hit)
    {
        ctx.floatingText.enqueue("MISS", entry.target->getPosition(), Color{200, 200, 200, 255});
        ctx.combatAnimations.enqueue("dodge", entry.target->getPosition());
        LOG_INFO("Battle", "Attack misses %s", entry.target->getName().c_str());
        return;
    }

    // A KO kills outright: no damage roll, no defense, no damage number. It is
    // the skill's whole consequence, so it takes precedence over Damage.
    if (skill && skill_effects::hasEffect(*skill, SkillEffectType::Ko))
    {
        const char *casterName = entry.hitCtx.attacker ? entry.hitCtx.attacker->getName().c_str() : "?";
        ctx.session.applyKo(*entry.target);
        m_pendingAnyDied = entry.target->isDead() || m_pendingAnyDied;
        LOG_INFO("Battle", "%s is KO'd by %s", entry.target->getName().c_str(), casterName);
        return;
    }

    ctx.session.applyDamage(*entry.target, entry.result);

    // Offensive skill carrying a rider debuff/buff (e.g. drain): apply the stat
    // mod on a landed hit. Status-based riders go through applyStatusRiders.
    if (skill && !skill->statKey.empty() &&
        (skill_effects::hasEffect(*skill, SkillEffectType::Buff) ||
         skill_effects::hasEffect(*skill, SkillEffectType::Debuff)))
    {
        entry.target->applyStatMod(skill_effects::statKindFromKey(skill->statKey),
                                   skill->statAmount, skill->buffTurns);
    }

    if (entry.target->isDead())
        m_pendingAnyDied = true;

    ctx.floatingText.enqueue(std::to_string(entry.result.damage), entry.target->getPosition(),
                             Color{255, 90, 90, 255});
    ctx.combatAnimations.enqueue("hit_impact", entry.target->getPosition());

    if (skill && !skill->statuses.empty())
        applyStatusRiders(entry, *skill);

    LOG_INFO("Battle", "Attack hits %s for %d damage!",
             entry.target->getName().c_str(), entry.result.damage);
}

void AttackResolutionController::applyStatusRiders(const PendingHitResult &entry,
                                                  const SkillData &skill)
{
    BattleState::AttackResolutionContext ctx = m_owner.makeAttackResolutionContext();
    const char *casterName = entry.hitCtx.attacker ? entry.hitCtx.attacker->getName().c_str() : "?";

    for (const StatusEffectApply &apply : skill.statuses)
    {
        if (!CombatSystem::chanceRoll(apply.chance))
            continue;
        entry.target->applyStatus(apply.type, apply.turns, apply.damagePerTick);
        const StatusInfo &info = statusInfo(apply.type);
        ctx.floatingText.enqueue(info.label, entry.target->getPosition(),
                                 info.harmful ? Color{200, 90, 220, 255} : Color{120, 190, 255, 255});
        ctx.combatAnimations.enqueue(info.harmful ? "debuff" : "buff", entry.target->getPosition());
        LOG_INFO("Battle", "%s inflicts %s on %s (%d turns)", casterName,
                 info.label, entry.target->getName().c_str(), apply.turns);
    }
}

void AttackResolutionController::advanceToNextTarget()
{
    ++m_pendingHitIndex;
    m_owner.makeAttackResolutionContext().turnTimer = kHitPacing; // stays in WaitingForAnimation
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
        // The basic attack is a real skill but must still count as an Attack
        // (it costs no MP), so keep the two notions separate here.
        const bool basic = skill && skill->isBasicAttack;
        if (skill && !basic)
            active->setCurrentMp(active->getCurrentMp() - skill->mpCost);
        active->setMajorAction(basic ? MajorAction::Attack
                                     : (skill ? MajorAction::Skill : MajorAction::Attack));
    }

    ctx.canUndoLastMove = false;
    ctx.selectedSkillId.clear();
    m_pendingSkillId.clear();

    ctx.damagePreview.hide();
    m_pendingAttack.clear();
    ctx.topBattleText.clear();
    m_pendingHitResults.clear();
    m_pendingHitIndex = 0;

    // No checkResult() call needed: BattleSession::applyDamage() evaluates the
    // result itself, so it is already current after the hits above.
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
    m_pendingHitResults.clear();
    m_pendingHitIndex = 0;
    ctx.damagePreview.hide();
    ctx.topBattleText.clear();
    m_owner.battleMenu().closeAllMenus();
    ctx.humanTurnPhase = BattleState::HumanTurnPhase::AttackTarget;
}
