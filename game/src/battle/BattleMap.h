#pragma once

// BattleMap — owns the gameplay grid for one battle.
//
// Responsibilities:
//   - Build the GameTile grid from a loaded TileMapData (buildFrom).
//   - Provide safe tile lookup by grid position (at, isValid).
//   - Expose spawn positions found during build (spawnPositions).
//
// BattleMap does NOT own or reference TileMapData after buildFrom() returns.
// It does NOT handle rendering — that stays in BattleState using TileMapData.
//
// Height extraction rule:
//   Layer name starting with "h" followed by a digit (h1, h2, h3 …) is
//   treated as an elevation layer. The elevation index is (layer number - 1),
//   so h1 = 0, h2 = 1, h3 = 2.
//   A cell's final height is the highest elevation layer that has a non-zero
//   GID at that position — lower layers are terrain underneath and are ignored
//   for gameplay purposes.
//
// Spawn extraction rule:
//   Any tile whose top-layer class is TileClass::SpawnPlayer,
//   TileClass::SpawnEnemy, etc. is recorded in the relevant spawn list.
//   (Spawn classes will be added to TileClass.h when defined in the tileset.)

#include "battle/GameTile.h"
#include "engine/data/TileMapData.h"

#include <vector>
#include <string_view>
#include <unordered_map>

class BattleMap
{
public:
    // Build the gameplay grid from map data.
    // Returns false and logs on failure (empty map, bad dimensions).
    bool buildFrom(const TileMapData &mapData) noexcept;

    // Reset to empty state.
    void clear() noexcept;

    // Returns true if (col, row) is within grid bounds.
    [[nodiscard]] bool isValid(int col, int row) const noexcept;

    // Returns the tile at (col, row). Asserts in debug if out of bounds.
    // Use isValid() first if the position is not guaranteed to be in range.
    [[nodiscard]] const GameTile &at(int col, int row) const noexcept;
    [[nodiscard]] GameTile &at(int col, int row) noexcept;

    // Grid dimensions in tiles.
    [[nodiscard]] int cols() const noexcept { return m_cols; }
    [[nodiscard]] int rows() const noexcept { return m_rows; }

    // Flat tile list — useful for iterating all cells.
    [[nodiscard]] const std::vector<GameTile> &tiles() const noexcept { return m_tiles; }

    // Spawn positions populated during buildFrom().
    // Extend these vectors with more spawn categories as needed.
    std::vector<GameTile *> playerSpawns;
    std::vector<GameTile *> allySpawns;
    std::unordered_map<int, std::vector<GameTile *>> enemySpawnsByTeam;

private:
    int m_cols = 0;
    int m_rows = 0;
    std::vector<GameTile> m_tiles; // row-major: index = row * m_cols + col

    // Parse the elevation index from a layer name ("h1"→0, "h2"→1 …).
    // Returns -1 if the layer is not a height layer.
    [[nodiscard]] static int heightIndexFromLayerName(std::string_view name) noexcept;
};
