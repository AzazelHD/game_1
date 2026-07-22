#include "battle/BattleMap.h"
#include "engine/data/TileClass.h"
#include "engine/core/Log.h"

#include <cassert>
#include <cctype>
#include <cmath>
#include <string_view>

// ─────────────────────────────────────────────────────────────────────────────
// Public
// ─────────────────────────────────────────────────────────────────────────────

bool BattleMap::buildFrom(const TileMapData &mapData) noexcept
{
    clear();

    if (mapData.isEmpty())
    {
        LOG_ERROR("BattleMap", "buildFrom: map data is empty");
        return false;
    }

    m_cols = mapData.width;
    m_rows = mapData.height;

    // Allocate grid — all cells start at height 0, no class, not impassable.
    m_tiles.resize(
        static_cast<std::size_t>(m_cols) *
        static_cast<std::size_t>(m_rows));

    // Initialise grid positions (height and classFlags filled in below).
    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c)
        {
            GameTile &tile = m_tiles[static_cast<std::size_t>(r * m_cols + c)];
            tile.col = c;
            tile.row = r;
        }

    // ── Pass 1: height layers ─────────────────────────────────────────────
    //
    // Layers named h1, h2, h3 … encode elevation (0, 1, 2 …).
    // For each non-zero GID, update the cell's height and terrain classFlags.
    // Processing in layer order means the last non-zero GID wins — the
    // topmost visible tile is the one the player stands on.

    for (const auto &layer : mapData.layers)
    {
        if (layer.type != LayerType::Tile)
            continue;

        const int elevationIndex = heightIndexFromLayerName(layer.name);
        if (elevationIndex < 0)
            continue; // not a height layer

        if (layer.tiles.empty())
            continue;

        const std::size_t expectedSize =
            static_cast<std::size_t>(layer.width) *
            static_cast<std::size_t>(layer.height);

        if (layer.tiles.size() < expectedSize)
        {
            LOG_WARN(
                "BattleMap",
                "buildFrom: layer '%s' has %zu tiles, expected %zu; skipping",
                layer.name.c_str(),
                layer.tiles.size(),
                expectedSize);
            continue;
        }

        for (int r = 0; r < layer.height; ++r)
        {
            for (int c = 0; c < layer.width; ++c)
            {
                const auto gid =
                    layer.tiles[static_cast<std::size_t>(r * layer.width + c)];

                if (gid == 0)
                    continue;

                if (!isValid(c, r))
                    continue;

                GameTile &tile = at(c, r);
                tile.height = elevationIndex;

                // Resolve terrain class from the tileset type registry.
                const TileTypeId typeId = mapData.tileTypeForGlobalTileId(gid);
                const std::string_view typeName = mapData.tileTypeName(typeId);
                tile.classFlags = TileClass::toFlag(typeName);
            }
        }
    }

    // ── Pass 2: object layers ─────────────────────────────────────────────
    //
    // Tiled stores object coordinates on isometric maps in its object plane,
    // not in the same screen-space coordinates used to render tile diamonds.
    // For point objects placed on tile cells, both axes advance in units of
    // tileHeight. Example from this map:
    //   x=107.8, y=155.8 with tileHeight=24 -> tile (4, 6)
    //   x=226.6, y=226.8 with tileHeight=24 -> tile (9, 9)
    //
    // So for spawn points we convert by dividing both axes by tileHeight and
    // flooring to the containing tile cell. This object-plane coordinate space
    // is distinct from projected render-space iso pixels.

    if (mapData.tileHeight <= 0)
    {
        LOG_WARN("BattleMap", "buildFrom: tileHeight is %d; cannot map object layers", mapData.tileHeight);
        return true;
    }

    const float objectUnit = static_cast<float>(mapData.tileHeight);

    for (const auto &layer : mapData.layers)
    {
        if (layer.type != LayerType::Object)
            continue;

        // Check for player spawn
        if (layer.className == TileClass::SpawnPlayer)
        {
            for (const auto &obj : layer.objects)
            {
                if (!obj.isPoint)
                    continue;

                const int col = static_cast<int>(std::floor(obj.x / objectUnit));
                const int row = static_cast<int>(std::floor(obj.y / objectUnit));

                if (!isValid(col, row))
                    continue;

                playerSpawns.push_back(&at(col, row));
            }
        }
        // Check for ally spawn
        else if (layer.className == TileClass::SpawnAlly)
        {
            for (const auto &obj : layer.objects)
            {
                if (!obj.isPoint)
                    continue;

                const int col = static_cast<int>(std::floor(obj.x / objectUnit));
                const int row = static_cast<int>(std::floor(obj.y / objectUnit));

                if (!isValid(col, row))
                    continue;

                allySpawns.push_back(&at(col, row));
            }
        }
        // Check for enemy spawn (starts with "spawn_enemy_")
        else if (layer.className.rfind("spawn_enemy_", 0) == 0)
        {
            int team = std::stoi(std::string(layer.className.substr(12)));

            for (const auto &obj : layer.objects)
            {
                if (!obj.isPoint)
                    continue;

                const int col = static_cast<int>(std::floor(obj.x / objectUnit));
                const int row = static_cast<int>(std::floor(obj.y / objectUnit));

                if (!isValid(col, row))
                    continue;

                enemySpawnsByTeam[team].push_back(&at(col, row));
            }
        }
    }

    LOG_INFO(
        "BattleMap",
        "built: %dx%d grid, %zu tiles, %zu player spawns, %zu ally spawns, %zu enemy teams",
        m_cols,
        m_rows,
        m_tiles.size(),
        playerSpawns.size(),
        allySpawns.size(),
        enemySpawnsByTeam.size());

    return true;
}

void BattleMap::clear() noexcept
{
    m_cols = 0;
    m_rows = 0;
    m_tiles.clear();
    playerSpawns.clear();
    allySpawns.clear();
    enemySpawnsByTeam.clear();
}

bool BattleMap::isValid(int col, int row) const noexcept
{
    return col >= 0 && col < m_cols && row >= 0 && row < m_rows;
}

const GameTile &BattleMap::at(int col, int row) const noexcept
{
    assert(isValid(col, row) && "BattleMap::at - position out of bounds");
    return m_tiles[static_cast<std::size_t>(row * m_cols + col)];
}

GameTile &BattleMap::at(int col, int row) noexcept
{
    assert(isValid(col, row) && "BattleMap::at - position out of bounds");
    return m_tiles[static_cast<std::size_t>(row * m_cols + col)];
}

// ─────────────────────────────────────────────────────────────────────────────
// Private
// ─────────────────────────────────────────────────────────────────────────────

// "h1" → 0, "h2" → 1, "h3" → 2 … Returns -1 for non-height layers.
int BattleMap::heightIndexFromLayerName(std::string_view name) noexcept
{
    if (name.size() < 2)
        return -1;

    if (name[0] != 'h' && name[0] != 'H')
        return -1;

    for (std::size_t i = 1; i < name.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(name[i])))
            return -1;

    const int layerNumber = std::stoi(std::string(name.substr(1)));
    return layerNumber - 1; // h1 → 0, h2 → 1 …
}
