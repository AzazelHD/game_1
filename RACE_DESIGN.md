# Race System – Companion to UNIT_DESIGN.md

Race does **not** change the skill system, promotion system, or point economy defined
in UNIT_DESIGN.md. It only gates **which base classes are eligible at recruitment**,
and may bias starting stats/growth slightly (comparable in scope to Nature).

## How it works

- Each race has a fixed list of base classes it can recruit as.
- A unit's race is permanent, chosen at recruitment (or fixed per recruit encounter).
- Everything after recruitment (aptitudes, nature, skill points, promotion path)
  works exactly as described in UNIT_DESIGN.md, scoped to whatever base class
  the unit rolled.
- Each race has a **stat growth curve** (per-level gains) and a **base stat
  bonus** (starting stat profile). See the tables in § Race Roster & Stat Growth.

## Race Roster & Stat Growth

The playable roster is **6 races**. Names are placeholders and easy to rename.
`Elf` and `Undead` are **not** playable race options:

- `Undead` is reclassified as a future **monster unit** type, not a recruit race.
- `Elf` is dropped from the roster entirely (it was only ever an example and its
  "fast magic skirmisher" identity is covered by Elin).

Archetypes are mapped from the FFTA/FFTA2 family to give each race a clear,
non-overlapping identity:

| Placeholder name | Archetype inspiration | Role / identity |
| ---------------- | --------------------- | --------------- |
| **Human**        | Hume (FFTA)           | Well-rounded all-rounder; jack of all trades. |
| **Elin**         | Viera (FFTA) / Tera Elin | Swift finesse glass-cannon skirmisher; high speed/evasion, low bulk. |
| **Kragan**       | Bangaa (FFTA)         | Reptilian frontline brute/tank; high HP/Atk/Def, slow, magic-weak. |
| **Fennari**      | Nu Mou (FFTA)         | Fox/beast-eared arcane scholar; pure caster, high Magic/MDef/MP, frail & slow. |
| **Tok**          | Moogle (FFTA)         | Small sturdy gear/utility folk; high Def/MDef, decent speed, low Atk/HP. |
| **Gria**         | Gria (FFTA2)          | Dragon-winged flyer that **ignores height and flies over blockers** on its movement path; fast, mobile physical skirmisher. |

> **Seeq** (FFTA2's pig-like race) is intentionally **not** in the roster for
> now: its physical-brute and fast-skirmisher niches are already covered by
> Kragan and Elin. Revisit later if a distinct mid-tier bruiser or
> trapper/zone-control role is wanted.

### Base stat bonuses (starting profile)

Additive flat bonuses applied to a recruit's level-1 base stats, on top of the
unit's individual aptitude. `0` means the race does not change that base stat.

| Stat       | Human | Elin | Kragan | Fennari | Tok | Gria |
| ---------- | :---: | :--: | :----: | :-----: | :-: | :-: |
| Max HP     |   0   |  -4  |  +12   |   -6    |  0  | -6  |
| Max MP     |   0   |  +4  |   -3   |   +8    | +2  | -2  |
| Attack     |   0   |  +1  |   +7   |   -3    | -2  | +3  |
| Defense    |   0   |  -2  |   +6   |   -3    | +5  | -2  |
| Magic      |   0   |  +3  |   -4   |   +8    | +2  | -3  |
| Mag. Def.  |   0   |   0  |   +4   |   +6    | +6  | -1  |
| Evasion    |   0   |  +8  |   -4   |    0    | +2  | +6  |
| Speed      |   0   | +12  |   -6   |   -4    |  0  | +10 |
| Move range |   0   |   0  |    0   |    0    |  0  | +1  |
| Atk range  |   0   |   0  |    0   |    0    |  0  |  0  |

### Per-level stat growth (race curve)

The **six** main stats (HP, Max MP, Attack, Defense, Magic, Mag. Def.) grow
**fractionally each level** by the per-level values below, on top of base
stats. **Speed is special**: it does not grow every level — instead each race
has a **percentage chance of +1 Speed per level** (FFTA-style; see § Speed
below). **Evasion** also grows fractionally each level alongside the six.

Base class and promotion add smaller typed offsets on top (see § Class Growth
below). Higher = a stat the race leans into; lower = a soft weakness.

| Stat      | Human | Elin | Kragan | Fennari | Tok | Gria |
| --------- | :---: | :--: | :----: | :-----: | :-: | :-: |
| Max HP    | 2.0   | 1.4  |  2.4   |  1.5    | 1.7 | 1.5 |
| Max MP    | 0.8   | 0.9  |  0.5   |  1.2    | 0.7 | 0.5 |
| Attack    | 0.7   | 0.6  |  0.9   |  0.4    | 0.5 | 0.8 |
| Defense   | 0.6   | 0.4  |  0.9   |  0.3    | 0.8 | 0.5 |
| Magic     | 0.6   | 0.8  |  0.4   |  1.0    | 0.7 | 0.4 |
| Mag. Def. | 0.6   | 0.6  |  0.8   |  0.9    | 0.9 | 0.5 |
| Evasion   | 0.1   | 0.3  |  0.05  |  0.1    | 0.15| 0.3 |

### Speed (percentage chance per level)

Speed is the one stat that does **not** grow every level. Instead each race
(and class) has a **percentage chance that Speed increases by 1 on a given
level up** — a 65% Speed means roughly a 2-in-3 chance of +1 Speed each level.
This makes Speed the scarcest, most build-critical stat, matching the FFTA
model where fast classes run ~90-99% and slow casters ~35-50%.

| Race   | Speed %/level |
| ------ | :-----------: |
| Elin   |      80%      |
| Gria   |      65%      |
| Human  |      58%      |
| Tok    |      55%      |
| Kragan |      42%      |
| Fennari|      40%      |

> These numbers mirror the project's existing balance scale
> (`UnitProgression.cpp` per-level floats). Tweak with the balance pass.
>
> **Code note:** the current `RaceGrowth` uses a fractional `speedPerLevel`
> float like the other stats. To match the percentage model, speed should be
> represented as a percentage chance (e.g. `speedGrowthPercent` in 0-100)
> rather than a per-level float; the other six stats stay fractional.

### Flight (Gria)

Gria are the one race that **flies**. Flight changes how their movement path is
evaluated in `battle/map`:

- **Ignores height cost**: terrain that costs extra movement for its height
  step costs them the flat moving cost instead.
- **Flies over blockers**: they can pass through enemy/ally units and low
  obstacles (short walls, gaps) that normally block or cost extra movement.
  They cannot end a turn on an occupied/blocked tile.
- **No landing penalty**: unlike units with a `jump` stat, flying across a gap
  does not consume jump budget or require a landing tile of equal height.

This is a **movement-model** trait, not a stat. Implement it as a per-unit/race
flag consulted by the pathfinder + movement range system; do not bake it into
per-level growth.

> Note: the promotion-tree sections below were written against the old
> Human/Elf/Elin roster. With Elf removed and Kragan/Fennari/Tok/Gria added they
> are **superseded and pending rework**. Classes are not yet final, so only the
> race roster and the class-eligibility table above are authoritative for now.

## Initial race/class table (placeholder — expand later)

| Race     | Eligible Base Classes |
| -------- | --------------------- |
| Human    | Soldier, Archer, Mage |
| Kragan   | Soldier, Duelist      |
| Fennari  | Mage, Scout           |
| Elin     | Archer, Mage, Duelist |
| Tok      | Soldier, Archer, Scout|
| Gria     | Soldier, Archer, Duelist |

## Human – Soldier promotion tree (placeholder)

Base class: **Soldier**

Promotes into one of 3 specializations (pick one, permanent):

- **Templar** — bias toward MP/magic-resist survivability
- **Knight** — bias toward defense/HP tanking
- **Paladin** — bias toward hybrid physical + support

Per UNIT_DESIGN.md promotion rules:

- Alters stat growth
- Adapts some base-class (Soldier) skills
- Adds exclusive specialization skills (no point cost, no levels, auto-granted)
- Base Soldier skills remain learnable/equippable after promotion — promotion
  adds on top, it does not remove access.
- Level cap 40 → 50, equipped actives 5 → 6

Specialization skills: for now, placeholder names only, all functionally
identical (same effect/values) until we balance them individually. Copilot
should generate 2-3 placeholder exclusive skills per specialization.

---

## Elf

Stat bias: higher magic and speed, higher evasion; lower defense/HP than Human.
Nimble spellcaster/archer archetype.

Eligible base classes: **Archer, Mage, Scout**

### Elf – Archer promotion tree (placeholder)

Base class: **Archer**

Promotes into one of 3 specializations (pick one, permanent):

- **Sharpshooter** — bias toward accuracy/critical hit at range
- **Ranger** — bias toward hybrid ranged + mobility/utility
- **Windrunner** — bias toward speed/evasion, hit-and-run playstyle

### Elf – Mage promotion tree (placeholder)

Base class: **Mage**

Promotes into one of 3 specializations (pick one, permanent):

- **Elementalist** — bias toward raw elemental damage output
- **Enchanter** — bias toward buffs/support magic
- **Warden** — bias toward defensive/control magic

### Elf – Scout promotion tree (placeholder)

Base class: **Scout**

Promotes into one of 3 specializations (pick one, permanent):

- **Infiltrator** — bias toward evasion/positioning, single-target burst
- **Trapper** — bias toward zone control/debuffs
- **Pathfinder** — bias toward movement range/map utility

Promotion effects (all Elf trees) follow the same rules as Human/Soldier:
alters stat growth, adds exclusive auto-granted skills, base-class skills
remain usable, level cap 40 → 50, equipped actives 5 → 6. Specialization
skills are placeholder names only for now, same as Human.

---

## Elin

Viera-inspired agile/finesse race. Stat bias: high speed and evasion, decent
magic, notably lower defense/HP — a glass-cannon skirmisher archetype.

Eligible base classes: **Archer, Mage, Duelist**

Archer and Mage trees for Elin follow the same 3-specialization placeholder
pattern as Elf (reuse the Elf Archer/Mage specialization names for now unless
we want Elin-specific flavor later — flag this as a placeholder decision).

### Elin – Duelist promotion tree (placeholder)

Base class: **Duelist** (fast melee finesse fighter, dexterity/evasion-based
rather than raw strength)

Promotes into one of 3 specializations (pick one, permanent):

- **Blade Dancer** — bias toward multi-hit/combo playstyle
- **Assassin** — bias toward single-target burst/critical damage
- **Fencer** — bias toward counter-attacks/reactive play

Same promotion rules as other trees: alters stat growth, adds exclusive
auto-granted skills, base-class skills remain usable, level cap 40 → 50,
equipped actives 5 → 6. Placeholder skill names only for now.

---

## Not yet defined / deferred

- **Undead is NOT a playable recruit race.** Reclassify it conceptually as a
  future **monster unit** type (zombies, hounds, flans, etc., in the FFTA
  sense) — fixed movesets, likely no gear slots or a very different slot
  model, no player-driven skill point investment. This needs its own design
  pass and is explicitly out of scope until requested. Do not build monster
  units, monster skill trees, or monster equip rules yet.
- **Elf is dropped** from the playable roster. It was only ever an example; its
  fast, evasive magic-skirmisher identity is covered by Elin.
- The final names for **Kragan, Fennari, and Tok** are placeholders pending a
  naming pass.
- The base-class eligibility table above and the race growth tables are the
  current authoritative data. Promotion trees for the new roster are not yet
  designed (classes are not final).
