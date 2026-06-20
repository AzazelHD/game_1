#include "ui/DialogBox.h"

#include <SDL3/SDL.h>

void DialogBox::open(Recti bounds, DialogSide side, Recti portraitBounds)
{
    m_bounds = bounds;
    m_side = side;
    m_portraitBounds = portraitBounds;
    m_hasPortrait = m_portraitBounds.w > 0 && m_portraitBounds.h > 0;
    m_visible = true;
    m_revealedCount = 0;
    m_timer = 0;
}

void DialogBox::close()
{
    m_visible = false;
    m_text.clear();
    m_revealedCount = 0;
    m_hasPortrait = false;
}

void DialogBox::setText(const std::string &text)
{
    m_text = text;
    m_revealedCount = 0;
    m_timer = 0;
}

void DialogBox::update(float dt)
{
    if (!m_visible || isFinished())
    {
        return;
    }

    m_timer += dt;
    while (m_timer >= CHAR_INTERVAL && !isFinished())
    {
        ++m_revealedCount;
        m_timer -= CHAR_INTERVAL;
    }
}

void DialogBox::skipToEnd()
{
    m_revealedCount = static_cast<int>(m_text.size());
}

bool DialogBox::isFinished() const
{
    return m_revealedCount >= static_cast<int>(m_text.size());
}

void DialogBox::render(SDL_Renderer *renderer)
{
    if (!m_visible || renderer == nullptr)
    {
        return;
    }

    SDL_BlendMode previousBlendMode = SDL_BLENDMODE_NONE;
    Uint8 prevR = 0;
    Uint8 prevG = 0;
    Uint8 prevB = 0;
    Uint8 prevA = 0;
    SDL_GetRenderDrawBlendMode(renderer, &previousBlendMode);
    SDL_GetRenderDrawColor(renderer, &prevR, &prevG, &prevB, &prevA);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_FRect dialogRect{(float)m_bounds.x, (float)m_bounds.y, (float)m_bounds.w, (float)m_bounds.h};

    if (m_frameTexture != nullptr)
    {
        SDL_RenderTexture(renderer, m_frameTexture, nullptr, &dialogRect);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 220);
        SDL_RenderFillRect(renderer, &dialogRect);
        SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
        SDL_RenderRect(renderer, &dialogRect);
    }

    if (m_hasPortrait && m_portraitBounds.w > 0 && m_portraitBounds.h > 0)
    {
        SDL_FRect portraitRect{
            (float)m_portraitBounds.x,
            (float)m_portraitBounds.y,
            (float)m_portraitBounds.w,
            (float)m_portraitBounds.h};

        SDL_SetRenderDrawColor(renderer, 90, 90, 90, 255);
        SDL_RenderFillRect(renderer, &portraitRect);

        if (m_side == DialogSide::Left)
        {
            SDL_SetRenderDrawColor(renderer, 120, 180, 255, 255);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 255, 180, 120, 255);
        }

        SDL_RenderRect(renderer, &portraitRect);
    }

    const int padding = 12;
    SDL_FRect textArea{
        dialogRect.x + padding,
        dialogRect.y + padding,
        dialogRect.w - (padding * 2),
        12.f};

    if (textArea.w > 0 && textArea.h > 0)
    {
        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
        SDL_RenderFillRect(renderer, &textArea);

        const float fraction = m_text.empty()
                                   ? 0.0f
                                   : static_cast<float>(m_revealedCount) / static_cast<float>(m_text.size());

        SDL_FRect progress = textArea;
        progress.w = textArea.w * fraction;

        SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
        SDL_RenderFillRect(renderer, &progress);
    }

    SDL_SetRenderDrawColor(renderer, prevR, prevG, prevB, prevA);
    SDL_SetRenderDrawBlendMode(renderer, previousBlendMode);
}

void DialogBox::setPortrait(DialogSide side, Recti portraitBounds)
{
    m_side = side;
    m_portraitBounds = portraitBounds;
    m_hasPortrait = portraitBounds.w > 0 && portraitBounds.h > 0;
}

void DialogBox::clearPortrait()
{
    m_hasPortrait = false;
    m_portraitBounds = Recti{};
}

void DialogBox::setFrameTexture(SDL_Texture *texture)
{
    m_frameTexture = texture;
}