
#pragma once

#include "../../BlockEntities/CommandBlockEntity.h"
#include "../../BlockEntities/CommandBlockRules.h"





namespace CommandBlockHandler
{
	static PowerLevel GetPowerDeliveredToPosition(const cChunk & a_Chunk, Vector3i a_Position, BLOCKTYPE a_BlockType, Vector3i a_QueryPosition, BLOCKTYPE a_QueryBlockType, bool IsLinked)
	{
		UNUSED(a_Chunk);
		UNUSED(a_Position);
		UNUSED(a_BlockType);
		UNUSED(a_QueryPosition);
		UNUSED(a_QueryBlockType);
		UNUSED(IsLinked);
		return 0;
	}

	static void Update(cChunk & a_Chunk, cChunk &, Vector3i a_Position, BLOCKTYPE a_BlockType, NIBBLETYPE a_Meta, const PowerLevel Power)
	{
		// LOGD("Evaluating commander the cmdblck (%d %d %d)", a_Position.x, a_Position.y, a_Position.z);
		UNUSED(a_Meta);

		const auto Previous = DataForChunk(a_Chunk).ExchangeUpdateOncePowerData(a_Position, Power);
		const auto Type = CommandBlockTypeFromBlockType(a_BlockType);
		const bool Powered = (Power != 0);

		a_Chunk.DoWithBlockEntityAt(a_Position, [&](cBlockEntity & a_BlockEntity)
		{
			auto & CommandBlock = static_cast<cCommandBlockEntity &>(a_BlockEntity);
			CommandBlock.SetPowered(Powered);

			// Repeating blocks run from their own Tick for as long as they are powered:
			if (Type == eCommandBlockType::Repeat)
			{
				return false;
			}

			// Impulse and chain blocks run once per activation (rising edge):
			if (Powered && (Previous == 0))
			{
				CommandBlock.Activate();
			}
			return false;
		});
	}

	static void ForValidSourcePositions(const cChunk & a_Chunk, Vector3i a_Position, BLOCKTYPE a_BlockType, NIBBLETYPE a_Meta, ForEachSourceCallback & Callback)
	{
		UNUSED(a_Chunk);
		UNUSED(a_BlockType);
		UNUSED(a_Meta);
		InvokeForAdjustedRelatives(Callback, a_Position, RelativeAdjacents);
	}
};
