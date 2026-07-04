#pragma once

#include "ui/UIEvent.h"

#include <functional>
#include <string>

class Input;
class Renderer;

class UIWindow
{
public:
    explicit UIWindow(std::string id, bool blocksInputBelow, bool blocksRenderBelow = false)
        : m_id(std::move(id)),
          m_blocksInputBelow(blocksInputBelow),
          m_blocksRenderBelow(blocksRenderBelow)
    {
    }

    virtual ~UIWindow() = default;

    const std::string &id() const { return m_id; }
    bool blocksInputBelow() const { return m_blocksInputBelow; }
    bool blocksRenderBelow() const { return m_blocksRenderBelow; }

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
    std::string m_id;
    bool m_blocksInputBelow = true;
    bool m_blocksRenderBelow = false;
    std::function<void(const UIEvent &)> m_emitEvent;
};
