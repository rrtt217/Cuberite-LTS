// ChorusRules.h

// Declares the pure survival rules of the chorus plant and chorus flower blocks,
// so that they can be unit-tested without constructing a chunk.

#pragma once

#include "../BlockType.h"





/** The blocks surrounding a chorus plant. All four horizontal "below" blocks are needed for the support rule. */
struct sChorusPlantNeighborhood
{
	BLOCKTYPE m_Up;
	BLOCKTYPE m_Down;
	BLOCKTYPE m_North;
	BLOCKTYPE m_South;
	BLOCKTYPE m_West;
	BLOCKTYPE m_East;
	BLOCKTYPE m_NorthBelow;
	BLOCKTYPE m_SouthBelow;
	BLOCKTYPE m_WestBelow;
	BLOCKTYPE m_EastBelow;
};





/** Returns whether the given block is a chorus plant. */
inline bool IsChorusPlant(BLOCKTYPE a_Block)
{
	return a_Block == E_BLOCK_CHORUS_PLANT;
}





/** Returns whether the given block can support a chorus plant (a chorus plant or End stone). */
inline bool IsChorusBase(BLOCKTYPE a_Block)
{
	return (a_Block == E_BLOCK_CHORUS_PLANT) || (a_Block == E_BLOCK_END_STONE);
}





/** Returns whether a chorus plant with the given neighbourhood may stay.
See the chorus plant specification for the exact rules. */
inline bool ChorusPlantCanSurvive(const sChorusPlantNeighborhood & a_N)
{
	// A plant touching another plant horizontally must have air above or below it:
	const bool HasHorizontalPlant = IsChorusPlant(a_N.m_North) || IsChorusPlant(a_N.m_South) || IsChorusPlant(a_N.m_West) || IsChorusPlant(a_N.m_East);
	const bool HasVerticalAir = (a_N.m_Up == E_BLOCK_AIR) || (a_N.m_Down == E_BLOCK_AIR);
	if (HasHorizontalPlant && !HasVerticalAir)
	{
		return false;
	}

	// Supported by End stone / a plant directly below:
	if (IsChorusBase(a_N.m_Down))
	{
		return true;
	}

	// Supported by a horizontally adjacent plant that itself has End stone / a plant below:
	return
		(IsChorusPlant(a_N.m_North) && IsChorusBase(a_N.m_NorthBelow)) ||
		(IsChorusPlant(a_N.m_South) && IsChorusBase(a_N.m_SouthBelow)) ||
		(IsChorusPlant(a_N.m_West)  && IsChorusBase(a_N.m_WestBelow))  ||
		(IsChorusPlant(a_N.m_East)  && IsChorusBase(a_N.m_EastBelow));
}





/** Returns whether a chorus flower may stay at a position with the given blocks.
a_HorizontalPlants is the number of chorus plants directly beside the flower, and is only used when there is no support below. */
inline bool ChorusFlowerCanSurvive(BLOCKTYPE a_Down, BLOCKTYPE a_Up, int a_HorizontalPlants)
{
	// The flower may sit on End stone or on a chorus plant:
	if ((a_Down == E_BLOCK_END_STONE) || (a_Down == E_BLOCK_CHORUS_PLANT))
	{
		return true;
	}

	// Otherwise it must hang in the air next to exactly one chorus plant:
	if (a_Up != E_BLOCK_AIR)
	{
		return false;
	}
	return (a_HorizontalPlants == 1);
}
