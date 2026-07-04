#pragma once

#include "ui/UIEvent.h"

#include <memory>
#include <string>
#include <vector>

class Input;
class Renderer;
class UIWindow;

class UIManager
{
public:
    UIManager() = default;

    template <typename TWindow, typename... TArgs>
    TWindow *push(TArgs &&...args)
    {
        auto window = std::make_unique<TWindow>(std::forward<TArgs>(args)...);
        TWindow *ptr = window.get();
        wireEventEmitter(*window);
        window->OnEnterTransition();
        m_stack.push_back(std::move(window));
        return ptr;
    }

    template <typename TWindow, typename... TArgs>
    TWindow *replaceTop(TArgs &&...args)
    {
        if (!m_stack.empty() && m_stack.back())
        {
            m_stack.back()->OnReplaceTransition();
            m_stack.back()->OnExitTransition();
            m_stack.pop_back();
        }
        return push<TWindow>(std::forward<TArgs>(args)...);
    }

    void popTop();
    void popById(const std::string &id);
    bool hasWindow(const std::string &id) const;
    void clear();

    void handleInput(const Input &input);
    void update(float dt);
    void render(Renderer *renderer) const;

    bool hasBlockingWindow() const;
    bool empty() const { return m_stack.empty(); }

    std::vector<UIEvent> drainEvents();

private:
    void wireEventEmitter(UIWindow &window);

    std::vector<std::unique_ptr<UIWindow>> m_stack;
    std::vector<UIEvent> m_eventQueue;
};
