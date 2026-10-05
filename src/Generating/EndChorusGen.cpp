// EndChorusGen.cpp

// Implements the natural chorus trees of the End's outer islands.

#include "Globals.h"

#include "EndChorusGen.h"
#include "ChorusPlantTree.h"

#include <random>

/** Number of chunks in each direction whose trees may reach into the current chunk. */
static constexpr int CHORUS_GEN_NEIGHBORHOOD = 1;





/** Returns a deterministic pseudo-random engine for one chunk.
std::minstd_rand is a standard LCG, so the placement is identical on every platform. */
static std::minstd_rand MakeChunkRng(int a_Seed, int a_ChunkX, int a_ChunkZ)
{
	// Mix the chunk coordinates into the world seed:
	const UInt32 Mixed = static_cast<UInt32>(a_Seed)
		^ (static_cast<UInt32>(a_ChunkX) * 73856093u)
		^ (static_cast<UInt32>(a_ChunkZ) * 19349663u);
	return std::minstd_rand(Mixed);
}





cEndChorusGen::cEndChorusGen(int a_Seed, cTerrainHeightGen & a_HeightGen) :
	m_Seed(a_Seed),
	m_HeightGen(a_HeightGen)
{
}





void cEndChorusGen::GenFinish(cChunkDesc & a_ChunkDesc)
{
	const auto Coords = a_ChunkDesc.GetChunkCoords();
	for (int x = -CHORUS_GEN_NEIGHBORHOOD; x <= CHORUS_GEN_NEIGHBORHOOD; x++)
	{
		for (int z = -CHORUS_GEN_NEIGHBORHOOD; z <= CHORUS_GEN_NEIGHBORHOOD; z++)
		{
			const cChunkCoords Neighbor(Coords.m_ChunkX + x, Coords.m_ChunkZ + z);
			if ((x == 0) && (z == 0))
			{
				// The current chunk already has its heightmap:
				GenerateChunkTrees(Neighbor, a_ChunkDesc.GetHeightMap(), a_ChunkDesc);
				continue;
			}

			// The attempt count does not depend on the heights, so skip chunks without any trees
			// before asking the shared composited-heightmap cache:
			auto Rng = MakeChunkRng(m_Seed, Neighbor.m_ChunkX, Neighbor.m_ChunkZ);
			if ((Rng() % (MAX_TREE_ATTEMPTS + 1)) == 0)
			{
				continue;
			}

			cChunkDef::HeightMap NeighborHeights;
			m_HeightGen.GenHeightMap(Neighbor, NeighborHeights);
			GenerateChunkTrees(Neighbor, NeighborHeights, a_ChunkDesc);
		}
	}
	a_ChunkDesc.UpdateHeightmap();
}





void cEndChorusGen::GenerateChunkTrees(const cChunkCoords & a_Coords, const cChunkDef::HeightMap & a_HeightMap, cChunkDesc & a_Target)
{
	auto Rng = MakeChunkRng(m_Seed, a_Coords.m_ChunkX, a_Coords.m_ChunkZ);
	const int Attempts = static_cast<int>(Rng() % (MAX_TREE_ATTEMPTS + 1));
	for (int i = 0; i < Attempts; i++)
	{
		// Always draw the same number of values, so that skipped attempts do not shift the sequence:
		const int RelX = static_cast<int>(Rng() % cChunkDef::Width);
		const int RelZ = static_cast<int>(Rng() % cChunkDef::Width);
		const int TreeSeed = static_cast<int>(Rng() & 0x7fffffff);

		// The End's surface is End stone, so the heightmap's top block is a valid soil:
		const int SurfaceY = cChunkDef::GetHeight(a_HeightMap, RelX, RelZ);
		if (SurfaceY < MIN_LAND_Y)
		{
			continue;
		}
		if (SurfaceY + 1 >= cChunkDef::Height)
		{
			continue;
		}

		const int WorldX = a_Coords.m_ChunkX * cChunkDef::Width + RelX;
		const int WorldZ = a_Coords.m_ChunkZ * cChunkDef::Width + RelZ;
		if (Vector3d(WorldX, 0, WorldZ).Length() <= MIN_DISTANCE)
		{
			continue;
		}

		const Vector3i Base(WorldX, SurfaceY + 1, WorldZ);
		ApplyImage(a_Target, cChorusPlantTree::Generate(Base, TreeSeed));
	}
}





void cEndChorusGen::ApplyImage(cChunkDesc & a_Target, const sSetBlockVector & a_Image)
{
	for (const auto & Block : a_Image)
	{
		if ((Block.m_ChunkX != a_Target.GetChunkX()) || (Block.m_ChunkZ != a_Target.GetChunkZ()))
		{
			continue;
		}
		if ((Block.m_RelY < 0) || (Block.m_RelY >= cChunkDef::Height))
		{
			continue;
		}
		if (a_Target.GetBlockType(Block.m_RelX, Block.m_RelY, Block.m_RelZ) != E_BLOCK_AIR)
		{
			continue;
		}
		a_Target.SetBlockTypeMeta(Block.m_RelX, Block.m_RelY, Block.m_RelZ, Block.m_BlockType, Block.m_BlockMeta);
	}
}
