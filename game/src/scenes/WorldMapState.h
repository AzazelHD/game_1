#pragma once

#include "engine/effects/ScreenTransition.h"
#include "engine/math/Vec2.h"
#include "engine/scene/Scene.h"
#include "engine/statemachine/StateMachine.h"
#include "data/TavernData.h"
#include "ui/UIManager.h"
#include "ui/windows/ShopWindow.h"
#include "world/WorldGraph.h"

#include <string>
#include <unordered_map>
#include <vector>

class ButtonMenuWindow;
class Renderer;

class WorldMapState final : public Scene
{
public:
    WorldMapState(StateMachine<Scene> &sm, Renderer *renderer, int startNodeId = 4,
                  int wonNodeId = -1);

    void onEnter() override;
    void onExit() override;
    void handleInput() override;
    void update(float dt) override;
    void render(float alpha) override;

private:
    enum class NodeType
    {
        Battle,
        Empty,
        Town,
    };

    struct NodeMeta
    {
        NodeType type = NodeType::Empty;
        std::string battleMapPath;
        std::string description = "Empty Node";
    };

    struct PendingBattle
    {
        int nodeId = -1;
        std::string mapPath;
    };

    // An interaction waiting at a node (FFTA-style). One per active quest
    // whose objective targets the node, plus one generic "Battle" event when
    // a battle node has no quest calling on it. Any non-empty event set opens
    // a chooser; Enter on the entry runs it.
    struct NodeEvent
    {
        std::string label;
        std::string questId; // empty for the generic battle event
        bool startBattle = false;
    };

private:
    void buildDefaultWorld();
    void updateCursor(float dt);
    void updateHoverNode();
    void applyNodeMagnetism(float dt);
    void updateCamera();
    void movePlayerToNode(int nodeId);
    void updatePlayerMovement(float dt);
    void onArriveAtNode(int nodeId);
    void collectNodeEvents(int nodeId);
    void presentNodeEvents();
    void playNodeEvent(int index);
    void requestBattleAtNode(int nodeId);
    void completeQuestObjectives(int nodeId, bool victorious);
    void openTownHub();
    void leaveTownHub();
    void openTownTalk();
    void openShopWindow();
    void openShopList(ShopWindow::Mode mode);
    void openTavernHub();
    void handleShopEvent(const UIEvent &event);
    void handleTavernEvent(const UIEvent &event);
    void openTavernQuestBoard();
    void showTavernQuestDetail(int index);
    void openTavernRecruitList();
    void refreshTavernQuestBoard();
    void refreshTavernRecruitList();
    void openTavernTalk();
    void performRecruit(std::size_t candidateIndex);
    void processUIEvents();
    void showNodeInfo(int nodeId);
    void openSaveList();
    void refreshSaveList();
    void closeSaveList();
    void refreshWorldMapMenuItems();

    void openJournal();
    void closeJournal();
    void showQuestDetail(int questIndex);

    void startBattle(PendingBattle battle);

    void drawWorld() const;

    std::string nodeNumberLabel(int id) const;

private:
    StateMachine<Scene> &m_sm;
    Renderer *m_renderer = nullptr;

    world::WorldGraph m_graph;
    std::unordered_map<int, Vec2i> m_nodeGridPos;
    std::unordered_map<int, NodeMeta> m_nodeMeta;
    std::vector<int> m_partyEntryInstanceIds;
    int m_activePartyDetailInstanceId = -1;

    int m_playerNodeId = 4;
    int m_hoveredNodeId = -1;
    Vec2f m_cursorPos{0.0f, 0.0f};
    Vec2f m_cameraPos{0.0f, 0.0f};

    float m_cursorSpeed = 380.0f;
    float m_nodeHitRadius = 24.0f;
    float m_magnetRadius = 28.0f;
    float m_magnetStrength = 6.0f;
    float m_worldMargin = 120.0f;

    bool m_isMoving = false;
    std::vector<int> m_path;
    std::size_t m_pathSegment = 0;
    float m_segmentProgress = 0.0f;
    float m_moveSpeed = 3.5f;

    PendingBattle m_pendingBattle;
    int m_wonNodeId = -1;
    UIManager m_uiManager;

    // Fade-in on enter, so arriving from a battle (which fades out to black)
    // crossfades instead of hard-cutting to a fully lit map. While it is
    // active the world map ignores input and cursor motion.
    ScreenTransition m_transition;

    std::vector<NodeEvent> m_nodeEvents;
    ButtonMenuWindow *m_nodeEventWindow = nullptr;

    ButtonMenuWindow *m_worldMapMenu = nullptr;
    ButtonMenuWindow *m_townHubMenu = nullptr;
    ButtonMenuWindow *m_saveListWindow = nullptr;
    ButtonMenuWindow *m_journalWindow = nullptr;
    ButtonMenuWindow *m_shopHubMenu = nullptr;
    ShopWindow *m_shopWindow = nullptr;
    ButtonMenuWindow *m_tavernHub = nullptr;
    ButtonMenuWindow *m_tavernBoardWindow = nullptr;
    ButtonMenuWindow *m_tavernRecruitWindow = nullptr;
    std::vector<int> m_saveListSlotIndexes;
    std::vector<int> m_journalQuestIndexes; // indices into QuestLog::all()
    int m_pendingDeleteSlotIndex = -1;
    int m_pendingShopIndex = -1;  // row index for the in-flight shop confirm
    bool m_pendingShopBuy = false; // true=buy, false=sell

    std::vector<std::string> m_tavernQuestIds;      // ids of available tavern quests (in list order)
    std::vector<std::size_t> m_tavernCandidateIndexes; // candidate index per recruit-list row
    std::vector<TavernCandidate> m_tavernCandidates;
    std::vector<TavernLine> m_tavernLore;
    bool m_tavernDataLoaded = false;
    std::size_t m_pendingRecruitIndex = 0;
    // Quest awaiting its accept confirmation (TavernQuestDetail → TavernQuestAccept).
    std::string m_pendingQuestId;
    std::vector<TavernLine> m_townTalk;
    bool m_townTalkLoaded = false;
};
