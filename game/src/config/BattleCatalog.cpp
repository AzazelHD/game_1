#include "config/BattleCatalog.h"

#include <array>

namespace
{
    const std::array<BattleDefinition, 3> kDefinitions = {
        BattleDefinition{
            .id = "battle_1v1_test",
            .mapPath = "assets/maps/1v1_test.tmj",
            .backgroundTop = FColor{14.0f / 255.0f, 22.0f / 255.0f, 56.0f / 255.0f, 1.0f},
            .backgroundBottom = FColor{26.0f / 255.0f, 62.0f / 255.0f, 130.0f / 255.0f, 1.0f},
            .musicId = "battle_theme_01",
            .defaultZoom = 1.35f,
            .maxUnits = 1,
            .enemies = {
                EnemyDefinition{.id = "soldier", .templatePath = "assets/units/soldier.json", .level = 1, .team = 2},
            },
            .playerUnits = {"roster_selection"},
            .objectives = BattleObjective{.type = "defeat_all"},
            .rewards = BattleRewards{.xp = 100, .gold = 120, .items = {"starter_ring"}},
            .events = {},
        },
        BattleDefinition{
            .id = "battle_test_map",
            .mapPath = "assets/maps/test_map.tmj",
            .backgroundTop = FColor{28.0f / 255.0f, 16.0f / 255.0f, 44.0f / 255.0f, 1.0f},
            .backgroundBottom = FColor{66.0f / 255.0f, 30.0f / 255.0f, 92.0f / 255.0f, 1.0f},
            .musicId = "battle_theme_02",
            .defaultZoom = 1.30f,
            .maxUnits = 6,
            .enemies = {
                EnemyDefinition{.id = "soldier", .templatePath = "assets/units/soldier.json", .level = 1, .team = 2},
                EnemyDefinition{.id = "soldier", .templatePath = "assets/units/soldier.json", .level = 1, .team = 2},
                EnemyDefinition{.id = "soldier", .templatePath = "assets/units/soldier.json", .level = 1, .team = 2},
            },
            .playerUnits = {"roster_selection"},
            .forcedUnits = {
                ForcedUnitRule{.unitRef = "Aria", .position = Vec2i{2, 3}, .isCritical = true},
            },
            .objectives = BattleObjective{.type = "defeat_all"},
            .rewards = BattleRewards{.xp = 250, .gold = 300, .items = {"starter_boots", "starter_ring"}},
            .events = {
                BattleEvent{.trigger = BattleTriggerType::OnDefeat, .actions = {
                                                                        BattleEventAction{.type = BattleActionType::ShowDialogue, .text = "Retreat and regroup."},
                                                                    }},
            },
            .defeatRule = BattleDefeatRule{.type = DefeatConditionType::CriticalUnitDied},
        },
        BattleDefinition{
            .id = "battle_boss_arena",
            .mapPath = "assets/maps/boss_arena.tmj",
            .backgroundTop = FColor{40.0f / 255.0f, 8.0f / 255.0f, 12.0f / 255.0f, 1.0f},
            .backgroundBottom = FColor{90.0f / 255.0f, 24.0f / 255.0f, 18.0f / 255.0f, 1.0f},
            .musicId = "battle_theme_02",
            .defaultZoom = 1.25f,
            .maxUnits = 3,
            .enemies = {
                EnemyDefinition{.id = "bandit_chief", .templatePath = "assets/units/bandit_chief.json", .level = 1, .team = 2},
            },
            .playerUnits = {"roster_selection"},
            .objectives = BattleObjective{.type = "defeat_boss"},
            .rewards = BattleRewards{.xp = 350, .gold = 450, .items = {"starter_boots", "starter_ring"}},
            .events = {
                // Wave 1: the chief calls in a raider the moment combat starts.
                BattleEvent{.trigger = BattleTriggerType::OnTurnStart,
                            .minTurn = 1,
                            .once = true,
                            .actions = {BattleEventAction{.type = BattleActionType::SpawnUnit, .unitTemplatePath = "assets/units/raider.json"},
                                        BattleEventAction{.type = BattleActionType::ShowDialogue, .text = "The chief whistles — a raider rushes in from the flank!"}}},
                // Wave 2: when the fight drags on, two reservists pile in. Only once.
                BattleEvent{.trigger = BattleTriggerType::OnTurnStart,
                            .minTurn = 4,
                            .once = true,
                            .actions = {BattleEventAction{.type = BattleActionType::SpawnUnit, .unitTemplatePath = "assets/units/raider.json"},
                                        BattleEventAction{.type = BattleActionType::SpawnUnit, .unitTemplatePath = "assets/units/raider.json"},
                                        BattleEventAction{.type = BattleActionType::ShowDialogue, .text = "Raiders pour in from the treeline! The chief's reserves answer the call."}}},
            },
            .victoryRule = BattleVictoryRule{.type = VictoryConditionType::KillBoss},
        },
    };

    const BattleDefinition kFallback = {
        .id = "battle_fallback",
        .mapPath = "assets/maps/1v1_test.tmj",
        .backgroundTop = FColor{10.0f / 255.0f, 18.0f / 255.0f, 55.0f / 255.0f, 1.0f},
        .backgroundBottom = FColor{25.0f / 255.0f, 60.0f / 255.0f, 120.0f / 255.0f, 1.0f},
        .musicId = "battle_theme_01",
        .defaultZoom = 1.35f,
        .maxUnits = 1,
        .enemies = {
            EnemyDefinition{.id = "soldier", .templatePath = "assets/units/soldier.json", .level = 1, .team = 2},
        },
        .playerUnits = {"roster_selection"},
        .objectives = BattleObjective{.type = "defeat_all"},
        .rewards = BattleRewards{.xp = 100, .gold = 120, .items = {}},
        .events = {},
    };
} // namespace

const BattleDefinition &getBattleDefinition(const std::string &mapPath)
{
    for (const BattleDefinition &def : kDefinitions)
    {
        if (def.mapPath == mapPath)
            return def;
    }
    return kFallback;
}
