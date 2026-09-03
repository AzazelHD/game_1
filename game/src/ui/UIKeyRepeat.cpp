#include "ui/UIKeyRepeat.h"

void UIKeyRepeat::start(float initialDelaySeconds, float intervalSeconds)
{
    m_initialDelay = initialDelaySeconds;
    m_interval = intervalSeconds;
}

void UIKeyRepeat::reset()
{
    m_wasDown = false;
    m_initialFired = false;
    m_heldFor = 0.f;
    m_sinceLastFire = 0.f;
}

bool UIKeyRepeat::tick(float dt, bool isCurrentlyDown)
{
    // Key released: clear state, wait for next down-edge.
    if (!isCurrentlyDown)
    {
        m_wasDown = false;
        m_initialFired = false;
        m_heldFor = 0.f;
        m_sinceLastFire = 0.f;
        return false;
    }

    // Down-edge: fire immediately and start the hold timers.
    if (!m_wasDown)
    {
        m_wasDown = true;
        m_initialFired = true;
        m_heldFor = 0.f;
        m_sinceLastFire = 0.f;
        return true;
    }

    // Already down. Don't re-fire until at least the initial delay has
    // elapsed — until then `m_initialFired` guards against any spurious
    // edge inside the same press.
    m_heldFor += dt;
    m_sinceLastFire += dt;
    if (m_heldFor < m_initialDelay)
        return false;

    // Steady-state: fire when the interval has elapsed.
    if (m_sinceLastFire >= m_interval)
    {
        m_sinceLastFire -= m_interval;
        return true;
    }
    return false;
}
