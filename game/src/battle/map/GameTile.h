#pragma once

// GameTile — one cell in the battle grid.
//
// Populated by BattleMap during map load; consumed by movement, targeting,
// and UI systems. Rendering uses TileMapData directly — GameTile is
// gameplay-only and carries no visual data.
//
// Fields:
//   col, row    — zero-based grid position (x = col, y = row in map space).
//   height      — elevation level derived from the tile layer index:
//                   h1 layer → height 0 (ground floor)
//                   h2 layer → height 1
//                   h3 layer → height 2  … etc.
//                 Height difference drives jump/fall rules; there is no
//                 separate "wall" type — a cliff is just a large height gap.
//   classFlags  — bitmask of TileClass::Flag* constants describing terrain
//                 type. Use TileClass::kWalkable to check traversability.
//   impassable  — true when a unit or impassable object (building, rock)
//                 is currently on this cell. Set at runtime, not from map data.

#include <cstdint>

struct GameTile
{
    int col = 0;
    int row = 0;
    int height = 0;           // elevation level (0 = ground floor)
    uint32_t classFlags = 0u; // TileClass::Flag* bitmask
    bool impassable = false;  // runtime — set when a building/rock occupies this cell
};
