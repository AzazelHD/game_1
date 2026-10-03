#include "ui/DamagePreview.h"
#include "battle/unit/Unit.h"
#include "battle/combat/CombatSystem.h"
#include "battle/combat/SkillEffects.h"
#include "data/SkillLoader.h"
#include "engine/renderer/Renderer.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/Aligment.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "config/GameConstants.h"

#include <algorithm>
#include <cstdio>

namespace
{
    constexpr float BOX_W = 264.0f;
    constexpr float BOX_H = 66.0f;
    // Sits below the vertical centre so a focused unit standing in the middle
    // of the screen stays readable while cycling targets.
    constexpr float BOX_X = (GameConstants::VIEW_W - BOX_W) * 0.5f;
    constexpr float BOX_Y = GameConstants::VIEW_H * 0.5f + 74.0f;

    constexpr float DAMAGE_H = 24.0f;
    constexpr float GAUGE_ROW_Y = 34.0f;
    constexpr float GAUGE_ROW_H = 16.0f;
    constexpr float ACC_LABEL_W = 34.0f;
    constexpr float ACC_TEXT_W = 40.0f;
    constexpr float GAUGE_W = BOX_W - ACC_LABEL_W - ACC_TEXT_W - 24.0f;
    constexpr float GAUGE_X = BOX_X + ACC_LABEL_W + 8.0f;
    constexpr float GAUGE_H = 10.0f;

    constexpr Color BG_COLOR = {12, 12, 20, 230};
    constexpr Color BORDER_COLOR = {180, 180, 180, 255};
    constexpr Color TEXT_COLOR = {235, 235, 235, 255};
    constexpr Color MUTED_COLOR = {150, 150, 160, 255};
    constexpr Color GAUGE_TRACK = {48, 48, 58, 255};

    Color gaugeColor(float hitChance)
    {
        if (hitChance >= 70.0f)
            return {110, 210, 120, 255};
        if (hitChance >= 40.0f)
            return {230, 190, 90, 255};
        return {220, 100, 100, 255};
    }
}

void DamagePreview::show(const Unit &attacker, const Unit &target, const SkillData *skill, const BattleMap *battleMap)
{
    // Same skill -> HitContext builder the real resolution and the AI use, so
    // the previewed numbers are the numbers that get rolled.
    HitContext ctx = skill ? CombatSystem::hitContextFor(*skill, attacker, target)
                           : CombatSystem::basicAttackContext(attacker, target);
    if (battleMap)
        CombatSystem::applyTileBonuses(ctx, *battleMap);

    const CombatResult preview = CombatSystem::preview(ctx);

    m_visible = true;
    m_damage = preview.damage;
    m_hitChance = preview.hitChance;
    // KO / heal / buff / debuff report no damage, but they still roll accuracy
    // so the gauge stays meaningful for them.
    m_dealsDamage = skill ? skill_effects::hasEffect(*skill, SkillEffectType::Damage)
                          : true;
}

void DamagePreview::hide()
{
    m_visible = false;
}

void DamagePreview::render(Renderer *renderer, const Font *font) const
{
    if (!m_visible || !renderer || !font)
        return;

    Rectf box{BOX_X, BOX_Y, BOX_W, BOX_H};

    renderer->setBlendMode(Renderer::BlendMode::Blend);
    renderer->setDrawColor(BG_COLOR);
    renderer->fillRect(box);
    renderer->setDrawColor(BORDER_COLOR);
    renderer->drawRect(box);

    char buf[64];

    // Damage line — a dash for skills that deal none.
    if (m_dealsDamage)
        std::snprintf(buf, sizeof(buf), "DMG %d", m_damage);
    else
        std::snprintf(buf, sizeof(buf), "---");

    Rectf damageRect{box.x, box.y + 6.0f, box.w, DAMAGE_H};
    renderer->renderTextInRect(font, buf, damageRect, TEXT_COLOR, HorizontalAlign::Center, VerticalAlign::Middle, false, false, false);

    // Accuracy gauge: label, filled bar, percentage.
    const float rowY = box.y + GAUGE_ROW_Y;
    Rectf labelRect{box.x + 8.0f, rowY, ACC_LABEL_W, GAUGE_ROW_H};
    renderer->renderTextInRect(font, "ACC", labelRect, MUTED_COLOR, HorizontalAlign::Left, VerticalAlign::Middle, false, false, false);

    const float clamped = std::clamp(m_hitChance, 0.0f, 100.0f);
    const float fill = GAUGE_W * (clamped / 100.0f);

    Rectf track{GAUGE_X, rowY + (GAUGE_ROW_H - GAUGE_H) * 0.5f, GAUGE_W, GAUGE_H};
    renderer->setDrawColor(GAUGE_TRACK);
    renderer->fillRect(track);
    if (fill > 0.0f)
    {
        renderer->setDrawColor(gaugeColor(clamped));
        renderer->fillRect(Rectf{GAUGE_X, track.y, fill, GAUGE_H});
    }

    std::snprintf(buf, sizeof(buf), "%.0f%%", clamped);
    Rectf pctRect{GAUGE_X + GAUGE_W + 8.0f, rowY, ACC_TEXT_W, GAUGE_ROW_H};
    renderer->renderTextInRect(font, buf, pctRect, TEXT_COLOR, HorizontalAlign::Right, VerticalAlign::Middle, false, false, false);
}
