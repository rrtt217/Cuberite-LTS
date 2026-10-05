// EndGateway.h

// Declares the shared block layout of an End gateway structure.

#pragma once

#include "ChunkDesc.h"
#include "../Vector3.h"

#include <array>





/** Offsets of the plus-shaped bedrock layers directly around an End gateway block.
Ref: https://minecraft.wiki/w/End_Gateway/Structure */
inline constexpr std::array<Vector3i, 10> ENDER_DRAGON_GATEWAY_BEDROCK_OFFSETS =
{
	Vector3i(0, -1, 0), Vector3i(-1, -1, 0), Vector3i(1, -1, 0), Vector3i(0, -1, -1), Vector3i(0, -1, 1),
	Vector3i(0, 1, 0), Vector3i(-1, 1, 0), Vector3i(1, 1, 0), Vector3i(0, 1, -1), Vector3i(0, 1, 1),
};





/** Places the bedrock caps and the gateway block of one End gateway into a chunk being generated.
a_Pos is the world position of the gateway block; anything outside the chunk is ignored. */
inline void PlaceEndGatewayStructure(cChunkDesc & a_ChunkDesc, const Vector3i & a_Pos)
{
	const Vector3i Base = cChunkDef::AbsoluteToRelative(a_Pos, a_ChunkDesc.GetChunkCoords());

	auto SetBlock = [&a_ChunkDesc, &Base](const Vector3i & a_Offset, BLOCKTYPE a_BlockType)
	{
		const Vector3i Rel = Base + a_Offset;
		if (cChunkDef::IsValidRelPos(Rel))
		{
			a_ChunkDesc.SetBlockType(Rel.x, Rel.y, Rel.z, a_BlockType);
		}
	};

	// The single bedrock caps above and below:
	SetBlock(Vector3i(0, -2, 0), E_BLOCK_BEDROCK);
	SetBlock(Vector3i(0, 2, 0), E_BLOCK_BEDROCK);

	// The plus-shaped bedrock layers directly around the gateway block:
	for (const auto & Offset : ENDER_DRAGON_GATEWAY_BEDROCK_OFFSETS)
	{
		SetBlock(Offset, E_BLOCK_BEDROCK);
	}

	// The gateway block itself:
	SetBlock(Vector3i(0, 0, 0), E_BLOCK_END_GATEWAY);
}
