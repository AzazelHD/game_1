# Graph Report - game_1  (2026-10-06)

## Corpus Check
- 181 files · ~91,512 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 5 file(s) not represented in the graph (top: .bat 3, (none) 2)

## Summary
- 3420 nodes · 6514 edges · 186 communities (155 shown, 31 thin omitted)
- Extraction: 92% EXTRACTED · 8% INFERRED · 0% AMBIGUOUS · INFERRED: 513 edges (avg confidence: 0.86)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `6fee23e1`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- BattleState Orchestration Shell
- SaveManager.h
- DeploymentPhaseController.cpp
- Gear Catalog Lookup
- World Map Scene
- Battle Session Flow
- Quest Data Loading
- Runtime Unit State
- vector
- UI Action Identifiers
- Battle Menu Controller
- Gear Subtype Hierarchy
- CombatAnimationSystem
- MenuContext
- ConfirmWindow
- UI Window Identifiers
- Unit Detail Window
- MovementAnimationController
- Skill Progression UI
- ButtonMenuWindow
- BattleState.cpp
- algorithm
- IntroState
- PartyContext
- Unit Panel Window
- .findPath
- Skill Progression Catalog
- Renderer
- Attack Resolution Controller
- Recruitment and Promotion Rules
- Inventory Window
- MainMenuState.cpp
- Element and Affinity Enums
- WorldMapState.cpp
- Turn Queue Scheduling
- World Projection Anchors
- AudioSettings.cpp
- PendingAttackController
- TavernCandidate
- Battle Renderer Draw Pass
- Unit Data Schema
- Skill Data Schema
- GraphicsSettings.cpp
- Battle Dialogue Controller
- CombatSystem
- Battle Grid
- BattleLoadResult
- Render Context Overlays
- UI Manager Window Stack
- Shop Window
- Game Constants and Settings
- DeploymentWindow
- ButtonMenuWindow.cpp
- SettingsState.cpp
- EnemyAI.h
- Roster System
- WorldGraph
- .chooseAction
- Hit Context and Attack Side
- Unit Inspect Window
- StatusType
- Unit Stat Accessors
- ResultState.cpp
- Unit Portrait Rendering
- Battle Map Model
- Unit Movement Points
- Unit.cpp
- Settings Persistence Schema
- UIWindow
- TRPG Game Architecture
- BootState.cpp
- Promotion Class Roster
- Equipment Eligibility Rules
- ShopWindow.h
- .processUIEvents
- Deployment System
- Progression Skill Definitions
- BattleDefinition
- World Node Metadata
- DialogWindow.cpp
- PartyWindow
- SettingsRowWindow.cpp
- Inspect Window Rendering
- .hitContextFor
- CombatAnimationSystem.cpp
- Grid Cursor
- CampaignSave
- BattleEventSystem
- EnemyAI.cpp
- DamagePreview
- PartySystem
- Race Growth Bonuses
- Race System Design
- UI Key Repeat Helper
- .commitMovement
- BattleLoader.h
- Roster Unit Schema
- UIManager Declarations
- Battle Module Ownership
- Module Responsibilities
- Skill System Design
- Unit Stat Bonuses
- Class Growth Curves
- Race Equip Slot Configuration
- Gear Stat Modifiers
- Unit.h
- Unit Detail Dependencies
- PendingAttackController.h
- Deployment Placement Logic
- QuestLoader.cpp
- MovementCommitSystem.h
- SkillLoader.cpp
- Floating Text System
- Shop Window Logic
- Shop Sell Row Schema
- Equipment Slot Entries
- ConfirmWindow
- Unit Lifecycle State
- Battle Renderer Dependencies
- Unit Render Proxy
- Battle Tutorial Tips
- SaveManager
- Settings Row Item Schema
- UIEventType
- Scene Flow and World Graph
- BattleMap
- TileMapData
- Shot
- Status Stat Effect Totals
- Battle Action Types
- Battle Objectives and Enemies
- BattleEvent
- SettingsRowWindow
- Gear Slot Enum
- Facing Dial Rendering
- Confirm Window Rendering
- Shop Window Input
- TRPG Game Data Schemas
- Combat Result Schema
- Game Tile Schema
- Major Action Budget
- Battle Menu Item
- UI Event Struct
- Item Candidate Entry
- CMake Source Lists
- BattleRequest
- Unit Action Type Enum
- SaveSlotInfo
- Skill Effect Types
- Tile Layer Reference
- UIEvent
- Unit Detail Construction
- vcpkg Dependencies
- Status Effect Application
- Status Tick Result
- DamagePreview.cpp
- Skill Visual Definition
- Battle Overlay Modes
- BattleState.h
- .render
- Settings Row Geometry
- Menu Text Alignment
- Temporary Stat Modifier
- Menu Geometry Cache
- Shop Buy Row
- Shop Window Mode
- Unit Detail UI State
- Engine Package Discovery
- Inventory Window Construction
- Settings Module Ownership
- StatusTickResult
- PendingResolution
- SegmentTiming
- AudioSettings::AudioSettings
- ConfirmDialogs.h

## God Nodes (most connected - your core abstractions)
1. `BattleState` - 189 edges
2. `Unit` - 118 edges
3. `WorldMapState` - 117 edges
4. `WindowId` - 93 edges
5. `UnitDetailWindow` - 77 edges
6. `ActionId` - 67 edges
7. `PartyContext` - 66 edges
8. `ButtonMenuWindow` - 59 edges
9. `ShopWindow` - 45 edges
10. `DeploymentSystem` - 42 edges

## Surprising Connections (you probably didn't know these)
- `Controllers` --references--> `MovementAnimationController`  [INFERRED]
  ARCHITECTURE.md → game/src/battle/controllers/MovementAnimationController.h
- `Current Gaps and Follow-Ups` --references--> `CombatAnimationSystem`  [INFERRED]
  ARCHITECTURE.md → game/src/battle/systems/CombatAnimationSystem.h
- ``events/`` --references--> `BattleEventSystem`  [INFERRED]
  ARCHITECTURE.md → game/src/events/BattleEventSystem.h
- `Item definition (runtime)` --references--> `ItemDefinition`  [INFERRED]
  DATA_SCHEMAS.md → game/src/inventory/Item.h
- ``world/`` --references--> `WorldGraph`  [INFERRED]
  ARCHITECTURE.md → game/src/world/WorldGraph.h

## Import Cycles
- None detected.

## Communities (186 total, 31 thin omitted)

### Community 0 - "BattleState Orchestration Shell"
Cohesion: 0.02
Nodes (90): BattleState, m_attackCenterTiles, m_attackResolution, m_battleDefinition, m_battleEndFading, m_battleEndPending, m_battleEndPendingWin, m_battleLeaveTutorialRetry (+82 more)

### Community 1 - "SaveManager.h"
Cohesion: 0.17
Nodes (10): baseClassFromInt(), battleFromJson(), formatSaveLabel(), obfuscate(), optIdFromJson(), optIdToJson(), promotionFromInt(), rosterSkillsFromJson() (+2 more)

### Community 2 - "DeploymentPhaseController.cpp"
Cohesion: 0.06
Nodes (17): BattleState, DeploymentPhaseController, m_deployment, m_deploymentPreviewUnits, m_deploymentWindow, m_facingChoiceInstanceId, m_isChoosingFacing, Unit (+9 more)

### Community 3 - "Gear Catalog Lookup"
Cohesion: 0.06
Nodes (13): GearCatalog, m_gears, Inventory, Inventory::Inventory(), m_capacity, m_stacks, m_usedCapacity, isValidItemDefinition() (+5 more)

### Community 4 - "World Map Scene"
Cohesion: 0.03
Nodes (52): ButtonMenuWindow, Renderer, WorldMapState, m_activePartyDetailInstanceId, m_cameraPos, m_cursorPos, m_cursorSpeed, m_graph (+44 more)

### Community 5 - "Battle Session Flow"
Cohesion: 0.12
Nodes (11): BattleSession, m_bossUnits, m_criticalUnits, m_defeatRule, m_queue, m_result, m_turnsElapsed, m_unitPtrs (+3 more)

### Community 6 - "Quest Data Loading"
Cohesion: 0.19
Nodes (4): QuestLog, m_active, m_catalog, m_completed

### Community 7 - "Runtime Unit State"
Cohesion: 0.04
Nodes (31): Unit, m_actions, m_attackGrowthAcc, m_bonuses, m_currentHp, m_currentMp, m_data, m_defenseGrowthAcc (+23 more)

### Community 8 - "vector"
Cohesion: 0.10
Nodes (9): unitAt(), SkillData, UIEvent, UIEvent, Grid, Renderer, GameTile, Font (+1 more)

### Community 9 - "UI Action Identifiers"
Cohesion: 0.04
Nodes (52): ActionId, Accept, AcceptMission, AdjustLeft, AdjustRight, Attack, Back, BuyItem (+44 more)

### Community 10 - "Battle Menu Controller"
Cohesion: 0.06
Nodes (18): BattleMenuController, m_actionMenuItems, m_inspectWindow, m_lastSkillIndex, m_lastSkillUnit, m_skillDetailVisible, m_skillMenuItems, m_systemMenuItems (+10 more)

### Community 11 - "Gear Subtype Hierarchy"
Cohesion: 0.06
Nodes (32): AccessorySubtype, Gloves, Necklace, Ring, Shoes, Amulet, Bow, Chest (+24 more)

### Community 12 - "CombatAnimationSystem"
Cohesion: 0.05
Nodes (28): CombatAnimationSystem, m_events, m_visuals, Event, anchored, animator, effectState, id (+20 more)

### Community 13 - "MenuContext"
Cohesion: 0.07
Nodes (31): BattleDefinition, BattleMap, AttackResolutionContext, BattleFlowPhase, Combat, Deployment, DeploymentContext, battleDefinition (+23 more)

### Community 14 - "ConfirmWindow"
Cohesion: 0.12
Nodes (9): ConfirmWindow, m_cancelButton, m_cancelLabelText, m_confirmButton, m_confirmLabelText, m_focus, m_font, m_prompt (+1 more)

### Community 15 - "UI Window Identifiers"
Cohesion: 0.05
Nodes (43): WindowId, BattleActionMenu, BattleDeployment, BattleDeploymentConfirm, BattleDialog, BattleInspect, BattleInspectMenu, BattleSkillMenu (+35 more)

### Community 16 - "Unit Detail Window"
Cohesion: 0.05
Nodes (34): UnitDetailWindow, m_actionFocus, m_actionRows, m_candidates, m_candidateScroll, m_catalog, m_className, m_exp (+26 more)

### Community 17 - "MovementAnimationController"
Cohesion: 0.10
Nodes (12): MovementAnimationController, m_heights, m_hopArcHeight, m_hopPauseFraction, m_hopSecondsPerTile, m_index, m_onComplete, m_path (+4 more)

### Community 18 - "Skill Progression UI"
Cohesion: 0.07
Nodes (22): availableProgressionSkills(), Font, Mode, Equip, Learn, PendingMode, LearnConfirm, None (+14 more)

### Community 19 - "ButtonMenuWindow"
Cohesion: 0.06
Nodes (22): ButtonMenuWindow, m_anchorBottomRight, m_bottomRightMargin, m_centerX, m_deleteAction, m_deleteHint, m_downRepeat, m_focus (+14 more)

### Community 21 - "algorithm"
Cohesion: 0.14
Nodes (5): BattleMap, Grid, Unit, ForcedUnitRule, RosterSystem

### Community 22 - "IntroState"
Cohesion: 0.10
Nodes (11): IntroState, IntroState::IntroState(), m_ending, m_renderer, m_scriptPath, m_shotIndex, m_shots, m_shotTime (+3 more)

### Community 23 - "PartyContext"
Cohesion: 0.08
Nodes (12): addDebugGearToCatalogAndInventory(), CampaignSave, PartyContext, m_battleControlsTutorialShown, m_controlsTutorialShown, m_gearCatalog, m_gold, m_initialized (+4 more)

### Community 24 - "Unit Panel Window"
Cohesion: 0.08
Nodes (16): Font, Unit, UnitPanelWindow, m_font, m_hasPreview, m_leftUnit, m_previewHp, m_previewLevel (+8 more)

### Community 25 - ".findPath"
Cohesion: 0.13
Nodes (8): Pathfinder, PathRequest, goal, start, PathRules, getNeighbors, moveCost, key

### Community 26 - "Skill Progression Catalog"
Cohesion: 0.13
Nodes (9): containsBaseClass(), findProgressionSkill(), SkillProgression, m_baseClass, m_equippedActive, m_equippedPassive, m_equippedReaction, m_levels (+1 more)

### Community 27 - "Renderer"
Cohesion: 0.06
Nodes (11): UnitInspectWindow::UnitInspectWindow(), main(), drawCircle(), drawRow(), formatButtonLabel(), m_emitEvent, Font, Renderer (+3 more)

### Community 28 - "Attack Resolution Controller"
Cohesion: 0.15
Nodes (7): AttackResolutionController, m_pendingAnyDied, m_pendingAttack, m_pendingHitIndex, m_pendingHitResults, m_pendingSkillId, BattleState

### Community 29 - "Recruitment and Promotion Rules"
Cohesion: 0.17
Nodes (10): canPromote(), eligibleBaseClasses(), isBaseClassEligible(), promotionOptions(), BaseClass, Archer, Duelist, Mage (+2 more)

### Community 30 - "Inventory Window"
Cohesion: 0.07
Nodes (19): Font, InventoryWindow, m_catalog, m_downRepeat, m_focus, m_focusRows, m_font, m_inventory (+11 more)

### Community 31 - "MainMenuState.cpp"
Cohesion: 0.08
Nodes (15): ButtonMenuWindow, Font, MainMenuState, m_fadeInOnEnter, m_mainMenuWindow, m_pendingDeleteSlotIndex, m_renderer, m_saveListSlotIndexes (+7 more)

### Community 32 - "Element and Affinity Enums"
Cohesion: 0.29
Nodes (7): Element, Dark, Fire, Holy, Ice, Lightning, Neutral

### Community 34 - "Turn Queue Scheduling"
Cohesion: 0.16
Nodes (15): normalise(), sortTimeline(), TurnEntry, timeUntilTurn, unit, TurnQueue, BASE_ACTION_COST, BASE_MOVE_AND_ACTION_COST (+7 more)

### Community 36 - "AudioSettings.cpp"
Cohesion: 0.13
Nodes (8): AudioSettings, m_musicSlider, m_renderer, m_showExitConfirm, m_uiManager, m_volumeSlider, Renderer, StateMachine

### Community 37 - "PendingAttackController"
Cohesion: 0.06
Nodes (24): PendingAttackController, m_center, m_focus, m_targets, m_tiles, computeHealAmount(), describeAttack(), describeDamage() (+16 more)

### Community 38 - "TavernCandidate"
Cohesion: 0.13
Nodes (12): 9) Tavern data (`assets/tavern/*.json`), `candidates.json` — recruitable units, `lore.json` — bar-room dialogue, TavernCandidate, cost, customName, flavor, templatePath (+4 more)

### Community 40 - "Unit Data Schema"
Cohesion: 0.05
Nodes (40): ElementAffinity, affinity, element, UnitData, acquisitionIndex, affinities, atkRange, attack (+32 more)

### Community 41 - "Skill Data Schema"
Cohesion: 0.08
Nodes (25): SkillData, area, basePower, buffTurns, castOncePerArea, description, effectTypes, element (+17 more)

### Community 42 - "GraphicsSettings.cpp"
Cohesion: 0.13
Nodes (7): GraphicsSettings, GraphicsSettings::GraphicsSettings(), m_renderer, m_showExitConfirm, m_uiManager, Renderer, StateMachine

### Community 43 - "Battle Dialogue Controller"
Cohesion: 0.10
Nodes (11): BattleState, DialogueBeat, speaker, speakerName, text, DialogueController, m_beats, m_onComplete (+3 more)

### Community 44 - "CombatSystem"
Cohesion: 0.17
Nodes (11): CombatSystem, AFFINITY_IMMUNE, AFFINITY_RESIST, AFFINITY_WEAK, CRIT_CHANCE, CRIT_MULTIPLIER, FRONT_DMG_MULTIPLIER, REAR_DMG_MULTIPLIER (+3 more)

### Community 45 - "Battle Grid"
Cohesion: 0.13
Nodes (14): Grid, m_height, m_tiles, m_width, Tile, height, moveCost, occupied (+6 more)

### Community 46 - "BattleLoadResult"
Cohesion: 0.17
Nodes (11): BattleLoadResult, battleMap, grid, mapData, mapOrigin, ok, spriteH, texH (+3 more)

### Community 47 - "Render Context Overlays"
Cohesion: 0.08
Nodes (28): BattleMap, BattleRendererContext, cursorHoverOffset, cursorTriH, cursorTriW, debugRenderer, facingChoiceSelection, facingChoiceUnit (+20 more)

### Community 48 - "UI Manager Window Stack"
Cohesion: 0.14
Nodes (3): UIManager, m_eventQueue, m_stack

### Community 49 - "Shop Window"
Cohesion: 0.09
Nodes (16): Font, GearCatalog, ShopWindow, m_buyRows, m_catalog, m_downRepeat, m_focus, m_focusRows (+8 more)

### Community 50 - "Game Constants and Settings"
Cohesion: 0.15
Nodes (4): SettingsManager, m_loaded, m_nativeResolutionIndex, m_settings

### Community 51 - "DeploymentWindow"
Cohesion: 0.12
Nodes (10): DeploymentWindow, DeploymentWindow::DeploymentWindow(), m_canStart, m_font, m_grabbedUnitLabel, m_hasGrabbedUnit, m_maxUnits, m_placed (+2 more)

### Community 53 - "SettingsState.cpp"
Cohesion: 0.13
Nodes (6): Renderer, SettingsState, m_renderer, m_uiManager, SettingsState::SettingsState(), StateMachine

### Community 54 - "EnemyAI.h"
Cohesion: 0.13
Nodes (15): `ai/`, Battle Flow Ownership, BattleMap, EnemyAI, EnemyAttackResult, attacked, result, target (+7 more)

### Community 55 - "Roster System"
Cohesion: 0.18
Nodes (4): baseClassOfTemplate(), RosterSystem, m_nextInstanceId, m_units

### Community 56 - "WorldGraph"
Cohesion: 0.15
Nodes (10): Edge, cost, to, Node, edges, id, position, WorldGraph (+2 more)

### Community 57 - ".chooseAction"
Cohesion: 0.14
Nodes (13): BattleSession, EnemyDecision, playersAliveBefore, target, EnemyTurnController, EnemyTurnOutcome, Continue, PlayerDefeat (+5 more)

### Community 58 - "Hit Context and Attack Side"
Cohesion: 0.10
Nodes (19): AttackSide, Front, Rear, Side, BattleMap, HitContext, attacker, basePower (+11 more)

### Community 59 - "Unit Inspect Window"
Cohesion: 0.09
Nodes (18): borderColorFor(), Font, Relation, Ally, Enemy, Player, Section, lines (+10 more)

### Community 60 - "StatusType"
Cohesion: 0.07
Nodes (24): ActiveStatus, damagePerTick, turnsRemaining, type, StatusInfo, abbr, breaksOnDamage, harmful (+16 more)

### Community 61 - "Unit Stat Accessors"
Cohesion: 0.13
Nodes (7): StatKind, Attack, Defense, Evasion, Magic, MagicDefense, Speed

### Community 62 - "ResultState.cpp"
Cohesion: 0.13
Nodes (6): Renderer, ResultState, m_playerWon, m_renderer, m_transition, ResultState::ResultState()

### Community 63 - "Unit Portrait Rendering"
Cohesion: 0.18
Nodes (11): PortraitStyle, mirrored, showPlacedMarker, team, transparent, renderText(), resolvePortraitPath(), teamFrameColor() (+3 more)

### Community 64 - "Battle Map Model"
Cohesion: 0.13
Nodes (7): BattleMap, allySpawns, enemySpawnsByTeam, m_cols, m_rows, m_tiles, playerSpawns

### Community 65 - "Unit Movement Points"
Cohesion: 0.12
Nodes (16): Gender, Female, Male, None, GenderData, bonusAtkRange, bonusAttack, bonusDefense (+8 more)

### Community 67 - "Settings Persistence Schema"
Cohesion: 0.11
Nodes (17): Resolution, height, width, Settings, appliedMasterVolume, appliedMusicVolume, appliedResolutionIndex, appliedWindowMode (+9 more)

### Community 68 - "UIWindow"
Cohesion: 0.11
Nodes (5): UIWindow, m_blocksInputBelow, m_blocksRenderBelow, m_id, m_visible

### Community 69 - "TRPG Game Architecture"
Cohesion: 0.25
Nodes (7): Current Gaps and Follow-Ups, Data Assets, Purpose, Scene Flow, TRPG Game Architecture, UI and Input Contract, POST_BUILD asset copy

### Community 70 - "BootState.cpp"
Cohesion: 0.15
Nodes (4): BootState, m_readyToTransition, m_renderer, Renderer

### Community 71 - "Promotion Class Roster"
Cohesion: 0.10
Nodes (18): PromotionClass, Assassin, BladeDancer, Elementalist, Enchanter, Fencer, Infiltrator, Knight (+10 more)

### Community 73 - "ShopWindow.h"
Cohesion: 0.17
Nodes (8): Renderer, UnitData, Renderer, Renderer, Renderer, Input, Renderer, RosterUnit

### Community 74 - ".processUIEvents"
Cohesion: 0.14
Nodes (5): 6) Generic Items and Inventory, Campaign ownership and equipment reservations, Inventory rules, Inventory serialization, Item definition (runtime)

### Community 75 - "Deployment System"
Cohesion: 0.06
Nodes (20): MovementRange, Result, costs, reachable, Vec2iHash, DeploymentEntry, critical, facing (+12 more)

### Community 76 - "Progression Skill Definitions"
Cohesion: 0.12
Nodes (15): ProgressionSkillCategory, Active, Passive, Reaction, ProgressionSkillDefinition, autoGranted, category, description (+7 more)

### Community 77 - "BattleDefinition"
Cohesion: 0.07
Nodes (31): BattleDefinition, backgroundBottom, backgroundTop, defaultZoom, defeatRule, enemies, events, forcedUnits (+23 more)

### Community 78 - "World Node Metadata"
Cohesion: 0.12
Nodes (15): NodeEvent, label, questId, startBattle, NodeMeta, battleMapPath, description, type (+7 more)

### Community 79 - "DialogWindow.cpp"
Cohesion: 0.09
Nodes (11): DialogWindow, DialogWindow::DialogWindow(), m_font, m_lineIndex, m_lines, m_timer, m_visibleChars, Font (+3 more)

### Community 80 - "PartyWindow"
Cohesion: 0.08
Nodes (14): Entry, hasSprite, name, templatePath, EntryFocusable, m_onActivate, m_selected, Font (+6 more)

### Community 82 - "SettingsRowWindow.cpp"
Cohesion: 0.15
Nodes (11): computeContentWidth(), computeRowLayout(), makeControlsColumn(), makeLabelColumn(), measureMaxControlsWidth(), measureMaxLabelWidth(), RowLayout, controlRect (+3 more)

### Community 83 - "Inspect Window Rendering"
Cohesion: 0.12
Nodes (16): QuestDefinition, brief, client, cost, description, id, isMain, objective (+8 more)

### Community 85 - "CombatAnimationSystem.cpp"
Cohesion: 0.21
Nodes (3): drawRing(), effectColor(), statusTypeFromString()

### Community 86 - "Grid Cursor"
Cohesion: 0.16
Nodes (6): Cursor, k_repeatDelay, k_repeatInterval, m_pos, m_repeatTimer, m_wasMoving

### Community 87 - "CampaignSave"
Cohesion: 0.16
Nodes (11): campaignFromJson(), CampaignSave, formatVersion, gold, inventoryJson, label, nextInstanceId, partyMemberIds (+3 more)

### Community 88 - "BattleEventSystem"
Cohesion: 0.11
Nodes (13): BattleEventSystem, m_callbacks, m_events, m_firedOnce, m_turn, m_variables, Callbacks, endBattle (+5 more)

### Community 89 - "EnemyAI.cpp"
Cohesion: 0.37
Nodes (3): bestTileToward(), evaluateTarget(), findBestTarget()

### Community 90 - "DamagePreview"
Cohesion: 0.15
Nodes (10): BattleMap, DamagePreview, m_damage, m_dealsDamage, m_hitChance, m_visible, Font, Renderer (+2 more)

### Community 91 - "PartySystem"
Cohesion: 0.22
Nodes (4): `systems/`, PartySystem, m_memberIds, RosterSystem

### Community 92 - "Race Growth Bonuses"
Cohesion: 0.14
Nodes (13): 3) Battle Definition Data (`config/BattleCatalog.*`), BattleDefeatRule, type, BattleVictoryRule, turnsToSurvive, type, DefeatConditionType, AllAlliesDead (+5 more)

### Community 93 - "Race System Design"
Cohesion: 0.07
Nodes (26): RaceGrowth, attackPerLevel, defensePerLevel, evasionPerLevel, hpPerLevel, magicDefPerLevel, magicPerLevel, mpPerLevel (+18 more)

### Community 94 - "UI Key Repeat Helper"
Cohesion: 0.19
Nodes (7): UIKeyRepeat, m_heldFor, m_initialDelay, m_initialFired, m_interval, m_sinceLastFire, m_wasDown

### Community 95 - ".commitMovement"
Cohesion: 0.18
Nodes (5): `battle/`, Combat and map primitives, Controllers, Core runtime state, MovementCommitSystem

### Community 97 - "BattleLoader.h"
Cohesion: 0.31
Nodes (3): BattleLoader, m_renderer, Renderer

### Community 98 - "Roster Unit Schema"
Cohesion: 0.17
Nodes (11): RosterUnit, baseClass, customName, equippedGear, exp, instanceId, level, recruited (+3 more)

### Community 99 - "UIManager Declarations"
Cohesion: 0.18
Nodes (7): Input, push(), Renderer, replaceTop(), UIWindow, Input, Renderer

### Community 100 - "Battle Module Ownership"
Cohesion: 0.18
Nodes (9): UnitSpawn, disappearOnDeath, isBoss, isCritical, level, rosterInstanceId, startPos, team (+1 more)

### Community 101 - "Module Responsibilities"
Cohesion: 0.13
Nodes (10): `config/`, `events/`, `inventory/`, Module Responsibilities, `renderer/`, `ui/`, `world/`, EquippedGearIds (+2 more)

### Community 102 - "Skill System Design"
Cohesion: 0.11
Nodes (17): 1. Aptitudes, 2. Nature, 3. Permanent Skill Point Investment, Active Skills, Combat, Levels, Passive Skills, Philosophy (+9 more)

### Community 103 - "Unit Stat Bonuses"
Cohesion: 0.18
Nodes (11): StatBonuses, atkRange, attack, defense, evasion, magic, magicDef, maxHp (+3 more)

### Community 104 - "Class Growth Curves"
Cohesion: 0.18
Nodes (11): ClassGrowth, attackPerLevel, baseClass, defensePerLevel, evasionPerLevel, hpPerLevel, magicDefPerLevel, magicPerLevel (+3 more)

### Community 105 - "Race Equip Slot Configuration"
Cohesion: 0.20
Nodes (10): Race, Elf, Elin, Human, Undead, makeDisabledConfig(), makeHumanConfig(), RaceEquipConfig (+2 more)

### Community 106 - "Gear Stat Modifiers"
Cohesion: 0.18
Nodes (11): GearStatModifiers, attack, defense, evasion, jump, magic, magicDefense, maxHp (+3 more)

### Community 107 - "Unit.h"
Cohesion: 0.15
Nodes (5): EquipmentLoadout, accessories, slots, GearCatalog, Inventory

### Community 108 - "Unit Detail Dependencies"
Cohesion: 0.18
Nodes (9): EquipmentLoadout, Font, GearCatalog, Input, Inventory, Renderer, RosterSystem, RosterUnit (+1 more)

### Community 109 - "PendingAttackController.h"
Cohesion: 0.20
Nodes (6): BattleMap, Grid, SkillData, Unit, BattleMap, Grid

### Community 110 - "Deployment Placement Logic"
Cohesion: 0.22
Nodes (7): 7) Gear and Equipment Rules, Data-driven slot rules, Gear modifiers and effects, Gear types, GearSpecialEffect, None, TeleportMovement

### Community 111 - "QuestLoader.cpp"
Cohesion: 0.15
Nodes (4): `data/`, parseObjectiveType(), QuestLoader, ShopStock

### Community 112 - "MovementCommitSystem.h"
Cohesion: 0.18
Nodes (8): BattleMap, BattleSession, Grid, MovementCommitResult, path, teleport, valid, Unit

### Community 113 - "SkillLoader.cpp"
Cohesion: 0.38
Nodes (3): parseEffectType(), parseElement(), SkillLoader

### Community 114 - "Floating Text System"
Cohesion: 0.20
Nodes (6): Entry, color, rise, text, tilePos, ttl

### Community 116 - "Shop Sell Row Schema"
Cohesion: 0.18
Nodes (7): SellRow, description, equippedCount, itemId, name, price, quantity

### Community 117 - "Equipment Slot Entries"
Cohesion: 0.22
Nodes (5): itemName(), SlotEntry, accessoryIndex, label, slot

### Community 119 - "ConfirmWindow"
Cohesion: 0.50
Nodes (4): pushConfirm(), pushDeleteSaveConfirm(), pushSettingsExitConfirm(), pushStartBattleConfirm()

### Community 120 - "Unit Lifecycle State"
Cohesion: 0.22
Nodes (7): UnitState, Attacking, Dead, Done, Idle, Moving, Selected

### Community 121 - "Battle Renderer Dependencies"
Cohesion: 0.22
Nodes (8): BattleMap, Camera, Cursor, DebugRenderer, Renderer, Texture, TileLayerData, Unit

### Community 122 - "Unit Render Proxy"
Cohesion: 0.25
Nodes (6): UnitRenderProxy, alive, debugLabel, pos, team, unit

### Community 123 - "Battle Tutorial Tips"
Cohesion: 0.22
Nodes (9): TutorialTip, ActionMenu, Attack, Defend, Deploy, Move, Skill, Wait (+1 more)

### Community 124 - "SaveManager"
Cohesion: 0.13
Nodes (11): deobfuscate(), readWholeFile(), SaveManager, kMaxSlots, m_indexLoaded, m_slots, Item, detail (+3 more)

### Community 125 - "Settings Row Item Schema"
Cohesion: 0.18
Nodes (7): Input, IRowControl, Renderer, RowItem, control, label, padding

### Community 126 - "UIEventType"
Cohesion: 0.22
Nodes (9): UIEventType, ActionCanceled, ActionChanged, ActionSelected, ConfirmResult, DialogAdvanced, DialogFinished, NavigateNext (+1 more)

### Community 129 - "TileMapData"
Cohesion: 0.29
Nodes (6): AnchorContext, scale, BattleMap, Camera, tileFeetAnchor(), TileMapData

### Community 130 - "Shot"
Cohesion: 0.18
Nodes (8): Line, speaker, text, Shot, bottom, duration, lines, top

### Community 131 - "Status Stat Effect Totals"
Cohesion: 0.25
Nodes (7): StatusStatEffect, ofTotal, pct, stat, StatusStatEffects, count, effects

### Community 132 - "Battle Action Types"
Cohesion: 0.14
Nodes (14): BattleActionType, EndBattle, GiveReward, ModifyVariable, PlayAnimation, ShowDialogue, SpawnUnit, StartCutscene (+6 more)

### Community 133 - "Battle Objectives and Enemies"
Cohesion: 0.29
Nodes (7): 4) Quest JSON (`assets/quests/*.json`), Example, Gating semantics, `objective` object, Optional fields (with defaults), Required fields, `reward` object

### Community 134 - "BattleEvent"
Cohesion: 0.15
Nodes (13): BattleEvent, actions, minTurn, once, trigger, BattleTriggerType, OnBattleStart, OnDefeat (+5 more)

### Community 135 - "SettingsRowWindow"
Cohesion: 0.22
Nodes (6): `settings/`, Font, SettingsRowWindow, m_focus, m_font, m_rows

### Community 136 - "Gear Slot Enum"
Cohesion: 0.15
Nodes (10): SlotConfig, capacity, enabled, slot, GearSlot, Accessory, Body, Head (+2 more)

### Community 137 - "Facing Dial Rendering"
Cohesion: 0.43
Nodes (4): BattleRenderer::BattleRenderer(), drawDot(), drawFacingDial(), facingScreenDir()

### Community 138 - "Confirm Window Rendering"
Cohesion: 0.29
Nodes (6): PendingHitResult, hitCtx, result, target, visualPlayed, Unit

### Community 141 - "TRPG Game Data Schemas"
Cohesion: 0.12
Nodes (16): 10) Town talk (`assets/town/talk.json`), 1) Unit JSON (`assets/units/*.json`), 2) Skill JSON (`assets/skills/*.json`), 5) Map Data (`assets/maps/*.tmj` + tilesets), 8) Shop data (`assets/shop/stock.json`), Example, Example, Objects used by battle setup (+8 more)

### Community 142 - "Combat Result Schema"
Cohesion: 0.14
Nodes (13): CombatResult, absorbed, affinityResult, crit, damage, hit, hitChance, Affinity (+5 more)

### Community 143 - "Game Tile Schema"
Cohesion: 0.29
Nodes (6): GameTile, classFlags, col, height, impassable, row

### Community 144 - "Major Action Budget"
Cohesion: 0.29
Nodes (5): MajorAction, Attack, Defend, None, Skill

### Community 145 - "Battle Menu Item"
Cohesion: 0.29
Nodes (5): BattleMenuItem, detail, enabled, label, onSelect

### Community 146 - "UI Event Struct"
Cohesion: 0.33
Nodes (6): QuestObjective, nodeId, type, QuestObjectiveType, ReachNode, VictoryAtNode

### Community 147 - "Item Candidate Entry"
Cohesion: 0.29
Nodes (5): Gear, ItemCandidate, gear, itemId, quantity

### Community 148 - "CMake Source Lists"
Cohesion: 0.38
Nodes (5): POST_BUILD runtime DLL copy (Windows), Duplicated DeploymentPhaseController registration, GAME_HEADERS list, GAME_SOURCES list, add_executable(game) target

### Community 149 - "BattleRequest"
Cohesion: 0.17
Nodes (6): `scenes/`, BattleRequest, mapPath, returnWorldNodeId, tutorial, BattleState::BattleState()

### Community 150 - "Unit Action Type Enum"
Cohesion: 0.33
Nodes (6): ActionType, Attack, Defend, Item, Move, Wait

### Community 151 - "SaveSlotInfo"
Cohesion: 0.20
Nodes (9): SaveKind, Battle, Campaign, SaveSlotInfo, kind, label, payload, slotIndex (+1 more)

### Community 152 - "Skill Effect Types"
Cohesion: 0.33
Nodes (6): SkillEffectType, Buff, Damage, Debuff, Heal, Ko

### Community 153 - "Tile Layer Reference"
Cohesion: 0.40
Nodes (4): BattleResult, Defeat, Ongoing, Victory

### Community 154 - "UIEvent"
Cohesion: 0.29
Nodes (6): UIEvent, actionId, confirmed, index, type, windowId

### Community 156 - "vcpkg Dependencies"
Cohesion: 0.33
Nodes (5): builtin-baseline, dependencies, name, $schema, version

### Community 157 - "Status Effect Application"
Cohesion: 0.40
Nodes (5): StatusEffectApply, chance, damagePerTick, turns, type

### Community 158 - "Status Tick Result"
Cohesion: 0.40
Nodes (5): SkillType, Foo_Elf, Foo_Elin, Foo_Human, Foo_Undead

### Community 160 - "Skill Visual Definition"
Cohesion: 0.40
Nodes (3): SkillVisual, duration, id

### Community 161 - "Battle Overlay Modes"
Cohesion: 0.40
Nodes (5): BattleOverlayMode, AttackRange, ConfirmTargets, MoveRange, None

### Community 162 - "BattleState.h"
Cohesion: 0.12
Nodes (12): AutoPlayPhase, ExecuteAction, Idle, MenuOpen, ShowMenu, BattleMenuItem, DebugRenderer, HitContext (+4 more)

### Community 165 - "Menu Text Alignment"
Cohesion: 0.40
Nodes (4): TextAlign, Center, Left, Right

### Community 166 - "Temporary Stat Modifier"
Cohesion: 0.50
Nodes (4): TemporaryStatMod, amount, stat, turnsRemaining

### Community 168 - "Menu Geometry Cache"
Cohesion: 0.50
Nodes (4): MenuGeometry, isScrollMenu, menuH, visibleCount

### Community 169 - "Shop Buy Row"
Cohesion: 0.50
Nodes (3): BuyRow, gear, price

### Community 170 - "Shop Window Mode"
Cohesion: 0.50
Nodes (3): Mode, Buy, Sell

### Community 171 - "Unit Detail UI State"
Cohesion: 0.50
Nodes (4): UIState, Details, ItemSelect, SlotSelect

### Community 172 - "Engine Package Discovery"
Cohesion: 0.67
Nodes (3): TRPG_USE_INSTALLED_ENGINE (DEV vs FINAL engine selection), find_package(TRPGEngine) Discovery, TRPG::engine / SDL3::SDL3 link

### Community 179 - "StatusTickResult"
Cohesion: 0.40
Nodes (5): StatusTickResult, dotDamage, dotType, skippedTurn, skipType

### Community 180 - "PendingResolution"
Cohesion: 0.40
Nodes (5): PendingResolution, EnemyAction, None, PlayerConfirmedAttack, ResolvingHits

### Community 181 - "SegmentTiming"
Cohesion: 0.50
Nodes (4): SegmentTiming, inPause, moveT, t

## Knowledge Gaps
- **1389 isolated node(s):** `Grid`, `TurnQueue`, `BattleMap`, `attacked`, `result` (+1384 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 1841 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **31 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `BattleState` connect `BattleState Orchestration Shell` to `TileMapData`, `DeploymentPhaseController.cpp`, `Battle Menu Controller`, `CombatAnimationSystem`, `MenuContext`, `MovementAnimationController`, `BattleState.cpp`, `BattleRequest`, `IntroState`, `PartyContext`, `Unit Panel Window`, `Attack Resolution Controller`, `BattleState.h`, `World Projection Anchors`, `AudioSettings.cpp`, `PendingAttackController`, `Battle Renderer Draw Pass`, `Battle Dialogue Controller`, `PendingResolution`, `ResultState.cpp`, `BootState.cpp`, `Deployment System`, `.hitContextFor`, `Grid Cursor`, `BattleEventSystem`, `DamagePreview`, `Battle Walk and Dialogue`, `PendingAttackController.h`, `Battle Tutorial Tips`?**
  _High betweenness centrality (0.086) - this node is a cross-community bridge._
- **Are the 2 inferred relationships involving `WorldMapState` (e.g. with `Scene Flow` and ``scenes/``) actually correct?**
  _`WorldMapState` has 2 INFERRED edges - model-reasoned connections that need verification._
- **What connects `Grid`, `TurnQueue`, `BattleMap` to the rest of the system?**
  _1389 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `BattleState Orchestration Shell` be split into smaller, more focused modules?**
  _Cohesion score 0.02127659574468085 - nodes in this community are weakly interconnected._
- **Why does `Unit` connect `Runtime Unit State` to `Major Action Budget`, `Unit Action Type Enum`, `Skill Progression Catalog`, `Recruitment and Promotion Rules`, `Status Tick Result`, `Temporary Stat Modifier`, `Unit Data Schema`, `StatusTickResult`, `StatusType`, `Unit Stat Accessors`, `Unit Movement Points`, `Unit.cpp`, `Promotion Class Roster`, `Unit Stat Bonuses`, `Race Equip Slot Configuration`, `Unit.h`, `Deployment Placement Logic`, `Unit Lifecycle State`, `Scene Flow and World Graph`?**
  _High betweenness centrality (0.077) - this node is a cross-community bridge._
- **Are the 2 inferred relationships involving `UnitDetailWindow` (e.g. with ``ui/`` and `Data-driven slot rules`) actually correct?**
  _`UnitDetailWindow` has 2 INFERRED edges - model-reasoned connections that need verification._
- **Should `DeploymentPhaseController.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.06292966684294024 - nodes in this community are weakly interconnected._