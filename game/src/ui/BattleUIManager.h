class BattleUIManager
{
public:
    explicit BattleUIManager(UIManager &ui);

    // Action menu
    void showActionMenu(std::vector<BattleMenuItem> items);
    void hideActionMenu();
    bool isActionMenuOpen() const;
    const std::vector<BattleMenuItem> &actionMenuItems() const;

    // Inspect
    void showInspect(Unit *unit);
    void hideInspect();

    // Confirm
    void showConfirm(const std::string &text);
    void hideConfirm();

    // Cleanup
    void clear();

private:
    ButtonMenuWindow *ensureActionMenu();
    UnitInspectWindow *ensureInspectWindow();
    ConfirmWindow *ensureConfirmWindow();

    UIManager &m_ui;

    std::vector<BattleMenuItem> m_actionMenuItems;

    ButtonMenuWindow *m_actionMenu = nullptr;
    UnitInspectWindow *m_inspect = nullptr;
    ConfirmWindow *m_confirm = nullptr;
};