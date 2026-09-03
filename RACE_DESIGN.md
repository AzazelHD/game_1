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

## Initial race/class table (placeholder — expand later)

| Race  | Eligible Base Classes |
| ----- | --------------------- |
| Human | Soldier, Archer, Mage |
| Elf   | Archer, Mage, Scout   |
| Elin  | Archer, Mage, Duelist |

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
- Elin Archer/Mage specialization names are borrowed placeholders (see above)
  and may get unique names later.
- Other races beyond Human/Elf/Elin are not yet planned.
