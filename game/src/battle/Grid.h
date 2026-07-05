#pragma once

#include "engine/math/Vec2.h"

#include <vector>

// Tile types — extend as your game grows (forest, mountain, water, etc.)
enum class TileType
{
    Plain,
    Forest,
    Mountain,
    Wall,
};

// A single tile in the grid. Keep it as a plain struct — no methods needed.
struct Tile
{
    TileType type = TileType::Plain;
    int moveCost = 1;      // movement points spent to enter this tile
    bool occupied = false; // true if a unit is standing here
    int height = 0;        // elevation — affects combat hit/evasion in some TRPGs
};

// Grid owns the 2D array of Tiles. It knows the map layout but nothing about units.
//
// [x]: Implement the class with:
//   - Constructor: Grid(int width, int height)  — allocates m_tiles as a flat vector.
//   - getTile(Vec2i pos) const → const Tile&
//   - getTile(Vec2i pos)       → Tile&   (mutable, for marking occupied)
//   - isValid(Vec2i pos) const → bool    (bounds check — use this everywhere!)
//   - getWidth(), getHeight()
//
// Storage: use a flat std::vector<Tile> of size width*height.
//          Index formula: pos.y * m_width + pos.x
class Grid
{
public:
    Grid() = default;
    Grid(int width, int height);

    const Tile &getTile(Vec2i pos) const;
    Tile &getTile(Vec2i pos);

    [[nodiscard]] bool isValid(Vec2i pos) const;

    [[nodiscard]] int getWidth() const;
    [[nodiscard]] int getHeight() const;

    // SAFE ACCESS (no exceptions, returns nullptr if invalid)
    [[nodiscard]] const Tile *tryGetTile(Vec2i pos) const;

    // Movement helper (centralized cost access)
    [[nodiscard]] int getMoveCost(Vec2i from, Vec2i to) const;

    // Simple occupancy query (no unit system dependency)
    [[nodiscard]] bool isOccupied(Vec2i pos) const;

private:
    [[nodiscard]] size_t getIndex(Vec2i pos) const;

    int m_width = 0;
    int m_height = 0;
    std::vector<Tile> m_tiles;
};