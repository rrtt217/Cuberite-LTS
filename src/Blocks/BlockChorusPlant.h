// BlockChorusPlant.h

#pragma once

#include "BlockHandler.h"
#include "ChorusRules.h"
#include "../FastRandom.h"





/** Handler for the chorus plant block.
Chorus plants are supported by End stone or neighbouring chorus plants and drop 0-1 chorus fruit when broken. */
class cChorusPlantHandler final :
	public cBlockHandler
{
	using Super = cBlockHandler;

public:

	using Super::Super;

private:

	virtual bool CanBeAt(const cChunk & a_Chunk, const Vector3i a_Position, const NIBBLETYPE a_Meta) const override
	{
		UNUSED(a_Meta);

		sChorusPlantNeighborhood N;
		N.m_Up = GetBlockAt(a_Chunk, a_Position.addedY(1));
		N.m_Down = GetBlockAt(a_Chunk, a_Position.addedY(-1));
		N.m_North = GetBlockAt(a_Chunk, a_Position.addedZ(-1));
		N.m_South = GetBlockAt(a_Chunk, a_Position.addedZ(1));
		N.m_West = GetBlockAt(a_Chunk, a_Position.addedX(-1));
		N.m_East = GetBlockAt(a_Chunk, a_Position.addedX(1));
		N.m_NorthBelow = GetBlockAt(a_Chunk, a_Position.addedZ(-1).addedY(-1));
		N.m_SouthBelow = GetBlockAt(a_Chunk, a_Position.addedZ(1).addedY(-1));
		N.m_WestBelow = GetBlockAt(a_Chunk, a_Position.addedX(-1).addedY(-1));
		N.m_EastBelow = GetBlockAt(a_Chunk, a_Position.addedX(1).addedY(-1));
		return ChorusPlantCanSurvive(N);
	}





	virtual cItems ConvertToPickups(const NIBBLETYPE a_BlockMeta, const cItem * const a_Tool) const override
	{
		UNUSED(a_BlockMeta);
		UNUSED(a_Tool);

		// Chorus plants drop 0-1 chorus fruit, regardless of the tool (Fortune has no effect):
		if (GetRandomProvider().RandBool(0.5))
		{
			return cItem(E_ITEM_CHORUS_FRUIT, 1);
		}
		return {};
	}





	/** Returns the block at the given chunk-relative position, or air if it cannot be resolved. */
	static BLOCKTYPE GetBlockAt(const cChunk & a_Chunk, const Vector3i a_RelPos)
	{
		BLOCKTYPE Block = E_BLOCK_AIR;
		a_Chunk.UnboundedRelGetBlockType(a_RelPos, Block);
		return Block;
	}
};
