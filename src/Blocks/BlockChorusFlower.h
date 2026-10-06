// BlockChorusFlower.h

#pragma once

#include "BlockHandler.h"
#include "ChorusRules.h"
#include "../FastRandom.h"

#include <algorithm>
#include <array>





/** Handler for the chorus flower block.
A flower grows the chorus plant structure upwards and sideways, and withers once it reaches age 5. */
class cChorusFlowerHandler final :
	public cBlockHandler
{
	using Super = cBlockHandler;

public:

	using Super::Super;

private:

	/** Age at which a chorus flower stops growing (it then appears purple / withered). */
	static constexpr NIBBLETYPE MAX_AGE = 5;

	/** How far down the structure below a flower is inspected when picking the growth chance. */
	static constexpr int MAX_BELOW_SCAN = 32;

	/** The four horizontal directions a flower may branch into. */
	static constexpr std::array<Vector3i, 4> HorizontalDirections =
	{
		Vector3i(-1, 0, 0),
		Vector3i(1, 0, 0),
		Vector3i(0, 0, -1),
		Vector3i(0, 0, 1),
	};

	virtual bool CanBeAt(const cChunk & a_Chunk, const Vector3i a_Position, const NIBBLETYPE a_Meta) const override
	{
		UNUSED(a_Meta);

		int PlantCount = 0;
		if (GetBlockAt(a_Chunk, a_Position.addedX(-1)) == E_BLOCK_CHORUS_PLANT)
		{
			PlantCount++;
		}
		if (GetBlockAt(a_Chunk, a_Position.addedX(1)) == E_BLOCK_CHORUS_PLANT)
		{
			PlantCount++;
		}
		if (GetBlockAt(a_Chunk, a_Position.addedZ(-1)) == E_BLOCK_CHORUS_PLANT)
		{
			PlantCount++;
		}
		if (GetBlockAt(a_Chunk, a_Position.addedZ(1)) == E_BLOCK_CHORUS_PLANT)
		{
			PlantCount++;
		}

		return ChorusFlowerCanSurvive(
			GetBlockAt(a_Chunk, a_Position.addedY(-1)),
			GetBlockAt(a_Chunk, a_Position.addedY(1)),
			PlantCount
		);
	}





	virtual void OnNeighborChanged(cChunkInterface & a_ChunkInterface, Vector3i a_BlockPos, eBlockFace a_WhichNeighbor) const override
	{
		UNUSED(a_WhichNeighbor);

		// Unlike most blocks, a flower that loses its support breaks without dropping anything:
		if (a_ChunkInterface.DoWithChunkAt(a_BlockPos, [&](cChunk & a_Chunk)
			{
				const Vector3i RelPos = cChunkDef::AbsoluteToRelative(a_BlockPos, a_Chunk.GetPos());
				return CanBeAt(a_Chunk, RelPos, a_Chunk.GetMeta(RelPos));
			}))
		{
			return;
		}
		a_ChunkInterface.SetBlock(a_BlockPos, E_BLOCK_AIR, 0);
	}





	virtual cItems ConvertToPickups(const NIBBLETYPE a_BlockMeta, const cItem * const a_Tool) const override
	{
		UNUSED(a_BlockMeta);
		UNUSED(a_Tool);

		// A flower drops itself; its age does not matter for the item:
		return cItem(E_BLOCK_CHORUS_FLOWER, 1, 0);
	}





	virtual void OnUpdate(
		cChunkInterface & a_ChunkInterface,
		cWorldInterface & a_WorldInterface,
		cBlockPluginInterface & a_PluginInterface,
		cChunk & a_Chunk,
		const Vector3i a_RelPos
	) const override
	{
		UNUSED(a_WorldInterface);
		UNUSED(a_PluginInterface);

		const NIBBLETYPE Age = a_Chunk.GetMeta(a_RelPos);
		if (Age >= MAX_AGE)
		{
			return;
		}

		const Vector3i AbsPos = a_Chunk.RelativeToAbsolute(a_RelPos);

		// The flower only grows when there is room above; a blocked flower does not even age:
		if (GetBlock(a_ChunkInterface, AbsPos.addedY(1)) != E_BLOCK_AIR)
		{
			return;
		}

		// Inspect the column directly below the flower to pick the growth chance:
		int PlantCount = 0;
		bool Grounded = false;
		for (int Offset = 1; Offset <= MAX_BELOW_SCAN; Offset++)
		{
			const BLOCKTYPE Below = GetBlock(a_ChunkInterface, AbsPos.addedY(-Offset));
			if (Below == E_BLOCK_CHORUS_PLANT)
			{
				PlantCount++;
				continue;
			}
			Grounded = (Below == E_BLOCK_END_STONE);
			break;
		}

		// The upward growth chance depends on the structure below; see the chorus plant specification:
		static constexpr float GroundedChances[] = { 1.0f, 1.0f, 0.6f, 0.4f, 0.2f };
		static constexpr float BranchChances[]   = { 1.0f, 1.0f, 0.5f, 0.25f, 0.0f };
		const int ChanceIndex = std::min(PlantCount, 4);
		const float UpwardChance = Grounded ? GroundedChances[ChanceIndex] : BranchChances[ChanceIndex];

		auto & Random = GetRandomProvider();
		bool Grew = false;

		// Try to grow upwards first:
		if (Random.RandReal() < UpwardChance)
		{
			const Vector3i Target = AbsPos.addedY(1);
			if (IsClearForUpwardGrowth(a_ChunkInterface, Target))
			{
				a_ChunkInterface.SetBlock(Target, E_BLOCK_CHORUS_FLOWER, Age);
				a_ChunkInterface.SetBlock(AbsPos, E_BLOCK_CHORUS_PLANT, 0);
				Grew = true;
			}
		}

		// If growing upwards did not happen and the flower is young enough, try to branch sideways:
		if (!Grew && (Age <= 3))
		{
			const bool ExtraAttempt = Grounded && (PlantCount >= 1);
			const int Attempts = Random.RandInt(3) + (ExtraAttempt ? 1 : 0);
			for (int i = 0; i < Attempts; i++)
			{
				const Vector3i Direction = HorizontalDirections[static_cast<size_t>(Random.RandInt(3))];
				const Vector3i Target = AbsPos + Direction;
				if (IsClearForBranch(a_ChunkInterface, Target, Direction))
				{
					a_ChunkInterface.SetBlock(Target, E_BLOCK_CHORUS_FLOWER, Age + 1);
					Grew = true;
				}
			}
			if (Grew)
			{
				a_ChunkInterface.SetBlock(AbsPos, E_BLOCK_CHORUS_PLANT, 0);
			}
		}

		const Vector3d SoundPos(AbsPos.x + 0.5, AbsPos.y + 0.5, AbsPos.z + 0.5);
		if (Grew)
		{
			a_Chunk.GetWorld()->BroadcastSoundEffect("block.chorus_flower.grow", SoundPos, 1.0f, 1.0f);
		}
		else
		{
			// No growth at all: the flower withers:
			a_ChunkInterface.SetBlockMeta(AbsPos, MAX_AGE);
			a_Chunk.GetWorld()->BroadcastSoundEffect("block.chorus_flower.death", SoundPos, 1.0f, 1.0f);
		}
	}





	/** Returns the block at the given absolute position, or air if it is outside the world. */
	static BLOCKTYPE GetBlock(cChunkInterface & a_ChunkInterface, const Vector3i & a_AbsPos)
	{
		return cChunkDef::IsValidHeight(a_AbsPos) ? a_ChunkInterface.GetBlock(a_AbsPos) : static_cast<BLOCKTYPE>(E_BLOCK_AIR);
	}





	/** Returns the block at the given chunk-relative position, or air if it cannot be resolved. */
	static BLOCKTYPE GetBlockAt(const cChunk & a_Chunk, const Vector3i a_RelPos)
	{
		BLOCKTYPE Block = E_BLOCK_AIR;
		a_Chunk.UnboundedRelGetBlockType(a_RelPos, Block);
		return Block;
	}





	/** Returns whether the target block has clearance for a new flower above it (vanilla upward growth check). */
	static bool IsClearForUpwardGrowth(cChunkInterface & a_ChunkInterface, const Vector3i & a_Target)
	{
		return
			(a_ChunkInterface.GetBlock(a_Target) == E_BLOCK_AIR) &&
			(GetBlock(a_ChunkInterface, a_Target.addedY(1)) == E_BLOCK_AIR) &&
			(GetBlock(a_ChunkInterface, a_Target.addedX(-1)) == E_BLOCK_AIR) &&
			(GetBlock(a_ChunkInterface, a_Target.addedX(1)) == E_BLOCK_AIR) &&
			(GetBlock(a_ChunkInterface, a_Target.addedZ(-1)) == E_BLOCK_AIR) &&
			(GetBlock(a_ChunkInterface, a_Target.addedZ(1)) == E_BLOCK_AIR);
	}





	/** Returns whether the target block has clearance for a new side branch (vanilla branching check). */
	static bool IsClearForBranch(cChunkInterface & a_ChunkInterface, const Vector3i & a_Target, const Vector3i & a_Direction)
	{
		if (a_ChunkInterface.GetBlock(a_Target) != E_BLOCK_AIR)
		{
			return false;
		}
		if (GetBlock(a_ChunkInterface, a_Target.addedY(-1)) != E_BLOCK_AIR)
		{
			return false;
		}

		// The three horizontal neighbours other than the flower itself must be clear:
		const Vector3i FlowerPos = a_Target - a_Direction;
		return
			((a_Target.addedX(-1) == FlowerPos) || (GetBlock(a_ChunkInterface, a_Target.addedX(-1)) == E_BLOCK_AIR)) &&
			((a_Target.addedX(1)  == FlowerPos) || (GetBlock(a_ChunkInterface, a_Target.addedX(1))  == E_BLOCK_AIR)) &&
			((a_Target.addedZ(-1) == FlowerPos) || (GetBlock(a_ChunkInterface, a_Target.addedZ(-1)) == E_BLOCK_AIR)) &&
			((a_Target.addedZ(1)  == FlowerPos) || (GetBlock(a_ChunkInterface, a_Target.addedZ(1))  == E_BLOCK_AIR));
	}
};
