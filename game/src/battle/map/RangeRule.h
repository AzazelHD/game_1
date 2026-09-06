#pragma once

// One RangeRule per skill or basic attack. All reach/splash checks use
// Manhattan distance (4-connected). Describes the tile reach, height
// tolerance, and line-of-sight requirements for both direct targeting
// and AoE splash expansion.
//
// Defaults represent a melee slash (range 1, same-height, no LOS).
struct RangeRule
{
    int range = 1;
    int area = 0;

    // Max absolute height difference allowed between attacker and target.
    // -1 = ignore height entirely; 0 = same height only; N = within N levels.
    int heightTolerance = 0;

    bool requiresLineOfSight = false;

    // When true, each AoE splash tile is individually LOS-checked from
    // the center. A unit behind a wall cannot be hit by splash that
    // passes through the wall.
    bool splashRespectsWalls = true;
};
