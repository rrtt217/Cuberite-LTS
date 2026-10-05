#include "Globals.h"
#include "Generating/BioGen.h"
#include "Generating/ChorusPlantTree.h"
#include "Generating/ChunkDesc.h"
#include "Generating/CompositedHeiGen.h"
#include "Generating/EndChorusGen.h"
#include "Generating/EndGen.h"
#include "Generating/HeiGen.h"
#include "Blocks/ChorusRules.h"
#include "../TestHelpers.h"

#include <algorithm>
#include <map>

/** The position at which the test trees are rooted. */
static const Vector3i TREE_BASE(1000, 64, 1000);





/** One block of a generated tree, keyed by its world position. */
struct sTestTreeBlock
{
	BLOCKTYPE m_Type;
	NIBBLETYPE m_Meta;
};





/** Rebuilds the tree's block map from its image. */
static std::map<Vector3i, sTestTreeBlock> MakeTreeMap(const sSetBlockVector & a_Blocks)
{
	std::map<Vector3i, sTestTreeBlock> Result;
	for (const auto & Block : a_Blocks)
	{
		const Vector3i Pos(Block.m_ChunkX * cChunkDef::Width + Block.m_RelX, Block.m_RelY, Block.m_ChunkZ * cChunkDef::Width + Block.m_RelZ);
		Result[Pos] = { Block.m_BlockType, Block.m_BlockMeta };
	}
	return Result;
}





/** Returns the type of a tree block, or air if there is none. */
static BLOCKTYPE GetTreeType(const std::map<Vector3i, sTestTreeBlock> & a_Map, const Vector3i & a_Pos)
{
	const auto Itr = a_Map.find(a_Pos);
	return (Itr == a_Map.end()) ? static_cast<BLOCKTYPE>(E_BLOCK_AIR) : Itr->second.m_Type;
}





/** Returns the type below a tree block; the whole ground plane below the base counts as End stone. */
static BLOCKTYPE GetTreeBelow(const std::map<Vector3i, sTestTreeBlock> & a_Map, const Vector3i & a_Pos)
{
	if (a_Pos.y == (TREE_BASE.y - 1))
	{
		return static_cast<BLOCKTYPE>(E_BLOCK_END_STONE);
	}
	return GetTreeType(a_Map, a_Pos);
}





/** Checks that every generated tree is a valid, grounded chorus tree. */
static void testTreeGeneration(void)
{
	int MaxHeight = 0;
	int TotalPlants = 0;
	int TotalFlowers = 0;
	for (int Seed = 0; Seed < 500; Seed++)
	{
		const auto Image = cChorusPlantTree::Generate(TREE_BASE, Seed);
		TEST_TRUE(!Image.empty());

		const auto Tree = MakeTreeMap(Image);
		TEST_TRUE(Tree.count(TREE_BASE) == 1);

		int MinY = 1000;
		int MaxY = -1000;
		for (const auto & Block : Tree)
		{
			const Vector3i & Pos = Block.first;
			const BLOCKTYPE Type = Block.second.m_Type;
			TEST_TRUE(((Type == E_BLOCK_CHORUS_PLANT) || (Type == E_BLOCK_CHORUS_FLOWER)));
			MinY = std::min(MinY, Pos.y);
			MaxY = std::max(MaxY, Pos.y);

			if (Type == E_BLOCK_CHORUS_PLANT)
			{
				TotalPlants++;

				sChorusPlantNeighborhood N;
				N.m_Up = GetTreeType(Tree, Pos.addedY(1));
				N.m_Down = GetTreeBelow(Tree, Pos.addedY(-1));
				N.m_North = GetTreeType(Tree, Pos.addedZ(-1));
				N.m_South = GetTreeType(Tree, Pos.addedZ(1));
				N.m_West = GetTreeType(Tree, Pos.addedX(-1));
				N.m_East = GetTreeType(Tree, Pos.addedX(1));
				N.m_NorthBelow = GetTreeBelow(Tree, Pos.addedZ(-1).addedY(-1));
				N.m_SouthBelow = GetTreeBelow(Tree, Pos.addedZ(1).addedY(-1));
				N.m_WestBelow = GetTreeBelow(Tree, Pos.addedX(-1).addedY(-1));
				N.m_EastBelow = GetTreeBelow(Tree, Pos.addedX(1).addedY(-1));
				TEST_TRUE(ChorusPlantCanSurvive(N));
			}
			else
			{
				TotalFlowers++;

				// Naturally generated flowers are always dead (age 5):
				TEST_TRUE(Block.second.m_Meta == 5);
			}
		}

		TEST_TRUE(MinY == TREE_BASE.y);
		MaxHeight = std::max(MaxHeight, MaxY - TREE_BASE.y + 1);
	}

	LOG("Chorus tree test: max height %d, %d plants, %d flowers", MaxHeight, TotalPlants, TotalFlowers);

	// The wiki states a chorus tree reaches at most 22 blocks:
	TEST_LESS_THAN_OR_EQUAL(MaxHeight, 22);
}





/** Runs the finisher over a block of End chunks and checks that it produces chorus blocks without crashing. */
static void testFinisherSmoke(void)
{
	cEndGen EndGen(12345);
	cBioGenConstant BiomeGen;
	cHeiGenMultiCache HeightCache(std::make_unique<cCompositedHeiGen>(BiomeGen, EndGen, EndGen), 16, 128);
	cEndChorusGen Finisher(12345, HeightCache);

	int TotalChorus = 0;
	for (int ChunkX = 64; ChunkX < 72; ChunkX++)
	{
		for (int ChunkZ = 64; ChunkZ < 72; ChunkZ++)
		{
			const cChunkCoords Coords(ChunkX, ChunkZ);
			cChunkDesc Desc(Coords);
			cChunkDesc::Shape Shape;
			cTerrainShapeGen & ShapeGen = EndGen;
			ShapeGen.GenShape(Coords, Shape);
			Desc.SetHeightFromShape(Shape);
			cTerrainCompositionGen & CompGen = EndGen;
			CompGen.ComposeTerrain(Desc, Shape);

			// GenFinish is protected in the derived class, but public in the cFinishGen interface:
			cFinishGen & Finish = Finisher;
			Finish.GenFinish(Desc);

			for (int Y = 0; Y < cChunkDef::Height; Y++)
			{
				for (int Z = 0; Z < cChunkDef::Width; Z++)
				{
					for (int X = 0; X < cChunkDef::Width; X++)
					{
						const BLOCKTYPE Type = Desc.GetBlockType(X, Y, Z);
						if ((Type == E_BLOCK_CHORUS_PLANT) || (Type == E_BLOCK_CHORUS_FLOWER))
						{
							TotalChorus++;
						}
					}
				}
			}
		}
	}

	LOG("Chorus finisher smoke: %d chorus blocks in 64 chunks", TotalChorus);
	TEST_GREATER_THAN_OR_EQUAL(TotalChorus, 1);
}





IMPLEMENT_TEST_MAIN("ChorusTreeTest",
	testTreeGeneration();
	testFinisherSmoke();
)
