#include "events/BattleEventSystem.h"

void BattleEventSystem::initialize(const BattleDefinition &definition, Callbacks callbacks)
{
    m_events = definition.events;
    m_callbacks = std::move(callbacks);
    m_variables.clear();
}

void BattleEventSystem::emit(BattleTriggerType trigger)
{
    for (const BattleEvent &ev : m_events)
    {
        if (ev.trigger != trigger)
            continue;

        for (const BattleEventAction &action : ev.actions)
            executeAction(action);
    }
}

void BattleEventSystem::executeAction(const BattleEventAction &action)
{
    switch (action.type)
    {
    case BattleActionType::SpawnUnit:
        if (m_callbacks.spawnUnitByTemplate && !action.unitTemplatePath.empty())
            m_callbacks.spawnUnitByTemplate(action.unitTemplatePath);
        break;

    case BattleActionType::ShowDialogue:
        if (m_callbacks.showDialogue && !action.text.empty())
            m_callbacks.showDialogue(action.text);
        break;

    case BattleActionType::GiveReward:
        if (m_callbacks.giveRewardXp)
            m_callbacks.giveRewardXp(action.value);
        break;

    case BattleActionType::PlayAnimation:
        if (m_callbacks.playAnimation)
            m_callbacks.playAnimation(action.text);
        break;

    case BattleActionType::ModifyVariable:
        m_variables[action.text] += action.value;
        break;

    case BattleActionType::StartCutscene:
        if (m_callbacks.startCutscene)
            m_callbacks.startCutscene(action.text);
        break;

    case BattleActionType::EndBattle:
        if (m_callbacks.endBattle)
            m_callbacks.endBattle(action.flag);
        break;
    }
}
