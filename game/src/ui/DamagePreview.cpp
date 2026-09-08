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

#include <cstdio>

namespace
{
    constexpr float BOX_X = GameConstants::VIEW_W - 240.0f;
    constexpr float BOX_Y = 80.0f;
    constexpr float BOX_W = 220.0f;
    constexpr float BOX_H = 42.0f;

    constexpr Color BG_COLOR = {12, 12, 20, 230};
    constexpr Color BORDER_COLOR = {180, 180, 180, 255};
    constexpr Color TEXT_COLOR = {235, 235, 235, 255};
}

void DamagePreview::show(const Unit &attacker, const Unit &target, const SkillData *skill)
{
    HitContext ctx;
    ctx.attacker = &attacker;
    ctx.target = &target;
    if (skill)
    {
        ctx.basePower = skill->basePower;
        ctx.isMagical = skill->isMagical;
        ctx.element = skill->element;
        ctx.skillAccuracy = skill->skillAccuracy;
    }
    else
    {
        ctx.basePower = 0;
        ctx.isMagical = false;
        ctx.element = Element::Neutral;
        ctx.skillAccuracy = 95;
    }
    ctx.side = AttackSide::Side; // will be dynamic later
    ctx.tileEvasionBonus = 0;

    m_visible = true;
    if (skill)
        m_label = skill_effects::describeSkill(*skill, ctx);
    else
    {
        char buf[64];
        const CombatResult preview = CombatSystem::preview(ctx);
        std::snprintf(buf, sizeof(buf), "DMG: %d   HIT: %.0f%%", preview.damage, preview.hitChance);
        m_label = buf;
    }
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

    // Panel background
    renderer->setBlendMode(Renderer::BlendMode::Blend);
    renderer->setDrawColor(BG_COLOR);
    renderer->fillRect(box);
    renderer->setDrawColor(BORDER_COLOR);
    renderer->drawRect(box);

    // Draw text centred inside the box
    renderer->renderTextInRect(font, m_label.c_str(), box, TEXT_COLOR, HorizontalAlign::Center, VerticalAlign::Middle, false, false, false);
}