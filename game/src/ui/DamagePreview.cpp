#include "ui/DamagePreview.h"
#include "battle/Unit.h"
#include "battle/CombatSystem.h"
#include "data/SkillLoader.h"
#include "engine/renderer/Renderer.h"
#include "engine/renderer/Font.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "config/GameConstants.h"

#include <algorithm>

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

    CombatResult preview = CombatSystem::preview(ctx);
    m_visible = true;
    m_damage = preview.damage;
    m_hitChance = preview.hitChance;
    m_critPossible = false;
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

    // Build the text line(s)
    char line1[64];
    std::snprintf(line1, sizeof(line1), "DMG: %d   HIT: %.0f%%", m_damage, m_hitChance);

    // Draw text centred inside the box
    const float textX = box.x + 10.0f;
    const float textY = box.y + (box.h - 8.0f) * 0.5f;
    renderer->renderText(font, line1, Vec2f{textX, textY}, TEXT_COLOR, false, false, false);
}
