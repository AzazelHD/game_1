#include "systems/CombatAnimationSystem.h"

#include "engine/renderer/Renderer.h"

#include <algorithm>

void CombatAnimationSystem::enqueue(const std::string &id, float ttl)
{
    m_events.push_back(Event{.id = id, .ttl = std::max(0.0f, ttl)});
}

void CombatAnimationSystem::update(float dt)
{
    for (Event &e : m_events)
        e.ttl -= dt;

    m_events.erase(std::remove_if(m_events.begin(), m_events.end(), [](const Event &e)
                                  { return e.ttl <= 0.0f; }),
                   m_events.end());
}

void CombatAnimationSystem::render(Renderer * /*renderer*/) const
{
    // Placeholder architecture hook. Future implementation will draw
    // skill projectiles, hit effects, and damage indicators.
}

void CombatAnimationSystem::clear()
{
    m_events.clear();
}
