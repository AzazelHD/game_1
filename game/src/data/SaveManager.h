#pragma once

#include "inventory/Inventory.h"
#include "systems/PartySystem.h"
#include "systems/RosterSystem.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// Campaign snapshot captured from / restored into PartyContext. Battle
// snapshots are a separate, richer payload handled by BattleSaveData.
struct CampaignSave
{
    int formatVersion = 1;
    int worldNodeId = 4;
    int nextInstanceId = 0;
    std::vector<RosterUnit> roster;
    std::vector<int> partyMemberIds;
    nlohmann::json inventoryJson;
    std::string label;
};

enum class SaveKind
{
    Campaign,
    Battle,
};

// One entry in the persistent save queue (newest first).
struct SaveSlotInfo
{
    int slotIndex = -1;            // file number, stable per save data file
    SaveKind kind = SaveKind::Campaign;
    std::string label;             // human-readable (e.g. "Node 4" / "Battle: node 6")
    std::uint64_t timestamp = 0;   // unix seconds
    nlohmann::json payload;        // campaign or battle payload for this entry
};

namespace
{
    // Lightweight obfuscation so saved game data is not plaintext or
    // trivially editable. This is not a security boundary — it is a
    // deterrent against casual hex/inventory editing.
    constexpr unsigned char kXorKey = 0x5A;

    constexpr const char *kSaveFilePath = "user.dat";

    std::vector<unsigned char> obfuscate(const std::string &plain)
    {
        std::vector<unsigned char> out(plain.size());
        for (std::size_t i = 0; i < plain.size(); ++i)
            out[i] = static_cast<unsigned char>(plain[i]) ^ kXorKey;
        return out;
    }

    std::string deobfuscate(const std::vector<unsigned char> &data)
    {
        std::string out(data.size(), '\0');
        for (std::size_t i = 0; i < data.size(); ++i)
            out[i] = static_cast<char>(data[i] ^ kXorKey);
        return out;
    }

    bool readWholeFile(const char *path, std::vector<unsigned char> &out)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
            return false;
        file.seekg(0, std::ios::end);
        const std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        if (size < 0)
            return false;
        out.resize(static_cast<std::size_t>(size));
        if (size > 0 && !file.read(reinterpret_cast<char *>(out.data()), size))
            return false;
        return true;
    }

    bool writeWholeFile(const char *path, const std::string &bytes)
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
            return false;
        file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        return static_cast<bool>(file);
    }

    // ItemId serializer/parser helpers (uint16 -> JSON).
    nlohmann::json optIdToJson(const std::optional<ItemId> &id)
    {
        return id.has_value() ? nlohmann::json(id.value()) : nlohmann::json(nullptr);
    }

    bool optIdFromJson(const nlohmann::json &value, std::optional<ItemId> &out)
    {
        if (value.is_null())
        {
            out.reset();
            return true;
        }
        if (!value.is_number_unsigned())
            return false;
        const auto id = value.get<std::uint64_t>();
        if (id > UINT16_MAX)
            return false;
        out = static_cast<ItemId>(id);
        return true;
    }

    bool campaignFromJson(const nlohmann::json &j, CampaignSave &out)
    {
        if (!j.contains("roster"))
            return false;

        out = CampaignSave{};
        out.formatVersion = j.value("formatVersion", 1);
        out.worldNodeId = j.value("worldNodeId", 4);
        out.label = j.value("label", std::string());

        for (const nlohmann::json &entry : j.at("roster"))
        {
            RosterUnit u;
            u.instanceId = entry.value("instanceId", -1);
            u.templatePath = entry.value("templatePath", std::string());
            u.customName = entry.value("customName", std::string());
            u.recruited = entry.value("recruited", true);
            u.exp = entry.value("exp", 0);
            u.level = entry.value("level", 1);
            if (u.instanceId < 0 || u.templatePath.empty())
                return false;

            if (entry.contains("equippedGear"))
            {
                const nlohmann::json &gear = entry.at("equippedGear");
                if (gear.contains("slots"))
                {
                    const auto &arr = gear.at("slots");
                    for (std::size_t i = 0; i < arr.size() && i < u.equippedGear.slots.size(); ++i)
                        if (!optIdFromJson(arr[i], u.equippedGear.slots[i]))
                            return false;
                }
                if (gear.contains("accessories"))
                {
                    const auto &arr = gear.at("accessories");
                    for (std::size_t i = 0; i < arr.size() && i < u.equippedGear.accessories.size(); ++i)
                        if (!optIdFromJson(arr[i], u.equippedGear.accessories[i]))
                            return false;
                }
            }
            out.roster.push_back(std::move(u));
        }

        out.partyMemberIds = j.value("party", std::vector<int>{});
        if (j.contains("inventory"))
            out.inventoryJson = j.at("inventory");

        for (const RosterUnit &u : out.roster)
            out.nextInstanceId = std::max(out.nextInstanceId, u.instanceId + 1);
        return true;
    }

    bool battleFromJson(const nlohmann::json &j, CampaignSave &out, nlohmann::json &battleOut)
    {
        if (!j.contains("roster") || !j.contains("battle"))
            return false;

        out = CampaignSave{};
        out.formatVersion = j.value("formatVersion", 1);
        out.worldNodeId = j.value("worldNodeId", 4);
        out.label = j.value("label", std::string());

        for (const nlohmann::json &entry : j.at("roster"))
        {
            RosterUnit u;
            u.instanceId = entry.value("instanceId", -1);
            u.templatePath = entry.value("templatePath", std::string());
            u.customName = entry.value("customName", std::string());
            u.recruited = entry.value("recruited", true);
            u.exp = entry.value("exp", 0);
            u.level = entry.value("level", 1);
            if (u.instanceId < 0 || u.templatePath.empty())
                return false;
            if (entry.contains("equippedGearSlots"))
            {
                const auto &arr = entry.at("equippedGearSlots");
                for (std::size_t i = 0; i < arr.size() && i < u.equippedGear.slots.size(); ++i)
                    if (!optIdFromJson(arr[i], u.equippedGear.slots[i]))
                        return false;
            }
            if (entry.contains("equippedGearAccessories"))
            {
                const auto &arr = entry.at("equippedGearAccessories");
                for (std::size_t i = 0; i < arr.size() && i < u.equippedGear.accessories.size(); ++i)
                    if (!optIdFromJson(arr[i], u.equippedGear.accessories[i]))
                        return false;
            }
            out.roster.push_back(std::move(u));
        }
        out.partyMemberIds = j.value("party", std::vector<int>{});
        if (j.contains("inventory"))
            out.inventoryJson = j.at("inventory");
        for (const RosterUnit &u : out.roster)
            out.nextInstanceId = std::max(out.nextInstanceId, u.instanceId + 1);

        battleOut = j.at("battle");
        return true;
    }
}

// Owns the single-slot-with-queue save system.
//
// File (single, loose in the working directory):
//   user.dat — the full save queue: metadata plus every save's payload,
//              obfuscated (not a security boundary).
//
// Layout after deobfuscation:
//   {
//     "v": 1,
//     "entries": [
//       { "slot": 0, "kind": 0, "label": "Node 4", "ts": 123, "data": { ... } }
//     ]
//   }
//
// Battle save semantics (FFTA-style):
//   - A battle save stores campaign state AND a full mid-battle snapshot.
//   - Only one battle save may exist at a time. Writing a new battle save
//     replaces the previous one.
//   - Loading a battle save reconstructs the battle, then that battle save
//     is consumed (removed). The campaign save beneath it remains, so
//     quitting after a battle-load returns the player to the world map.
class SaveManager
{
public:
    static SaveManager &instance();

    // ── Save queue metadata ───────────────────────────────────────────────
    // Loads the whole user.dat queue (metadata + payloads) on first call.
    void loadIndex();
    const std::vector<SaveSlotInfo> &slots() const { return m_slots; }
    bool hasSave() const { return !m_slots.empty(); }

    // Most recent campaign save (battle saves excluded). Used as the
    // fallback target when a battle save is consumed.
    int latestCampaignSlot() const;

    // Removes a save (any kind) from the queue and rewrites user.dat.
    void deleteSave(int slotIndex);

    // ── Campaign save ─────────────────────────────────────────────────────
    bool saveCampaign(const RosterSystem &roster, const PartySystem &party,
                      const Inventory &inventory, int worldNodeId);

    // Loads the newest campaign save. Returns false when none exists or
    // the file is corrupt.
    bool loadLatestCampaign(CampaignSave &out) const;

    // Loads a specific campaign save by its queue entry's slotIndex.
    // Returns false when the slot is not a campaign save or is corrupt.
    bool loadCampaign(int slotIndex, CampaignSave &out) const;

    // ── Battle save ───────────────────────────────────────────────────────
    // Replaces the current battle save (if any) with a fresh one.
    bool saveBattle(const CampaignSave &campaign, const nlohmann::json &battlePayload);

    // Loads a battle save entry's campaign + battle payload. Does not
    // consume the entry — call consumeBattleSave() only after the battle
    // actually loads successfully.
    bool loadBattle(int slotIndex, CampaignSave &out, nlohmann::json &battleOut) const;

    // Removes a battle save entry from the queue (called after a battle
    // load has successfully reconstructed the fight).
    void consumeBattleSave(int slotIndex);

    // Returns the index of the battle save, or -1 if none exists.
    int battleSaveSlot() const;

private:
    SaveManager() = default;
    SaveManager(const SaveManager &) = delete;
    SaveManager &operator=(const SaveManager &) = delete;

    int allocateNewSlot();
    bool writeFile();

    std::vector<SaveSlotInfo> m_slots;
    bool m_indexLoaded = false;

    static constexpr int kMaxSlots = 8;
};

inline SaveManager &SaveManager::instance()
{
    static SaveManager s;
    return s;
}

inline void SaveManager::loadIndex()
{
    if (m_indexLoaded)
        return;
    m_indexLoaded = true;
    m_slots.clear();

    std::vector<unsigned char> raw;
    if (!readWholeFile(kSaveFilePath, raw))
        return;

    const std::string text = deobfuscate(raw);
    try
    {
        const nlohmann::json j = nlohmann::json::parse(text);
        if (!j.is_object() || !j.contains("entries") || !j.at("entries").is_array())
            return;

        for (const nlohmann::json &entry : j.at("entries"))
        {
            SaveSlotInfo info;
            info.slotIndex = entry.value("slot", -1);
            info.kind = entry.value("kind", 0) == 1 ? SaveKind::Battle : SaveKind::Campaign;
            info.label = entry.value("label", std::string());
            info.timestamp = entry.value("ts", 0ULL);
            if (info.slotIndex >= 0 && entry.contains("data"))
            {
                info.payload = entry.at("data");
                m_slots.push_back(std::move(info));
            }
        }
    }
    catch (...)
    {
        m_slots.clear();
    }
}

inline int SaveManager::latestCampaignSlot() const
{
    for (const SaveSlotInfo &info : m_slots)
        if (info.kind == SaveKind::Campaign)
            return info.slotIndex;
    return -1;
}

inline int SaveManager::battleSaveSlot() const
{
    for (const SaveSlotInfo &info : m_slots)
        if (info.kind == SaveKind::Battle)
            return info.slotIndex;
    return -1;
}

inline int SaveManager::allocateNewSlot()
{
    int maxSlot = -1;
    for (const SaveSlotInfo &info : m_slots)
        maxSlot = std::max(maxSlot, info.slotIndex);
    return maxSlot + 1;
}

inline bool SaveManager::saveCampaign(const RosterSystem &roster, const PartySystem &party,
                                      const Inventory &inventory, int worldNodeId)
{
    nlohmann::json j;
    j["formatVersion"] = 1;
    j["kind"] = 0;
    j["worldNodeId"] = worldNodeId;

    // Roster
    nlohmann::json rosterJson = nlohmann::json::array();
    for (const RosterUnit &u : roster.units())
    {
        nlohmann::json entry;
        entry["instanceId"] = u.instanceId;
        entry["templatePath"] = u.templatePath;
        entry["customName"] = u.customName;
        entry["recruited"] = u.recruited;
        entry["exp"] = u.exp;
        entry["level"] = u.level;

        nlohmann::json gearJson;
        nlohmann::json slots = nlohmann::json::array();
        for (const auto &slot : u.equippedGear.slots)
            slots.push_back(optIdToJson(slot));
        gearJson["slots"] = std::move(slots);

        nlohmann::json acc = nlohmann::json::array();
        for (const auto &a : u.equippedGear.accessories)
            acc.push_back(optIdToJson(a));
        gearJson["accessories"] = std::move(acc);

        entry["equippedGear"] = std::move(gearJson);
        rosterJson.push_back(std::move(entry));
    }
    j["roster"] = std::move(rosterJson);

    // Party
    j["party"] = party.memberIds();

    // Inventory
    j["inventory"] = inventory.toJson();

    // Queue bookkeeping — newest first.
    const int slot = allocateNewSlot();
    SaveSlotInfo info;
    info.slotIndex = slot;
    info.kind = SaveKind::Campaign;
    info.label = "Node " + std::to_string(worldNodeId);
    info.timestamp = static_cast<std::uint64_t>(std::time(nullptr));
    info.payload = std::move(j);

    m_slots.insert(m_slots.begin(), std::move(info));
    while (static_cast<int>(m_slots.size()) > kMaxSlots)
    {
        // Drop the oldest (unless it is the sole battle save — that is
        // protected by the single-battle-save model).
        const SaveSlotInfo &oldest = m_slots.back();
        if (oldest.kind == SaveKind::Battle)
            break;
        m_slots.pop_back();
    }
    return writeFile();
}

inline bool SaveManager::loadLatestCampaign(CampaignSave &out) const
{
    const int slot = latestCampaignSlot();
    if (slot < 0)
        return false;
    return loadCampaign(slot, out);
}

inline bool SaveManager::loadCampaign(int slotIndex, CampaignSave &out) const
{
    for (const SaveSlotInfo &info : m_slots)
    {
        if (info.slotIndex == slotIndex)
        {
            if (info.kind != SaveKind::Campaign)
                return false;
            return campaignFromJson(info.payload, out);
        }
    }
    return false;
}

inline bool SaveManager::saveBattle(const CampaignSave &campaign, const nlohmann::json &battlePayload)
{
    // Remove any existing battle save first (single-battle-save model).
    const int existing = battleSaveSlot();
    if (existing >= 0)
    {
        m_slots.erase(std::remove_if(m_slots.begin(), m_slots.end(),
                                     [existing](const SaveSlotInfo &s)
                                     { return s.slotIndex == existing; }),
                      m_slots.end());
    }

    nlohmann::json j;
    j["formatVersion"] = 1;
    j["kind"] = 1;
    j["worldNodeId"] = campaign.worldNodeId;

    nlohmann::json rosterJson = nlohmann::json::array();
    for (const RosterUnit &u : campaign.roster)
    {
        nlohmann::json entry;
        entry["instanceId"] = u.instanceId;
        entry["templatePath"] = u.templatePath;
        entry["customName"] = u.customName;
        entry["recruited"] = u.recruited;
        entry["exp"] = u.exp;
        entry["level"] = u.level;
        nlohmann::json slots = nlohmann::json::array();
        for (const auto &slot : u.equippedGear.slots)
            slots.push_back(optIdToJson(slot));
        entry["equippedGearSlots"] = std::move(slots);
        nlohmann::json acc = nlohmann::json::array();
        for (const auto &a : u.equippedGear.accessories)
            acc.push_back(optIdToJson(a));
        entry["equippedGearAccessories"] = std::move(acc);
        rosterJson.push_back(std::move(entry));
    }
    j["roster"] = std::move(rosterJson);
    j["party"] = campaign.partyMemberIds;
    j["inventory"] = campaign.inventoryJson;
    j["battle"] = battlePayload;

    const int slot = allocateNewSlot();
    SaveSlotInfo info;
    info.slotIndex = slot;
    info.kind = SaveKind::Battle;
    info.label = "Battle: Node " + std::to_string(campaign.worldNodeId);
    info.timestamp = static_cast<std::uint64_t>(std::time(nullptr));
    info.payload = std::move(j);

    m_slots.insert(m_slots.begin(), std::move(info));
    return writeFile();
}

inline bool SaveManager::loadBattle(int slotIndex, CampaignSave &out, nlohmann::json &battleOut) const
{
    for (const SaveSlotInfo &info : m_slots)
    {
        if (info.slotIndex == slotIndex)
        {
            if (info.kind != SaveKind::Battle)
                return false;
            return battleFromJson(info.payload, out, battleOut);
        }
    }
    return false;
}

inline void SaveManager::consumeBattleSave(int slotIndex)
{
    deleteSave(slotIndex);
}

inline void SaveManager::deleteSave(int slotIndex)
{
    auto it = std::find_if(m_slots.begin(), m_slots.end(),
                           [slotIndex](const SaveSlotInfo &s)
                           { return s.slotIndex == slotIndex; });
    if (it == m_slots.end())
        return;
    m_slots.erase(it);
    (void)writeFile();
}

inline bool SaveManager::writeFile()
{
    nlohmann::json j = nlohmann::json::object();
    j["v"] = 1;
    nlohmann::json entries = nlohmann::json::array();
    for (const SaveSlotInfo &info : m_slots)
    {
        nlohmann::json entry;
        entry["slot"] = info.slotIndex;
        entry["kind"] = info.kind == SaveKind::Battle ? 1 : 0;
        entry["label"] = info.label;
        entry["ts"] = info.timestamp;
        entry["data"] = info.payload;
        entries.push_back(std::move(entry));
    }
    j["entries"] = std::move(entries);

    const std::string plain = j.dump();
    const std::vector<unsigned char> encoded = obfuscate(plain);
    return writeWholeFile(kSaveFilePath,
                          std::string(reinterpret_cast<const char *>(encoded.data()), encoded.size()));
}