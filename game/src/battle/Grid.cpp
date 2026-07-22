#include "battle/Grid.h"

#include <stdexcept>
#include <limits>

Grid::Grid(int width, int height)
    : m_width(width), m_height(height),
      m_tiles(static_cast<size_t>(width) * static_cast<size_t>(height))
{
    if (m_width <= 0 || m_height <= 0)
    {
        throw std::invalid_argument("Grid dimensions must be positive");
    }
}

const Tile &Grid::getTile(Vec2i pos) const
{
    if (!isValid(pos))
    {
        throw std::out_of_range("Grid position is out of bounds");
    }

    return m_tiles[getIndex(pos)];
}

Tile &Grid::getTile(Vec2i pos)
{
    if (!isValid(pos))
    {
        throw std::out_of_range("Grid position is out of bounds");
    }

    return m_tiles[getIndex(pos)];
}

bool Grid::isValid(Vec2i pos) const
{
    return pos.x >= 0 &&
           pos.x < m_width &&
           pos.y >= 0 &&
           pos.y < m_height;
}

int Grid::getWidth() const
{
    return m_width;
}

int Grid::getHeight() const
{
    return m_height;
}

size_t Grid::getIndex(Vec2i pos) const
{
    return static_cast<size_t>(pos.y) * static_cast<size_t>(m_width) +
           static_cast<size_t>(pos.x);
}

// ---------------------- NEW SAFE LAYER ----------------------

const Tile *Grid::tryGetTile(Vec2i pos) const
{
    if (!isValid(pos))
        return nullptr;

    return &m_tiles[getIndex(pos)];
}

int Grid::getMoveCost(Vec2i /*from*/, Vec2i to) const
{
    if (!isValid(to))
        return std::numeric_limits<int>::max();

    return m_tiles[getIndex(to)].moveCost;
}

bool Grid::isOccupied(Vec2i pos) const
{
    if (!isValid(pos))
        return false;

    return m_tiles[getIndex(pos)].occupied;
}
