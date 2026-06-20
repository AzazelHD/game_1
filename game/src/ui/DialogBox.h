#pragma once

#include <SDL3/SDL.h>
#include <string>

#include "engine/math/Rect.h"

enum class DialogSide
{
    Left,
    Right
};

// DialogBox is a game-owned presentation widget.
// It renders a framed text box with optional portrait placement and typewriter timing.
class DialogBox
{
public:
    void open(Recti bounds, DialogSide side = DialogSide::Left, Recti portraitBounds = Recti{});
    void close();
    void setText(const std::string &text);
    void update(float dt);
    void skipToEnd();
    bool isFinished() const;
    void render(SDL_Renderer *renderer);
    void setPortrait(DialogSide side, Recti portraitBounds);
    void clearPortrait();
    void setFrameTexture(SDL_Texture *texture);

private:
    Recti m_bounds;
    std::string m_text;
    int m_revealedCount = 0;
    float m_timer = 0.f;
    bool m_visible = false;
    DialogSide m_side = DialogSide::Left;
    Recti m_portraitBounds{};
    bool m_hasPortrait = false;
    SDL_Texture *m_frameTexture = nullptr;

    static constexpr float CHAR_INTERVAL = 0.03f;
};