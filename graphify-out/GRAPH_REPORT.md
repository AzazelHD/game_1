# Graph Report - game_1  (2026-10-05)

## Corpus Check
- 181 files · ~91,349 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 5 file(s) not represented in the graph (top: .bat 3, (none) 2)

## Summary
- 3420 nodes · 6500 edges · 179 communities (148 shown, 31 thin omitted)
- Extraction: 92% EXTRACTED · 8% INFERRED · 0% AMBIGUOUS · INFERRED: 499 edges (avg confidence: 0.86)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `37829d16`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- BattleState Orchestration Shell
- Campaign Save Serialization
- Deployment Phase Controller
- Gear Catalog Lookup
- World Map Scene
- Battle Session Flow
- Quest Data Loading
- Runtime Unit State
- Battle Participant Construction
- UI Action Identifiers
- Battle Menu Controller
- Gear Subtype Hierarchy
- Combat Animation Queue
- Battle Context Factories
- Enemy AI Target Selection
- UI Window Identifiers
- Unit Detail Window
- Movement Animation
- Skill Progression UI
- Button Menu Window
- Battle Turn Methods
- Range Computation Utilities
- Intro State Scene
- Quest and Tavern Integration
- Unit Panel Window
- Pathfinding
- Skill Progression Catalog
- Progression and Slot Config
- Attack Resolution Controller
- Recruitment and Promotion Rules
- Inventory Window
- Main Menu Scene
- Element and Affinity Enums
- World Map Rendering
- Turn Queue Scheduling
- World Projection Anchors
- Audio Settings Scene
- Pending Attack Targeting
- Shop and Tavern Stock Data
- Battle Renderer Draw Pass
- Unit Data Schema
- Skill Data Schema
- Graphics Settings Scene
- Battle Dialogue Controller
- Damage Formula Engine
- Battle Grid
- Battle Map Loading
- Render Context Overlays
- UI Manager Window Stack
- Shop Window
- Game Constants and Settings
- Deployment Window
- Menu Geometry and Events
- Settings Scene
- Battle Type Includes
- Roster System
- World Graph and Pathfinding
- Enemy Turn Controller
- Hit Context and Attack Side
- Unit Inspect Window
- Status Effect Visuals
- Unit Stat Accessors
- Result Scene
- Unit Portrait Rendering
- Battle Map Model
- Unit Movement Points
- Settings Persistence Schema
- UI Window Base Class
- Architecture Overview
- Boot State Scene
- Promotion Class Roster
- Equipment Eligibility Rules
- Town Hub Interactions
- Combat VFX Library
- Deployment System
- Progression Skill Definitions
- Battle Definition Schema
- World Node Metadata
- Dialog Window
- Party Window
- Settings Row Layout
- Inspect Window Rendering
- Hit Context Construction
- Skill Effect Descriptions
- Grid Cursor
- Confirm Window
- Battle Event Dispatcher
- Economy and Shop Design
- Damage Preview Overlay
- Attack and Movement Ranges
- Race Growth Bonuses
- Race System Design
- UI Key Repeat Helper
- Party Window Rendering
- Settings Row Window
- Roster Unit Schema
- UIManager Declarations
- Battle Module Ownership
- Campaign Ownership and Save
- Skill System Design
- Unit Stat Bonuses
- Class Growth Curves
- Race Equip Slot Configuration
- Gear Stat Modifiers
- Dialog Window Rendering
- Unit Detail Dependencies
- Human Turn Controller
- Deployment Placement Logic
- Deployment Entry Schema
- Battle Event Action Schema
- Skill JSON Parser
- Floating Text System
- Shop Window Logic
- Shop Sell Row Schema
- Equipment Slot Entries
- Confirm Dialog Helpers
- Unit Lifecycle State
- Battle Renderer Dependencies
- Unit Render Proxy
- Battle Tutorial Tips
- Save Slot Menu Rows
- Settings Row Item Schema
- UI Event Types
- Scene Flow and World Graph
- Participant Builder Interface
- Deployment Tile Placement
- Status Info Metadata
- Status Stat Effect Totals
- Battle Action Types
- Battle Objectives and Enemies
- Battle Trigger Types
- Battle Event Callbacks
- Gear Slot Enum
- Facing Dial Rendering
- Confirm Window Rendering
- Party Entry Focusable
- Shop Window Input
- Combat and Map Design
- Combat Result Schema
- Game Tile Schema
- Major Action Budget
- Battle Menu Item
- UI Event Struct
- Item Candidate Entry
- CMake Source Lists
- Forced Deployment Units
- Unit Action Type Enum
- Battle Event Action Payload
- Skill Effect Types
- Tile Layer Reference
- Damage Preview Dependencies
- Unit Detail Construction
- vcpkg Dependencies
- Status Effect Application
- Status Tick Result
- Skill Visual Definition
- Battle Overlay Modes
- Battle Autoplay Phases
- Pending Resolution Phases
- Settings Row Geometry
- Menu Text Alignment
- Temporary Stat Modifier
- Menu Geometry Cache
- Shop Buy Row
- Shop Window Mode
- Unit Detail UI State
- Engine Package Discovery
- Range Rule Conversion
- Inventory Window Construction
- Settings Module Ownership

## God Nodes (most connected - your core abstractions)
1. `BattleState` - 189 edges
2. `Unit` - 118 edges
3. `WorldMapState` - 117 edges
4. `WindowId` - 93 edges
5. `UnitDetailWindow` - 76 edges
6. `ActionId` - 67 edges
7. `PartyContext` - 66 edges
8. `ButtonMenuWindow` - 59 edges
9. `ShopWindow` - 44 edges
10. `DeploymentSystem` - 42 edges

## Surprising Connections (you probably didn't know these)
- ``ai/`` --references--> `EnemyAI`  [INFERRED]
  ARCHITECTURE.md → game/src/ai/EnemyAI.h
- `Combat and map primitives` --references--> `CombatSystem`  [INFERRED]
  ARCHITECTURE.md → game/src/battle/combat/CombatSystem.h
- `Controllers` --references--> `MovementAnimationController`  [INFERRED]
  ARCHITECTURE.md → game/src/battle/controllers/MovementAnimationController.h
- ``events/`` --references--> `BattleEventSystem`  [INFERRED]
  ARCHITECTURE.md → game/src/events/BattleEventSystem.h
- `Item definition (runtime)` --references--> `ItemDefinition`  [INFERRED]
  DATA_SCHEMAS.md → game/src/inventory/Item.h

## Import Cycles
- None detected.

## Communities (179 total, 31 thin omitted)

### Community 0 - "BattleState Orchestration Shell"
Cohesion: 0.02
Nodes (90): BattleState, m_attackCenterTiles, m_attackResolution, m_battleDefinition, m_battleEndFading, m_battleEndPending, m_battleEndPendingWin, m_battleLeaveTutorialRetry (+82 more)

### Community 1 - "Campaign Save Serialization"
Cohesion: 0.06
Nodes (40): `systems/`, baseClassFromInt(), battleFromJson(), campaignFromJson(), CampaignSave, formatVersion, gold, inventoryJson (+32 more)

### Community 2 - "Deployment Phase Controller"
Cohesion: 0.06
Nodes (17): `data/`, BattleState, DeploymentPhaseController, m_deployment, m_deploymentPreviewUnits, m_deploymentWindow, m_facingChoiceInstanceId, m_isChoosingFacing (+9 more)

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

### Community 8 - "Battle Participant Construction"
Cohesion: 0.08
Nodes (12): unitAt(), SkillData, UIEvent, Grid, Renderer, UnitData, GameTile, UnitData (+4 more)

### Community 9 - "UI Action Identifiers"
Cohesion: 0.04
Nodes (52): ActionId, Accept, AcceptMission, AdjustLeft, AdjustRight, Attack, Back, BuyItem (+44 more)

### Community 10 - "Battle Menu Controller"
Cohesion: 0.06
Nodes (18): BattleMenuController, m_actionMenuItems, m_inspectWindow, m_lastSkillIndex, m_lastSkillUnit, m_skillDetailVisible, m_skillMenuItems, m_systemMenuItems (+10 more)

### Community 11 - "Gear Subtype Hierarchy"
Cohesion: 0.06
Nodes (32): AccessorySubtype, Gloves, Necklace, Ring, Shoes, Amulet, Bow, Chest (+24 more)

### Community 12 - "Combat Animation Queue"
Cohesion: 0.09
Nodes (15): CombatAnimationSystem, m_events, m_visuals, drawRing(), effectColor(), SpriteMotion, FadeInHoldRise, Ring (+7 more)

### Community 13 - "Battle Context Factories"
Cohesion: 0.06
Nodes (38): BattleDefinition, BattleMap, AttackResolutionContext, BattleFlowPhase, Combat, Deployment, BattleRequest, mapPath (+30 more)

### Community 15 - "UI Window Identifiers"
Cohesion: 0.05
Nodes (43): WindowId, BattleActionMenu, BattleDeployment, BattleDeploymentConfirm, BattleDialog, BattleInspect, BattleInspectMenu, BattleSkillMenu (+35 more)

### Community 16 - "Unit Detail Window"
Cohesion: 0.05
Nodes (34): UnitDetailWindow, m_actionFocus, m_actionRows, m_candidates, m_candidateScroll, m_catalog, m_className, m_exp (+26 more)

### Community 17 - "Movement Animation"
Cohesion: 0.08
Nodes (16): MovementAnimationController, m_heights, m_hopArcHeight, m_hopPauseFraction, m_hopSecondsPerTile, m_index, m_onComplete, m_path (+8 more)

### Community 18 - "Skill Progression UI"
Cohesion: 0.07
Nodes (22): availableProgressionSkills(), Font, Mode, Equip, Learn, PendingMode, LearnConfirm, None (+14 more)

### Community 19 - "Button Menu Window"
Cohesion: 0.06
Nodes (22): ButtonMenuWindow, m_anchorBottomRight, m_bottomRightMargin, m_centerX, m_deleteAction, m_deleteHint, m_downRepeat, m_focus (+14 more)

### Community 21 - "Range Computation Utilities"
Cohesion: 0.13
Nodes (5): BattleMap, Grid, Unit, ForcedUnitRule, RosterSystem

### Community 22 - "Intro State Scene"
Cohesion: 0.07
Nodes (19): IntroState, IntroState::IntroState(), m_ending, m_renderer, m_scriptPath, m_shotIndex, m_shots, m_shotTime (+11 more)

### Community 23 - "Quest and Tavern Integration"
Cohesion: 0.07
Nodes (17): 6) Generic Items and Inventory, Campaign ownership and equipment reservations, Inventory rules, Inventory serialization, Item definition (runtime), addDebugGearToCatalogAndInventory(), CampaignSave, PartyContext (+9 more)

### Community 24 - "Unit Panel Window"
Cohesion: 0.08
Nodes (16): Font, Unit, UnitPanelWindow, m_font, m_hasPreview, m_leftUnit, m_previewHp, m_previewLevel (+8 more)

### Community 25 - "Pathfinding"
Cohesion: 0.06
Nodes (17): Pathfinder, PathRequest, goal, start, PathRules, getNeighbors, moveCost, BattleMap (+9 more)

### Community 26 - "Skill Progression Catalog"
Cohesion: 0.13
Nodes (9): containsBaseClass(), findProgressionSkill(), SkillProgression, m_baseClass, m_equippedActive, m_equippedPassive, m_equippedReaction, m_levels (+1 more)

### Community 27 - "Progression and Slot Config"
Cohesion: 0.08
Nodes (8): UnitInspectWindow::UnitInspectWindow(), drawCircle(), drawRow(), formatButtonLabel(), Font, Renderer, Unit, PartyWindow::PartyWindow()

### Community 28 - "Attack Resolution Controller"
Cohesion: 0.15
Nodes (7): AttackResolutionController, m_pendingAnyDied, m_pendingAttack, m_pendingHitIndex, m_pendingHitResults, m_pendingSkillId, BattleState

### Community 29 - "Recruitment and Promotion Rules"
Cohesion: 0.17
Nodes (10): canPromote(), eligibleBaseClasses(), isBaseClassEligible(), promotionOptions(), BaseClass, Archer, Duelist, Mage (+2 more)

### Community 30 - "Inventory Window"
Cohesion: 0.07
Nodes (19): Font, InventoryWindow, m_catalog, m_downRepeat, m_focus, m_focusRows, m_font, m_inventory (+11 more)

### Community 31 - "Main Menu Scene"
Cohesion: 0.09
Nodes (15): ButtonMenuWindow, Font, MainMenuState, m_fadeInOnEnter, m_mainMenuWindow, m_pendingDeleteSlotIndex, m_renderer, m_saveListSlotIndexes (+7 more)

### Community 32 - "Element and Affinity Enums"
Cohesion: 0.29
Nodes (7): Element, Dark, Fire, Holy, Ice, Lightning, Neutral

### Community 34 - "Turn Queue Scheduling"
Cohesion: 0.16
Nodes (15): normalise(), sortTimeline(), TurnEntry, timeUntilTurn, unit, TurnQueue, BASE_ACTION_COST, BASE_MOVE_AND_ACTION_COST (+7 more)

### Community 36 - "Audio Settings Scene"
Cohesion: 0.13
Nodes (9): AudioSettings, AudioSettings::AudioSettings(), m_musicSlider, m_renderer, m_showExitConfirm, m_uiManager, m_volumeSlider, Renderer (+1 more)

### Community 37 - "Pending Attack Targeting"
Cohesion: 0.12
Nodes (5): PendingAttackController, m_center, m_focus, m_targets, m_tiles

### Community 38 - "Shop and Tavern Stock Data"
Cohesion: 0.12
Nodes (12): 9) Tavern data (`assets/tavern/*.json`), `candidates.json` — recruitable units, `lore.json` — bar-room dialogue, TavernCandidate, cost, customName, flavor, templatePath (+4 more)

### Community 40 - "Unit Data Schema"
Cohesion: 0.05
Nodes (40): ElementAffinity, affinity, element, UnitData, acquisitionIndex, affinities, atkRange, attack (+32 more)

### Community 41 - "Skill Data Schema"
Cohesion: 0.08
Nodes (25): SkillData, area, basePower, buffTurns, castOncePerArea, description, effectTypes, element (+17 more)

### Community 42 - "Graphics Settings Scene"
Cohesion: 0.13
Nodes (7): GraphicsSettings, GraphicsSettings::GraphicsSettings(), m_renderer, m_showExitConfirm, m_uiManager, Renderer, StateMachine

### Community 43 - "Battle Dialogue Controller"
Cohesion: 0.10
Nodes (11): BattleState, DialogueBeat, speaker, speakerName, text, DialogueController, m_beats, m_onComplete (+3 more)

### Community 44 - "Damage Formula Engine"
Cohesion: 0.19
Nodes (11): CombatSystem, AFFINITY_IMMUNE, AFFINITY_RESIST, AFFINITY_WEAK, CRIT_CHANCE, CRIT_MULTIPLIER, FRONT_DMG_MULTIPLIER, REAR_DMG_MULTIPLIER (+3 more)

### Community 45 - "Battle Grid"
Cohesion: 0.13
Nodes (14): Grid, m_height, m_tiles, m_width, Tile, height, moveCost, occupied (+6 more)

### Community 46 - "Battle Map Loading"
Cohesion: 0.15
Nodes (12): TileMapData, BattleLoadResult, battleMap, grid, mapData, mapOrigin, ok, spriteH (+4 more)

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

### Community 51 - "Deployment Window"
Cohesion: 0.11
Nodes (11): UIEvent, DeploymentWindow, DeploymentWindow::DeploymentWindow(), m_canStart, m_font, m_grabbedUnitLabel, m_hasGrabbedUnit, m_maxUnits (+3 more)

### Community 53 - "Settings Scene"
Cohesion: 0.12
Nodes (6): Renderer, SettingsState, m_renderer, m_uiManager, SettingsState::SettingsState(), StateMachine

### Community 54 - "Battle Type Includes"
Cohesion: 0.14
Nodes (12): BattleMap, EnemyAttackResult, attacked, result, target, EnemyTurnPlan, destination, target (+4 more)

### Community 55 - "Roster System"
Cohesion: 0.18
Nodes (4): baseClassOfTemplate(), RosterSystem, m_nextInstanceId, m_units

### Community 56 - "World Graph and Pathfinding"
Cohesion: 0.19
Nodes (9): Edge, cost, to, Node, edges, id, position, WorldGraph (+1 more)

### Community 57 - "Enemy Turn Controller"
Cohesion: 0.12
Nodes (13): BattleSession, EnemyDecision, playersAliveBefore, target, EnemyTurnController, EnemyTurnOutcome, Continue, PlayerDefeat (+5 more)

### Community 58 - "Hit Context and Attack Side"
Cohesion: 0.10
Nodes (19): AttackSide, Front, Rear, Side, BattleMap, HitContext, attacker, basePower (+11 more)

### Community 59 - "Unit Inspect Window"
Cohesion: 0.09
Nodes (18): borderColorFor(), Font, Relation, Ally, Enemy, Player, Section, lines (+10 more)

### Community 60 - "Status Effect Visuals"
Cohesion: 0.08
Nodes (21): ActiveStatus, damagePerTick, turnsRemaining, type, StatusTickVisual, floatColor, ringColor, StatusType (+13 more)

### Community 61 - "Unit Stat Accessors"
Cohesion: 0.13
Nodes (7): StatKind, Attack, Defense, Evasion, Magic, MagicDefense, Speed

### Community 62 - "Result Scene"
Cohesion: 0.10
Nodes (8): main(), Renderer, Renderer, ResultState, m_playerWon, m_renderer, m_transition, ResultState::ResultState()

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

### Community 68 - "UI Window Base Class"
Cohesion: 0.09
Nodes (7): UIWindow, m_blocksInputBelow, m_blocksRenderBelow, m_emitEvent, m_id, m_visible, ConfirmWindow::ConfirmWindow()

### Community 69 - "Architecture Overview"
Cohesion: 0.22
Nodes (8): Battle Flow Ownership, Current Gaps and Follow-Ups, Data Assets, Purpose, Scene Flow, TRPG Game Architecture, UI and Input Contract, POST_BUILD asset copy

### Community 70 - "Boot State Scene"
Cohesion: 0.18
Nodes (3): BootState, m_readyToTransition, m_renderer

### Community 71 - "Promotion Class Roster"
Cohesion: 0.10
Nodes (18): PromotionClass, Assassin, BladeDancer, Elementalist, Enchanter, Fencer, Infiltrator, Knight (+10 more)

### Community 73 - "Town Hub Interactions"
Cohesion: 0.19
Nodes (6): Renderer, Renderer, Renderer, Input, Renderer, RosterUnit

### Community 74 - "Combat VFX Library"
Cohesion: 0.27
Nodes (4): CombatVfxLibrary, m_textures, Renderer, Texture

### Community 75 - "Deployment System"
Cohesion: 0.06
Nodes (20): MovementRange, Result, costs, reachable, Vec2iHash, DeploymentEntry, critical, facing (+12 more)

### Community 76 - "Progression Skill Definitions"
Cohesion: 0.12
Nodes (15): ProgressionSkillCategory, Active, Passive, Reaction, ProgressionSkillDefinition, autoGranted, category, description (+7 more)

### Community 77 - "Battle Definition Schema"
Cohesion: 0.07
Nodes (27): BattleDefinition, backgroundBottom, backgroundTop, defaultZoom, defeatRule, enemies, events, forcedUnits (+19 more)

### Community 78 - "World Node Metadata"
Cohesion: 0.12
Nodes (15): NodeEvent, label, questId, startBattle, NodeMeta, battleMapPath, description, type (+7 more)

### Community 79 - "Dialog Window"
Cohesion: 0.10
Nodes (11): DialogWindow, DialogWindow::DialogWindow(), m_font, m_lineIndex, m_lines, m_timer, m_visibleChars, Font (+3 more)

### Community 80 - "Party Window"
Cohesion: 0.07
Nodes (14): Entry, hasSprite, name, templatePath, EntryFocusable, m_onActivate, m_selected, Font (+6 more)

### Community 82 - "Settings Row Layout"
Cohesion: 0.05
Nodes (26): `settings/`, computeContentWidth(), computeRowLayout(), Font, makeControlsColumn(), makeLabelColumn(), measureMaxControlsWidth(), measureMaxLabelWidth() (+18 more)

### Community 83 - "Inspect Window Rendering"
Cohesion: 0.12
Nodes (16): QuestDefinition, brief, client, cost, description, id, isMain, objective (+8 more)

### Community 85 - "Skill Effect Descriptions"
Cohesion: 0.33
Nodes (12): computeHealAmount(), describeAttack(), describeDamage(), describeSkill(), describeSkillDetail(), describeSkillRiders(), describeStatMod(), hasEffect() (+4 more)

### Community 86 - "Grid Cursor"
Cohesion: 0.16
Nodes (6): Cursor, k_repeatDelay, k_repeatInterval, m_pos, m_repeatTimer, m_wasMoving

### Community 87 - "Confirm Window"
Cohesion: 0.12
Nodes (10): Event, anchored, animator, effectState, id, tilePos, ttl, Renderer (+2 more)

### Community 88 - "Battle Event Dispatcher"
Cohesion: 0.17
Nodes (6): BattleEventSystem, m_callbacks, m_events, m_firedOnce, m_turn, m_variables

### Community 89 - "Economy and Shop Design"
Cohesion: 0.36
Nodes (4): bestTileToward(), EnemyAI, evaluateTarget(), findBestTarget()

### Community 90 - "Damage Preview Overlay"
Cohesion: 0.12
Nodes (11): BattleMap, DamagePreview, m_damage, m_dealsDamage, m_hitChance, m_visible, Font, gaugeColor() (+3 more)

### Community 92 - "Race Growth Bonuses"
Cohesion: 0.14
Nodes (13): 3) Battle Definition Data (`config/BattleCatalog.*`), BattleDefeatRule, type, BattleVictoryRule, turnsToSurvive, type, DefeatConditionType, AllAlliesDead (+5 more)

### Community 93 - "Race System Design"
Cohesion: 0.07
Nodes (26): RaceGrowth, attackPerLevel, defensePerLevel, evasionPerLevel, hpPerLevel, magicDefPerLevel, magicPerLevel, mpPerLevel (+18 more)

### Community 94 - "UI Key Repeat Helper"
Cohesion: 0.19
Nodes (7): UIKeyRepeat, m_heldFor, m_initialDelay, m_initialFired, m_interval, m_sinceLastFire, m_wasDown

### Community 95 - "Party Window Rendering"
Cohesion: 0.17
Nodes (12): `ai/`, `battle/`, Combat and map primitives, `config/`, Controllers, Core runtime state, `events/`, Module Responsibilities (+4 more)

### Community 97 - "Settings Row Window"
Cohesion: 0.24
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

### Community 101 - "Campaign Ownership and Save"
Cohesion: 0.22
Nodes (4): `inventory/`, EquippedGearIds, accessories, slots

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

### Community 107 - "Dialog Window Rendering"
Cohesion: 0.24
Nodes (3): EquipmentLoadout, accessories, slots

### Community 108 - "Unit Detail Dependencies"
Cohesion: 0.18
Nodes (9): EquipmentLoadout, Font, GearCatalog, Input, Inventory, Renderer, RosterSystem, RosterUnit (+1 more)

### Community 109 - "Human Turn Controller"
Cohesion: 0.25
Nodes (3): BattleState, HumanTurnController, Input

### Community 110 - "Deployment Placement Logic"
Cohesion: 0.22
Nodes (7): 7) Gear and Equipment Rules, Data-driven slot rules, Gear modifiers and effects, Gear types, GearSpecialEffect, None, TeleportMovement

### Community 112 - "Battle Event Action Schema"
Cohesion: 0.20
Nodes (9): BattleEvent, actions, minTurn, once, trigger, BattleRewards, gold, items (+1 more)

### Community 114 - "Floating Text System"
Cohesion: 0.20
Nodes (6): Entry, color, rise, text, tilePos, ttl

### Community 116 - "Shop Sell Row Schema"
Cohesion: 0.18
Nodes (7): SellRow, description, equippedCount, itemId, name, price, quantity

### Community 117 - "Equipment Slot Entries"
Cohesion: 0.22
Nodes (5): itemName(), SlotEntry, accessoryIndex, label, slot

### Community 119 - "Confirm Dialog Helpers"
Cohesion: 0.42
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

### Community 124 - "Save Slot Menu Rows"
Cohesion: 0.22
Nodes (5): Item, detail, enabled, id, label

### Community 125 - "Settings Row Item Schema"
Cohesion: 0.18
Nodes (7): Input, IRowControl, Renderer, RowItem, control, label, padding

### Community 126 - "UI Event Types"
Cohesion: 0.13
Nodes (15): UIEvent, actionId, confirmed, index, type, windowId, UIEventType, ActionCanceled (+7 more)

### Community 129 - "Deployment Tile Placement"
Cohesion: 0.32
Nodes (5): AnchorContext, scale, BattleMap, Camera, tileFeetAnchor()

### Community 130 - "Status Info Metadata"
Cohesion: 0.25
Nodes (8): StatusInfo, abbr, breaksOnDamage, harmful, label, skipsTurn, stacks, type

### Community 131 - "Status Stat Effect Totals"
Cohesion: 0.25
Nodes (7): StatusStatEffect, ofTotal, pct, stat, StatusStatEffects, count, effects

### Community 132 - "Battle Action Types"
Cohesion: 0.14
Nodes (14): BattleActionType, EndBattle, GiveReward, ModifyVariable, PlayAnimation, ShowDialogue, SpawnUnit, StartCutscene (+6 more)

### Community 133 - "Battle Objectives and Enemies"
Cohesion: 0.29
Nodes (7): 4) Quest JSON (`assets/quests/*.json`), Example, Gating semantics, `objective` object, Optional fields (with defaults), Required fields, `reward` object

### Community 134 - "Battle Trigger Types"
Cohesion: 0.25
Nodes (8): BattleTriggerType, OnBattleStart, OnDefeat, OnTileEnter, OnTurnEnd, OnTurnStart, OnUnitDeath, OnVictory

### Community 135 - "Battle Event Callbacks"
Cohesion: 0.25
Nodes (7): Callbacks, endBattle, giveRewardXp, playAnimation, showDialogue, spawnUnitByTemplate, startCutscene

### Community 136 - "Gear Slot Enum"
Cohesion: 0.15
Nodes (10): SlotConfig, capacity, enabled, slot, GearSlot, Accessory, Body, Head (+2 more)

### Community 137 - "Facing Dial Rendering"
Cohesion: 0.43
Nodes (4): BattleRenderer::BattleRenderer(), drawDot(), drawFacingDial(), facingScreenDir()

### Community 138 - "Confirm Window Rendering"
Cohesion: 0.29
Nodes (6): PendingHitResult, hitCtx, result, target, visualPlayed, Unit

### Community 141 - "Combat and Map Design"
Cohesion: 0.15
Nodes (13): 10) Town talk (`assets/town/talk.json`), 1) Unit JSON (`assets/units/*.json`), 2) Skill JSON (`assets/skills/*.json`), 8) Shop data (`assets/shop/stock.json`), Example, Example, Optional fields (with defaults), Optional fields (with defaults) (+5 more)

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

### Community 150 - "Unit Action Type Enum"
Cohesion: 0.33
Nodes (6): ActionType, Attack, Defend, Item, Move, Wait

### Community 151 - "Battle Event Action Payload"
Cohesion: 0.40
Nodes (5): 5) Map Data (`assets/maps/*.tmj` + tilesets), Objects used by battle setup, Required map-level fields (Tiled), Supported layer kinds, Tileset support

### Community 152 - "Skill Effect Types"
Cohesion: 0.33
Nodes (6): SkillEffectType, Buff, Damage, Debuff, Heal, Ko

### Community 153 - "Tile Layer Reference"
Cohesion: 0.40
Nodes (4): BattleResult, Defeat, Ongoing, Victory

### Community 154 - "Damage Preview Dependencies"
Cohesion: 0.40
Nodes (5): BuiltinVisual, asset, id, motion, scale

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

### Community 162 - "Battle Autoplay Phases"
Cohesion: 0.08
Nodes (17): BattleMap, Grid, SkillData, Unit, BattleMap, Grid, AutoPlayPhase, ExecuteAction (+9 more)

### Community 163 - "Pending Resolution Phases"
Cohesion: 0.67
Nodes (3): PlayerControlMode, AI, Human

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

## Knowledge Gaps
- **1391 isolated node(s):** `Grid`, `TurnQueue`, `BattleMap`, `attacked`, `result` (+1386 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 1843 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **31 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `BattleState` connect `BattleState Orchestration Shell` to `Deployment Phase Controller`, `Battle Menu Controller`, `Combat Animation Queue`, `Battle Context Factories`, `Enemy AI Target Selection`, `Movement Animation`, `Battle Turn Methods`, `Forced Deployment Units`, `Intro State Scene`, `Quest and Tavern Integration`, `Unit Panel Window`, `Attack Resolution Controller`, `Battle Autoplay Phases`, `World Projection Anchors`, `Pending Resolution Phases`, `Battle Renderer Draw Pass`, `Battle Dialogue Controller`, `Battle Map Loading`, `Settings Scene`, `Result Scene`, `Combat VFX Library`, `Deployment System`, `Hit Context Construction`, `Grid Cursor`, `Battle Event Dispatcher`, `Damage Preview Overlay`, `Battle Walk and Dialogue`, `Human Turn Controller`, `Battle Tutorial Tips`?**
  _High betweenness centrality (0.085) - this node is a cross-community bridge._
- **Are the 2 inferred relationships involving `WorldMapState` (e.g. with `Scene Flow` and ``scenes/``) actually correct?**
  _`WorldMapState` has 2 INFERRED edges - model-reasoned connections that need verification._
- **What connects `Grid`, `TurnQueue`, `BattleMap` to the rest of the system?**
  _1391 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `BattleState Orchestration Shell` be split into smaller, more focused modules?**
  _Cohesion score 0.02127659574468085 - nodes in this community are weakly interconnected._
- **Why does `Unit` connect `Runtime Unit State` to `Unit Movement Points`, `Unit Equipment and Experience`, `Temporary Stat Modifier`, `Promotion Class Roster`, `Unit Stat Bonuses`, `Race Equip Slot Configuration`, `Unit Data Schema`, `Deployment Placement Logic`, `Major Action Budget`, `Recruitment and Promotion Rules`, `Range Computation Utilities`, `Unit Action Type Enum`, `Unit Lifecycle State`, `Skill Progression Catalog`, `Status Effect Visuals`, `Unit Stat Accessors`, `Status Tick Result`, `Scene Flow and World Graph`?**
  _High betweenness centrality (0.077) - this node is a cross-community bridge._
- **Should `Campaign Save Serialization` be split into smaller, more focused modules?**
  _Cohesion score 0.057902973395931145 - nodes in this community are weakly interconnected._
- **Why does `WindowId` connect `UI Window Identifiers` to `Deployment Phase Controller`, `Battle Participant Construction`, `Battle Menu Controller`, `Unit Panel Window`, `Progression and Slot Config`, `Unit Detail Construction`, `Main Menu Scene`, `Audio Settings Scene`, `Graphics Settings Scene`, `Inventory Window Construction`, `UI Manager Window Stack`, `Deployment Window`, `Menu Geometry and Events`, `Settings Scene`, `UI Window Base Class`, `Town Hub Interactions`, `Dialog Window`, `Settings Row Layout`, `UIManager Declarations`, `Shop Window Logic`, `Confirm Dialog Helpers`, `UI Event Types`?**
  _High betweenness centrality (0.068) - this node is a cross-community bridge._