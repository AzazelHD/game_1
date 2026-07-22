#pragma once

#include "engine/ui/FocusGroup.h"
#include "engine/ui/IFocusable.h"
#include "ui/WindowId.h"
#include "ui/UIWindow.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class Font;
class Renderer;

class PartyWindow : public UIWindow
{
public:
    struct Entry
    {
        std::string name;
        std::string templatePath; // needed so Accept can open InspectWindow for this unit
        bool hasSprite = false;
    };

    explicit PartyWindow(WindowId id);

    void setFont(const Font *font) { m_font = font; }
    void setEntries(std::vector<Entry> entries);

    void handleInput(const Input &input) override;
    void update(float dt) override;
    void render(Renderer *renderer) const override;

private:
    // Thin IFocusable wrapper per row — just enough for FocusGroup to drive
    // Up/Down navigation and Accept. PartyWindow keeps its own custom
    // rendering (icons, scroll window), so this doesn't need the full
    // IRowControl interface that SettingsRowWindow's layout engine wants.
    class EntryFocusable : public IFocusable
    {
    public:
        explicit EntryFocusable(std::function<bool()> onActivate)
            : m_onActivate(std::move(onActivate))
        {
        }

        bool activate() const override { return m_onActivate ? m_onActivate() : false; }
        void setSelected(bool selected) override { m_selected = selected; }
        bool isEnabled() const override { return true; }

    private:
        std::function<bool()> m_onActivate;
        bool m_selected = false;
    };

    const Font *m_font = nullptr;
    std::vector<Entry> m_entries;
    std::vector<std::unique_ptr<EntryFocusable>> m_focusables;
    FocusGroup m_focus;
    int m_scroll = 0;
};
