#pragma once

#include "ui/UIEvent.h"

#include <functional>

class Input;
class Renderer;

class UIWindow
{
public:
    explicit UIWindow(WindowId id, bool blocksInputBelow, bool blocksRenderBelow = false)
        : m_id(id),
          m_blocksInputBelow(blocksInputBelow),
          m_blocksRenderBelow(blocksRenderBelow)
    {
    }

    virtual ~UIWindow() = default;

    WindowId id() const { return m_id; }
    bool blocksInputBelow() const { return m_blocksInputBelow; }
    bool blocksRenderBelow() const { return m_blocksRenderBelow; }

    // Persistent-window support: a hidden window stays on the UIManager
    // stack (owned, alive, state preserved) but is skipped by handleInput/
    // render/hasBlockingWindow, and hasWindow(id) reports it as absent.
    // Lets owners like BattleHud create a window once and toggle it instead
    // of push/pop-ing (and thereby destroying/reconstructing) it every time.
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }

    virtual void handleInput(const Input &input) = 0;
    virtual void update(float dt) = 0;
    virtual void render(Renderer *renderer) const = 0;

    // Optional UI transition hooks. Default behavior stays instant.
    virtual void OnEnterTransition() {}
    virtual void OnExitTransition() {}
    virtual void OnReplaceTransition() {}

    void setEventEmitter(std::function<void(const UIEvent &)> emitter)
    {
        m_emitEvent = std::move(emitter);
    }

protected:
    void emit(const UIEvent &event) const
    {
        if (m_emitEvent)
            m_emitEvent(event);
    }

private:
    WindowId m_id;
    bool m_blocksInputBelow = true;
    bool m_blocksRenderBelow = false;
    bool m_visible = true;
    std::function<void(const UIEvent &)> m_emitEvent;
};
