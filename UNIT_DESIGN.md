# Unit Progression System – Design Document

## Philosophy

The player controls an army of recruitable units, not fixed protagonists.

- Each unit belongs to **one class for life** and cannot freely change class.
- Identity is built through aptitudes, nature, specialisation, and permanent skill point investment.
- It should be rewarding to train multiple units of the same class: two identical‑on‑paper units can fill completely different roles.

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

Nature suggests a build without forcing the player to follow it.

### 3. Permanent Skill Point Investment

Points are **never** re-assignable. If the player wants a different variant of the same class, they must train another unit.

---

## Classes

- Each unit is locked to a single base class.
- Each class has:
  - Its own set of **active skills**
  - Several **passive skills**
  - Several **reaction skills**
- Only that class can learn those skills.

### Promotion

Late in the game, a unit may promote to one of several specialisations. Promotion is permanent and irreversible.

Effects:

- Alters stat growth
- Adapts some base‑class skills
- Adds exclusive specialisation skills (obtained automatically, no point cost, no levels)
- Increases max level from 40 → 50
- Increases equipped active skills limit

Promotion specialises the role, it does not completely change it.

---

## Levels

| Phase      | Max Level |
| ---------- | --------- |
| Base class | 40        |
| Promoted   | 50        |

Each level grants **exactly 1 skill point**.  
Total max over a playthrough: **50 points**.

---

## Skill System

Skills are divided into three categories.

### Active Skills

Used in combat. Each class has its own set.  
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

|                    | Active skills (equipped) | Passive | Reaction |
| ------------------ | ------------------------ | ------- | -------- |
| **Base class**     | 5                        | 1       | 1        |
| **Promoted class** | 6                        | 1       | 1        |

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
4. Learning new skills
5. Class promotion
6. Obtaining/crafting equipment from enemy materials
7. Recruiting new units with different aptitudes and natures

---

## Unit Identity

Two units of the same class can end up vastly different thanks to:

- Aptitudes
- Nature
- Chosen specialisation
- Skills learned and their levels
- Equipped passive and reaction
- Equipment

Because skill points are permanent and irreversible, every unit develops a unique identity over the campaign. To try a different build of the same class, the player must train a new unit – no respecs.
