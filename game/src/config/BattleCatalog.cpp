#include "config/BattleCatalog.h"

#include <array>

namespace
{
    const std::array<BattleDefinition, 2> kDefinitions = {
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
            .rewards = BattleRewards{.xp = 100, .items = {}},
            .events = {
                BattleEvent{.trigger = BattleTriggerType::OnVictory, .actions = {
                                                                         BattleEventAction{.type = BattleActionType::GiveReward, .value = 100},
                                                                     }},
            },
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
            .rewards = BattleRewards{.xp = 250, .items = {}},
            .events = {
                BattleEvent{.trigger = BattleTriggerType::OnDefeat, .actions = {
                                                                        BattleEventAction{.type = BattleActionType::ShowDialogue, .text = "Retreat and regroup."},
                                                                    }},
            },
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
        .rewards = BattleRewards{.xp = 100, .items = {}},
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
