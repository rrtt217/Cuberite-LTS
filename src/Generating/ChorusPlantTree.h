// ChorusPlantTree.h

#pragma once

#include "../ChunkDef.h"





/** Builds the image of one naturally generated chorus tree.
The tree is grown from a single flower on End stone using the same rules as a player-planted flower,
with every random choice taken from a deterministic RNG seeded from the tree's base position. */
class cChorusPlantTree
{
public:
	/** Generates the blocks of a chorus tree rooted at a_Base.
	a_Base is the lowest block of the tree (the first plant block, directly above End stone).
	Returns the tree's blocks in world coordinates. */
	static sSetBlockVector Generate(const Vector3i & a_Base, int a_Seed);
};
