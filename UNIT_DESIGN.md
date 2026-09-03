# Unit Progression System – Design Document

## Philosophy

The player controls an army of recruitable units, not fixed protagonists.

- There is **no class/job system**. A unit is not locked into a role by a label.
- Identity is built through race, aptitudes, nature, and permanent skill point investment.
- It should be rewarding to train multiple units of the same race: two identical‑on‑paper units can fill completely different roles.

---

## Recruitment

Every unit has three layers of identity:

### 1. Aptitudes

Small random variations in base stats (like Pokémon IVs). Differences are tiny – any recruit is perfectly viable – but sufficient to reward optimisers.

### 2. Nature

A visible nature at birth that slightly modifies stat growth with a small bonus and a small malus.

Examples:

- **Firm** (+Strength, –Speed)
- **Serene** (+Magic Resist, –Strength)
- **Stoic** (+Defense, –MP)
- **Cunning** (+Dexterity, –Resist)

Nature suggests a build without forcing the player to follow it. Nature does **not** restrict which skills a unit can learn — it only affects how well those skills perform, since skills key off stats the nature modifies.

### 3. Permanent Skill Point Investment

Points are **never** re-assignable. If the player wants a different variant of the same race, they must train another unit.

---

## Race

- Each unit belongs to a **race**, chosen at recruitment. Race is permanent.
- Race determines:
  - Base stats and stat growth curves
  - Equipment slot access (see Gear doc)
  - Possibly a small race-flavored bonus/malus, similar in scope to Nature
- Race does **not** restrict which skills a unit can learn. All units draw from a single shared skill pool (see Skill System below).

### Promotion

Late in the game, a unit may promote. Promotion is permanent and irreversible.

Effects:

- Alters stat growth
- Unlocks a **tier-2 skill pool** (higher-tier active/passive/reaction skills become learnable)
- Increases max level from 40 → 50
- Increases equipped active skills limit

Promotion deepens what the unit can become, it does not assign a new role or lock the unit into anything.

---

## Levels

| Phase             | Max Level |
| ----------------- | --------- |
| Base (unpromoted) | 40        |
| Promoted          | 50        |

Each level grants **exactly 1 skill point**.
Total max over a playthrough: **50 points**.

---

## Skill System

All units learn from the **same shared skill pool** — there is no per-race or per-class restriction on which skills are eligible. What a unit ends up good at is a function of stats (aptitude + nature + equipment), not eligibility gates.

Skills are divided into three categories.

### Active Skills

Used in combat.
Active skills have **5 levels**. A skill is unavailable until Level 1 is learned. Each level permanently improves the skill – not just damage, but carefully balanced refinements.

| Level | Point Cost |
| ----- | ---------- |
| 1     | 1          |
| 2     | 1          |
| 3     | 2          |
| 4     | 2          |
| 5     | 3          |

**Total to max one active skill:** 9 points.

Improvements may include:

- Damage increase
- Accuracy increase
- Critical hit chance
- MP cost reduction
- Ignoring part of defense
- Higher chance to apply a status
- Longer effect duration
- Any skill‑specific refinement

Improvements refine the skill without radically altering its function (no large range jumps, no area‑of‑effect changes).

### Passive Skills

Always active in battle when equipped. Also levelable, but progression is shorter.

| Level | Point Cost |
| ----- | ---------- |
| 1     | 1          |
| 2     | 1          |
| 3     | 2          |

**Total to max one passive:** 4 points.

Passive improvements strengthen the effect without changing its nature.

### Reaction Skills

Trigger automatically on a specific condition (e.g., counter‑attack, dodge, reflect magic).

- **No levels.**
- Learning a reaction costs **1 point**. Once learned, it remains available forever.

---

## Skill Equipping

Skills are permanently learned. They can be freely swapped outside combat.

|                       | Active skills (equipped) | Passive | Reaction |
| --------------------- | ------------------------ | ------- | -------- |
| **Base (unpromoted)** | 5                        | 1       | 1        |
| **Promoted**          | 6                        | 1       | 1        |

A unit’s identity is determined not just by what it has learned, but by the exact combination equipped for each mission.

---

## Combat

- Skills consume MP (Mana Points).
- Movement and actions can be freely combined within the turn rules.
- MP cost is independent of movement.

---

## Player Progression Layers

Progression is built on several simultaneous layers:

1. Leveling up (1–50)
2. Earning skill points
3. Permanently improving skills
4. Learning new skills from the shared pool
5. Promotion
6. Obtaining/crafting equipment from enemy materials
7. Recruiting new units with different races, aptitudes, and natures

---

## Unit Identity

Two units of the same race can end up vastly different thanks to:

- Aptitudes
- Nature
- Skills learned and their levels (drawn from the same shared pool, but different choices)
- Equipped passive and reaction
- Equipment

Because skill points are permanent and irreversible, every unit develops a unique identity over the campaign. To try a different build, the player must train a new unit – no respecs.
