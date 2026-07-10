#include "ui/FloatingTextSystem.h"

#include <algorithm>

void FloatingTextSystem::enqueue(const std::string &text, Vec2i tilePos, Color color, float ttl)
{
    m_entries.push_back(Entry{.text = text, .tilePos = tilePos, .color = color, .ttl = ttl, .age = 0.0f});
}

void FloatingTextSystem::update(float dt)
{
    for (Entry &e : m_entries)
    {
        e.ttl -= dt;
        e.age += dt;
    }
    m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(), [](const Entry &e)
                                   { return e.ttl <= 0.0f; }),
                    m_entries.end());
}

void FloatingTextSystem::render(Renderer * /*renderer*/, const Font * /*font*/) const
{
    // TODO: see header — needs iso/camera transform to place text over the
    // right screen position per m_entries[i].tilePos.
}

void FloatingTextSystem::clear()
{
    m_entries.clear();
}