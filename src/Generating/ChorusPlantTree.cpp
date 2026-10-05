// ChorusPlantTree.cpp

// Implements the natural chorus tree builder.

#include "Globals.h"

#include "ChorusPlantTree.h"
#include "../Blocks/ChorusRules.h"

#include <map>
#include <queue>
#include <random>





namespace
{
	/** Age at which a chorus flower stops growing. */
	constexpr NIBBLETYPE MAX_AGE = 5;

	/** How far down the structure below a flower is inspected. */
	constexpr int MAX_BELOW_SCAN = 32;

	/** The four horizontal directions a flower may branch into. */
	constexpr Vector3i HORIZONTAL_DIRECTIONS[4] =
	{
		Vector3i(-1, 0, 0),
		Vector3i(1, 0, 0),
		Vector3i(0, 0, -1),
		Vector3i(0, 0, 1),
	};

	/** One block of the tree being built. */
	struct sTreeBlock
	{
		BLOCKTYPE m_Type;
		NIBBLETYPE m_Meta;
	};

	/** A flower that still has to be grown. */
	struct sTreeFlower
	{
		Vector3i m_Pos;
		NIBBLETYPE m_Age;
	};
}





sSetBlockVector cChorusPlantTree::Generate(const Vector3i & a_Base, int a_Seed)
{
	std::map<Vector3i, sTreeBlock> Blocks;
	std::queue<sTreeFlower> Queue;
	std::minstd_rand Rng(static_cast<std::minstd_rand::result_type>(static_cast<unsigned>(a_Seed)));
	std::uniform_real_distribution<float> RealDist(0.0f, 1.0f);
	std::uniform_int_distribution<int> IntDist(0, 3);

	auto IsTreePlant = [&Blocks](const Vector3i & a_Pos)
	{
		const auto Itr = Blocks.find(a_Pos);
		return (Itr != Blocks.end()) && (Itr->second.m_Type == E_BLOCK_CHORUS_PLANT);
	};
	auto IsAir = [&Blocks](const Vector3i & a_Pos) { return Blocks.find(a_Pos) == Blocks.end(); };

	auto IsClearForUpward = [&IsAir](const Vector3i & a_Target)
	{
		return
			IsAir(a_Target) &&
			IsAir(a_Target.addedY(1)) &&
			IsAir(a_Target.addedX(-1)) &&
			IsAir(a_Target.addedX(1)) &&
			IsAir(a_Target.addedZ(-1)) &&
			IsAir(a_Target.addedZ(1));
	};
	auto IsClearForBranch = [&IsAir](const Vector3i & a_Target, const Vector3i & a_Direction)
	{
		if (!IsAir(a_Target) || !IsAir(a_Target.addedY(-1)))
		{
			return false;
		}

		// The three horizontal neighbours other than the flower itself must be clear:
		const Vector3i FlowerPos = a_Target - a_Direction;
		for (const auto & Direction : HORIZONTAL_DIRECTIONS)
		{
			const Vector3i Neighbor = a_Target + Direction;
			if ((Neighbor == FlowerPos) || IsAir(Neighbor))
			{
				continue;
			}
			return false;
		}
		return true;
	};

	// Start from a single flower on End stone:
	const Vector3i Ground = a_Base.addedY(-1);
	Blocks[a_Base] = { E_BLOCK_CHORUS_FLOWER, 0 };
	Queue.push({ a_Base, 0 });

	while (!Queue.empty())
	{
		const sTreeFlower Flower = Queue.front();
		Queue.pop();
		if (Flower.m_Age >= MAX_AGE)
		{
			continue;
		}

		// If the flower is blocked, it never grows (and is left as a dead natural flower):
		if (!IsAir(Flower.m_Pos.addedY(1)))
		{
			Blocks[Flower.m_Pos] = { E_BLOCK_CHORUS_FLOWER, MAX_AGE };
			continue;
		}

		// Inspect the column directly below the flower:
		int PlantCount = 0;
		bool Grounded = false;
		for (int Offset = 1; Offset <= MAX_BELOW_SCAN; Offset++)
		{
			const Vector3i Below = Flower.m_Pos.addedY(-Offset);
			if (IsTreePlant(Below))
			{
				PlantCount++;
				continue;
			}
			Grounded = (Below == Ground);
			break;
		}

		const int ChanceIndex = std::min(PlantCount, 4);
		const float UpwardChance = Grounded ? CHORUS_UPWARD_GROUNDED[ChanceIndex] : CHORUS_UPWARD_BRANCH[ChanceIndex];
		bool Grew = false;

		// Try to grow upwards first:
		if (RealDist(Rng) < UpwardChance)
		{
			const Vector3i Target = Flower.m_Pos.addedY(1);
			if (IsClearForUpward(Target))
			{
				Blocks[Target] = { E_BLOCK_CHORUS_FLOWER, Flower.m_Age };
				Blocks[Flower.m_Pos] = { E_BLOCK_CHORUS_PLANT, 0 };
				Queue.push({ Target, Flower.m_Age });
				Grew = true;
			}
		}

		// If growing upwards did not happen and the flower is young enough, try to branch sideways:
		if (!Grew && (Flower.m_Age <= 3))
		{
			const bool ExtraAttempt = Grounded && (PlantCount >= 1);
			const int Attempts = IntDist(Rng) + (ExtraAttempt ? 1 : 0);
			for (int i = 0; i < Attempts; i++)
			{
				const Vector3i Direction = HORIZONTAL_DIRECTIONS[IntDist(Rng)];
				const Vector3i Target = Flower.m_Pos + Direction;
				if (!IsClearForBranch(Target, Direction))
				{
					continue;
				}
				const NIBBLETYPE NewAge = static_cast<NIBBLETYPE>(Flower.m_Age + 1);
				Blocks[Target] = { E_BLOCK_CHORUS_FLOWER, NewAge };
				Queue.push({ Target, NewAge });
				Grew = true;
			}
			if (Grew)
			{
				Blocks[Flower.m_Pos] = { E_BLOCK_CHORUS_PLANT, 0 };
			}
		}

		if (!Grew)
		{
			// No growth at all: the flower withers:
			Blocks[Flower.m_Pos] = { E_BLOCK_CHORUS_FLOWER, MAX_AGE };
		}
	}

	sSetBlockVector Result;
	Result.reserve(Blocks.size());
	for (const auto & Block : Blocks)
	{
		Result.emplace_back(Block.first, Block.second.m_Type, Block.second.m_Meta);
	}
	return Result;
}
