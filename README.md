# game_1 — tactical TRPG game layer

Gameplay layer for the shared `engine`. Scene flow: boot → main menu → intro
(tutorial duel) → world map → deployment → combat → result → world map. All
gameplay logic lives here; the engine is consumed through its public headers
only and is never modified from this repo.

- **Agent rules** (build, hard architecture rules, style, verification):
  [`AGENTS.md`](AGENTS.md)
- **Module ownership / render pipeline**: [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
- **Engine public surface cheat-sheet**: [`docs/ENGINE_API.md`](docs/ENGINE_API.md)

## Build & run

[`AGENTS.md`](AGENTS.md) §8 is the authoritative build recipe. Short version
(DEV — debug game + engine package from `engine/build`):

```bat
:: only needed after engine source changes
cd /d H:\Coding\Games\TRPG\engine
tools\build_debug.bat

cd /d H:\Coding\Games\TRPG\game_1
tools\build_debug.bat

:: run
build\debug\game\Debug\game.exe
```

FINAL (release game + engine installed into `..\install`): run
`engine\tools\build_release.bat`, then
`tools\vsenv.bat cmake --install build --config Release` in the engine, then
`game_1\tools\build_release.bat`. VS Code tasks live in `.vscode/tasks.json`.

Saves: the game writes `user.dat` in its working directory; `tools\clear_saves.bat`
removes every copy (see `AGENTS.md` §9.1).

## Documentation index

| Doc | What it is |
| --- | --- |
| [`AGENTS.md`](AGENTS.md) | Agent-facing rules: engine boundary, module ownership, battle/turn flow, style, build + verification |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Module responsibilities, scene/battle flow, UI contract, render pipeline + coordinate spaces |
| [`docs/ROADMAP.md`](docs/ROADMAP.md) | Mission, current vertical-slice status, remaining work by category |
| [`docs/TODO.md`](docs/TODO.md) | Live bugs, verification items, QA backlog |
| [`docs/CHANGELOG.md`](docs/CHANGELOG.md) | Dated milestone log — what shipped, when |
| [`docs/DATA_SCHEMAS.md`](docs/DATA_SCHEMAS.md) | JSON + runtime struct contracts (units, skills, maps, items, saves) |
| [`docs/TUNING.md`](docs/TUNING.md) | "Where do I change X" map for camera/UI/layout tunables |
| [`docs/ENGINE_API.md`](docs/ENGINE_API.md) | Engine public surface: what to call, what to reuse, iso formula |
| [`docs/design/UNIT_PROGRESSION.md`](docs/design/UNIT_PROGRESSION.md) | Progression design (M4/M5 slices shipped; promotions/passives deferred) |
| [`docs/design/RACES.md`](docs/design/RACES.md) | Race/class eligibility (design pitch; the roster is partly aspirational) |
| [`docs/design/FRAMED_LAYOUT.md`](docs/design/FRAMED_LAYOUT.md) | Framed battle layout + rails (M1 shipped, M2–M5 planned) |
| [`docs/archive/MILESTONES.md`](docs/archive/MILESTONES.md) | Shipped milestone records moved out of `TODO.md` |
| [`docs/archive/RESOLVED_ISSUES.md`](docs/archive/RESOLVED_ISSUES.md) | Root-cause write-ups of fixed bugs |

Generated, never hand-edited: `graphify-out/` (code knowledge graph).

### Where did the old files go? (2026-10-19 docs reorg)

| Old (repo root) | Now |
| --- | --- |
| `ARCHITECTURE.md` | `docs/ARCHITECTURE.md` |
| `PLAN.md` | `docs/ROADMAP.md` (duplicated sections dropped) |
| `DATA_SCHEMAS.md` | `docs/DATA_SCHEMAS.md` |
| `TODO.md` | `docs/TODO.md` + `docs/archive/MILESTONES.md` |
| `CHANGELOG.md` | `docs/CHANGELOG.md` |
| `TUNING.md` | `docs/TUNING.md` |
| `UNIT_DESIGN.md` | `docs/design/UNIT_PROGRESSION.md` |
| `RACE_DESIGN.md` | `docs/design/RACES.md` |
| `FRAMED_LAYOUT_PLAN.md` | `docs/design/FRAMED_LAYOUT.md` |
| `ISSUE.md` | `docs/archive/RESOLVED_ISSUES.md` |
| `USEFUL_COMMANDS.txt` | deleted — the iso formula lives in `docs/ENGINE_API.md` §3 |
| `AGENTS.md` §14 (engine API) | `docs/ENGINE_API.md` |
| `build_out.txt`, `latest_debug_run.txt` | deleted (transient build/log captures) |
