#pragma once

#include "ui/UIWindow.h"
#include "ui/WindowId.h"

#include <string>

class Font;

class DeploymentWindow : public UIWindow
{
public:
    explicit DeploymentWindow(WindowId id);

    void setFont(const Font *font) { m_font = font; }
    void setSelectedUnitLabel(std::string label) { m_selectedUnitLabel = std::move(label); }
    void setGrabbedState(bool grabbed, std::string label)
    {
        m_hasGrabbedUnit = grabbed;
        m_grabbedUnitLabel = std::move(label);
    }
    void setDeploymentStatus(int placed, int maxUnits, bool canStart);

    void handleInput(const Input &input) override;
    void update(float dt) override;
    void render(Renderer *renderer) const override;

private:
    const Font *m_font = nullptr;
    std::string m_selectedUnitLabel;
    std::string m_grabbedUnitLabel;
    int m_placed = 0;
    int m_maxUnits = 1;
    bool m_canStart = false;
    bool m_hasGrabbedUnit = false;
};
