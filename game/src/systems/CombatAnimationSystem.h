#pragma once

#include <string>
#include <vector>

class Renderer;

class CombatAnimationSystem
{
public:
    struct Event
    {
        std::string id;
        float ttl = 0.0f;
    };

    void enqueue(const std::string &id, float ttl = 0.2f);
    void update(float dt);
    void render(Renderer *renderer) const;
    void clear();

private:
    std::vector<Event> m_events;
};
