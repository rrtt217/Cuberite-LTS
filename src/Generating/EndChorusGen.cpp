// EndChorusGen.cpp

// Implements the natural chorus trees of the End's outer islands.

#include "Globals.h"

#include "EndChorusGen.h"
#include "ChorusPlantTree.h"

#include <random>

/** Number of chunks in each direction whose terrain is regenerated to catch trees rooted there. */
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





cEndChorusGen::cEndChorusGen(int a_Seed, cBiomeGen & a_BiomeGen, cTerrainShapeGen & a_ShapeGen, cTerrainCompositionGen & a_CompositionGen) :
	m_Seed(a_Seed),
	m_BiomeGen(a_BiomeGen),
	m_ShapeGen(a_ShapeGen),
	m_CompositionGen(a_CompositionGen)
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
				GenerateChunkTrees(Neighbor, a_ChunkDesc, a_ChunkDesc);
				continue;
			}

			// Regenerate the neighbouring terrain so that trees rooted there can be grown here:
			cChunkDesc Worker(Neighbor);
			m_BiomeGen.GenBiomes(Neighbor, Worker.GetBiomeMap());
			cChunkDesc::Shape Shape;
			m_ShapeGen.GenShape(Neighbor, Shape);
			Worker.SetHeightFromShape(Shape);
			m_CompositionGen.ComposeTerrain(Worker, Shape);
			GenerateChunkTrees(Neighbor, Worker, a_ChunkDesc);
		}
	}
	a_ChunkDesc.UpdateHeightmap();
}





void cEndChorusGen::GenerateChunkTrees(const cChunkCoords & a_Coords, cChunkDesc & a_TreeDesc, cChunkDesc & a_Target)
{
	auto Rng = MakeChunkRng(m_Seed, a_Coords.m_ChunkX, a_Coords.m_ChunkZ);
	const int Attempts = static_cast<int>(Rng() % (MAX_TREE_ATTEMPTS + 1));
	for (int i = 0; i < Attempts; i++)
	{
		// Always draw the same number of values, so that skipped attempts do not shift the sequence:
		const int RelX = static_cast<int>(Rng() % cChunkDef::Width);
		const int RelZ = static_cast<int>(Rng() % cChunkDef::Width);
		const int TreeSeed = static_cast<int>(Rng() & 0x7fffffff);

		const int SurfaceY = a_TreeDesc.GetHeight(RelX, RelZ);
		if (SurfaceY < MIN_LAND_Y)
		{
			continue;
		}
		if (a_TreeDesc.GetBlockType(RelX, SurfaceY, RelZ) != E_BLOCK_END_STONE)
		{
			continue;
		}
		if ((SurfaceY + 1 >= cChunkDef::Height) || (a_TreeDesc.GetBlockType(RelX, SurfaceY + 1, RelZ) != E_BLOCK_AIR))
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
