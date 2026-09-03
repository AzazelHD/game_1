#pragma once

#include "inventory/Item.h"

#include <utility>

enum class GearSlot
{
    Weapon,
    Offhand,
    Head,
    Body,
    Accessory,
};

enum class WeaponHandedness
{
    OneHanded,
    TwoHanded,
};

enum class AccessorySubtype
{
    Ring,
    Necklace,
    Gloves,
    Shoes,
};

// Future effects belong in this tag enum. Do not add item-ID-specific behavior
// in movement/combat code; systems query equipped Gear through this value.
enum class GearSpecialEffect
{
    None,
    TeleportMovement,
};

// Additive modifiers use the same stat names/types as UnitData. All current
// values default to zero until gear content is authored and balanced.
struct GearStatModifiers
{
    int maxHp = 0;
    int maxMp = 0;
    int attack = 0;
    int defense = 0;
    int magic = 0;
    int magicDefense = 0;
    int evasion = 0;
    int speed = 0;
    int moveRange = 0;
    int jump = 0;
};

class Gear : public ItemDefinition
{
public:
    Gear(ItemDefinition definition,
         GearSlot slot,
         bool weapon = false,
         WeaponHandedness handedness = WeaponHandedness::OneHanded,
         bool ranged = false,
         AccessorySubtype accessorySubtype = AccessorySubtype::Ring,
         GearStatModifiers statModifiers = {},
         GearSpecialEffect specialEffect = GearSpecialEffect::None)
        : ItemDefinition(std::move(definition)),
          m_slot(slot),
          m_isWeapon(weapon),
          m_weaponHandedness(handedness),
          m_isRanged(ranged),
          m_accessorySubtype(accessorySubtype),
          m_statModifiers(statModifiers),
          m_specialEffect(specialEffect)
    {
        stackable = false;
        maxStackSize = 1;
    }

    virtual ~Gear() = default;

    GearSlot slot() const { return m_slot; }
    bool isWeapon() const { return m_isWeapon; }
    WeaponHandedness weaponHandedness() const { return m_weaponHandedness; }
    bool isRanged() const { return m_isRanged; }
    AccessorySubtype accessorySubtype() const { return m_accessorySubtype; }
    const GearStatModifiers &statModifiers() const { return m_statModifiers; }
    GearSpecialEffect specialEffect() const { return m_specialEffect; }

private:
    GearSlot m_slot;
    bool m_isWeapon = false;
    WeaponHandedness m_weaponHandedness = WeaponHandedness::OneHanded;
    bool m_isRanged = false;
    AccessorySubtype m_accessorySubtype = AccessorySubtype::Ring;
    GearStatModifiers m_statModifiers{};
    GearSpecialEffect m_specialEffect = GearSpecialEffect::None;
};

class Sword final : public Gear
{
public:
    explicit Sword(ItemDefinition definition,
                   GearStatModifiers statModifiers = {},
                   GearSpecialEffect specialEffect = GearSpecialEffect::None)
        : Gear(std::move(definition), GearSlot::Weapon, true, WeaponHandedness::OneHanded,
               false, AccessorySubtype::Ring, statModifiers, specialEffect)
    {
    }
};

class Mace final : public Gear
{
public:
    explicit Mace(ItemDefinition definition,
                  GearStatModifiers statModifiers = {},
                  GearSpecialEffect specialEffect = GearSpecialEffect::None)
        : Gear(std::move(definition), GearSlot::Weapon, true, WeaponHandedness::OneHanded,
               false, AccessorySubtype::Ring, statModifiers, specialEffect)
    {
    }
};

class Bow final : public Gear
{
public:
    // Future reconsideration: a later gear redesign may make bows one-handed
    // and add a Quiver/ammo slot. No quiver system is implemented currently.
    explicit Bow(ItemDefinition definition,
                 GearStatModifiers statModifiers = {},
                 GearSpecialEffect specialEffect = GearSpecialEffect::None)
        : Gear(std::move(definition), GearSlot::Weapon, true, WeaponHandedness::TwoHanded,
               true, AccessorySubtype::Ring, statModifiers, specialEffect)
    {
    }
};

class Shield final : public Gear
{
public:
    explicit Shield(ItemDefinition definition,
                    GearStatModifiers statModifiers = {},
                    GearSpecialEffect specialEffect = GearSpecialEffect::None)
        : Gear(std::move(definition), GearSlot::Offhand, false, WeaponHandedness::OneHanded,
               false, AccessorySubtype::Ring, statModifiers, specialEffect)
    {
    }
};

class Helmet final : public Gear
{
public:
    explicit Helmet(ItemDefinition definition,
                    GearStatModifiers statModifiers = {},
                    GearSpecialEffect specialEffect = GearSpecialEffect::None)
        : Gear(std::move(definition), GearSlot::Head, false, WeaponHandedness::OneHanded,
               false, AccessorySubtype::Ring, statModifiers, specialEffect)
    {
    }
};

class Chest final : public Gear
{
public:
    explicit Chest(ItemDefinition definition,
                   GearStatModifiers statModifiers = {},
                   GearSpecialEffect specialEffect = GearSpecialEffect::None)
        : Gear(std::move(definition), GearSlot::Body, false, WeaponHandedness::OneHanded,
               false, AccessorySubtype::Ring, statModifiers, specialEffect)
    {
    }
};

class Gloves final : public Gear
{
public:
    explicit Gloves(ItemDefinition definition,
                    GearStatModifiers statModifiers = {},
                    GearSpecialEffect specialEffect = GearSpecialEffect::None)
        : Gear(std::move(definition), GearSlot::Accessory, false, WeaponHandedness::OneHanded,
               false, AccessorySubtype::Gloves, statModifiers, specialEffect)
    {
    }
};

class Shoes final : public Gear
{
public:
    explicit Shoes(ItemDefinition definition,
                   GearStatModifiers statModifiers = {},
                   GearSpecialEffect specialEffect = GearSpecialEffect::None)
        : Gear(std::move(definition), GearSlot::Accessory, false, WeaponHandedness::OneHanded,
               false, AccessorySubtype::Shoes, statModifiers, specialEffect)
    {
    }
};

class Amulet final : public Gear
{
public:
    Amulet(ItemDefinition definition,
           AccessorySubtype subtype,
           GearStatModifiers statModifiers = {},
           GearSpecialEffect specialEffect = GearSpecialEffect::None)
        : Gear(std::move(definition), GearSlot::Accessory, false,
               WeaponHandedness::OneHanded, false, subtype, statModifiers, specialEffect)
    {
    }
};