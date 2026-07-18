#pragma once

#include "ui/UIEvent.h"
#include "ui/WindowId.h"

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
    void popById(WindowId id);
    // Hides a window in place instead of destroying it (see UIWindow::
    // setVisible). Use this instead of popById for persistent windows —
    // popById would erase the unique_ptr and dangle any raw pointer an
    // owner (e.g. BattleHud) cached for reuse.
    void hideById(WindowId id);
    bool hasWindow(WindowId id) const;
    void clear();

    void handleInput(const Input &input);
    void update(float dt);
    void render(Renderer *renderer) const;

    bool hasBlockingWindow() const;
    bool empty() const { return m_stack.empty(); }

    std::vector<UIEvent> drainEvents();

private:
    void wireEventEmitter(UIWindow &window);
    // Topmost window that isn't hidden — the real "top of stack" once
    // persistent/hidden windows are taken into account.
    UIWindow *topVisibleWindow() const;

    std::vector<std::unique_ptr<UIWindow>> m_stack;
    std::vector<UIEvent> m_eventQueue;
};
