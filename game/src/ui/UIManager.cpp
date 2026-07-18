#include "ui/UIManager.h"

#include "engine/input/Input.h"
#include "ui/UIWindow.h"
#include "ui/WindowId.h"

#include <algorithm>
#include <utility>

void UIManager::popTop()
{
    if (!m_stack.empty())
    {
        if (m_stack.back())
            m_stack.back()->OnExitTransition();
        m_stack.pop_back();
    }
}

void UIManager::popById(WindowId id)
{
    auto it = std::find_if(m_stack.rbegin(), m_stack.rend(),
                           [&id](const std::unique_ptr<UIWindow> &w)
                           { return w && w->id() == id; });

    if (it == m_stack.rend())
        return;

    auto eraseIt = std::next(it).base();
    if (*eraseIt)
        (*eraseIt)->OnExitTransition();
    m_stack.erase(eraseIt);
}

void UIManager::hideById(WindowId id)
{
    for (auto &window : m_stack)
    {
        if (window && window->id() == id)
        {
            window->setVisible(false);
            return;
        }
    }
}

bool UIManager::hasWindow(WindowId id) const
{
    // Visible-only by design: every prior call site (BattleState's ESC/
    // update-phase checks, BattleHud::isOpen) uses this to mean "is this
    // window currently showing", which is exactly what it meant back when
    // "not showing" implied "not on the stack at all". A hidden-but-alive
    // persistent window must keep reporting as absent here.
    return std::any_of(m_stack.begin(), m_stack.end(), [&id](const std::unique_ptr<UIWindow> &w)
                       { return w && w->id() == id && w->isVisible(); });
}

void UIManager::clear()
{
    for (auto &window : m_stack)
    {
        if (window)
            window->OnExitTransition();
    }
    m_stack.clear();
    m_eventQueue.clear();
}

void UIManager::handleInput(const Input &input)
{
    UIWindow *top = topVisibleWindow();
    if (top)
        top->handleInput(input);
}

void UIManager::update(float dt)
{
    for (const auto &window : m_stack)
    {
        if (window)
            window->update(dt);
    }
}

void UIManager::render(Renderer *renderer) const
{
    std::size_t beginIndex = 0;
    for (std::size_t i = m_stack.size(); i > 0; --i)
    {
        const UIWindow *window = m_stack[i - 1].get();
        if (window && window->isVisible() && window->blocksRenderBelow())
        {
            beginIndex = i - 1;
            break;
        }
    }

    for (std::size_t i = beginIndex; i < m_stack.size(); ++i)
    {
        const auto &window = m_stack[i];
        if (window && window->isVisible())
            window->render(renderer);
    }
}

bool UIManager::hasBlockingWindow() const
{
    const UIWindow *top = topVisibleWindow();
    return top ? top->blocksInputBelow() : false;
}

std::vector<UIEvent> UIManager::drainEvents()
{
    std::vector<UIEvent> events;
    events.swap(m_eventQueue);
    return events;
}

UIWindow *UIManager::topVisibleWindow() const
{
    for (auto it = m_stack.rbegin(); it != m_stack.rend(); ++it)
    {
        if (*it && (*it)->isVisible())
            return it->get();
    }
    return nullptr;
}

void UIManager::wireEventEmitter(UIWindow &window)
{
    window.setEventEmitter([this](const UIEvent &event)
                           { m_eventQueue.push_back(event); });
}
