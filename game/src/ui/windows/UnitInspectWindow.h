#pragma once

#include "ui/WindowId.h"
#include "ui/UIWindow.h"

#include <string>
#include <vector>

class Font;
class Renderer;
struct UnitData;

class UnitInspectWindow : public UIWindow
{
public:
    enum class Relation
    {
        Player,
        Ally,
        Enemy,
    };

    struct StatLine
    {
        std::string label;
        std::string value;
    };

    // A named group of lines. Only "Stats" exists today — Skills/Gear/etc.
    // can be added later as additional sections without touching render().
    struct Section
    {
        std::string title;
        std::vector<StatLine> lines;
    };

    explicit UnitInspectWindow(WindowId id);

    void setFont(const Font *font) { m_font = font; }
    void setRelation(Relation relation) { m_relation = relation; }
    void setHeader(std::string name, std::string job)
    {
        m_name = std::move(name);
        m_job = std::move(job);
    }
    void setSections(std::vector<Section> sections) { m_sections = std::move(sections); }

    // Builds the one section this window shows today: core combat stats
    // pulled straight from effective (already race/gender-adjusted)
    // UnitData. No team, no affinities, nothing else.
    static Section buildStatsSection(const UnitData &data);

    void handleInput(const Input &input) override;
    void update(float dt) override;
    void render(Renderer *renderer) const override;

private:
    const Font *m_font = nullptr;
    std::string m_name;
    std::string m_job;
    std::vector<Section> m_sections;
    Relation m_relation = Relation::Player;
};
