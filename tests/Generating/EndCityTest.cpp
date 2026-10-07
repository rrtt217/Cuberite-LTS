#include "Globals.h"
#include "Generating/ChunkDesc.h"
#include "Generating/ComposableGenerator.h"
#include "Generating/EndCityGen.h"
#include "Generating/EndCityBlueprintData.h"
#include "../TestHelpers.h"

#include <cstring>





/** Height of the fake flat outer-island surface used by the tests. */
static constexpr int TEST_SURFACE_Y = 60;

/** The End City grid cell size, the origin range within it, and the cell size in blocks. */
static constexpr int TEST_CELL_CHUNKS = 20;
static constexpr int TEST_ORIGIN_RANGE_CHUNKS = 9;
static constexpr int TEST_CELL_BLOCKS = TEST_CELL_CHUNKS * cChunkDef::Width;

/** The highest Y that is scanned for city blocks. */
static constexpr int TEST_MAX_SCAN_Y = 160;





/** A height generator with a constant surface plus an optional linear variation. */
class cTestHeightGen:
	public cTerrainHeightGen
{
public:

	cTestHeightGen(int a_Base, int a_Slope):
		m_Base(a_Base),
		m_Slope(a_Slope)
	{
	}

	virtual void GenHeightMap(cChunkCoords, cChunkDef::HeightMap & a_HeightMap) override
	{
		for (size_t i = 0; i < ARRAYCOUNT(a_HeightMap); i++)
		{
			a_HeightMap[i] = static_cast<HEIGHTTYPE>(m_Base);
		}
	}

	virtual HEIGHTTYPE GetHeightAt(int a_BlockX, int a_BlockZ) override
	{
		return static_cast<HEIGHTTYPE>(m_Base + (m_Slope * (a_BlockX % 7)));
	}

private:

	int m_Base;
	int m_Slope;
} ;





/** Returns true if the block type is part of an End City. */
static bool isCityBlock(BLOCKTYPE a_Type)
{
	switch (a_Type)
	{
		case E_BLOCK_AIR:
		case E_BLOCK_PURPUR_BLOCK:
		case E_BLOCK_PURPUR_PILLAR:
		case E_BLOCK_PURPUR_STAIRS:
		case E_BLOCK_PURPUR_SLAB:
		case E_BLOCK_END_BRICKS:
		case E_BLOCK_STAINED_GLASS:
		case E_BLOCK_END_ROD:
		case E_BLOCK_OBSIDIAN:
		case E_BLOCK_HEAD:
		case E_BLOCK_LADDER:
		case E_BLOCK_CHEST:
		case E_BLOCK_ENDER_CHEST:
		case E_BLOCK_BREWING_STAND:
		case E_BLOCK_WALL_BANNER:
		{
			return true;
		}
	}
	return false;
}





/** Generates the chunks in the rectangle and returns the number of non-air city blocks they contain.
Asserts that every non-air block is part of a city. */
static int countCityBlocks(cEndCityGen & a_Gen, int a_FromChunkX, int a_ToChunkX, int a_FromChunkZ, int a_ToChunkZ)
{
	cFinishGen & Finisher = a_Gen;
	int Count = 0;
	for (int chunkX = a_FromChunkX; chunkX <= a_ToChunkX; chunkX++)
	{
		for (int chunkZ = a_FromChunkZ; chunkZ <= a_ToChunkZ; chunkZ++)
		{
			cChunkDesc Chunk({chunkX, chunkZ});
			Finisher.GenFinish(Chunk);
			for (int y = 0; y <= TEST_MAX_SCAN_Y; y++)
			{
				for (int z = 0; z < cChunkDef::Width; z++)
				{
					for (int x = 0; x < cChunkDef::Width; x++)
					{
						const BLOCKTYPE Type = Chunk.GetBlockType(x, y, z);
						if (Type == E_BLOCK_AIR)
						{
							continue;
						}
						TEST_TRUE(isCityBlock(Type));
						Count++;
					}
				}
			}
		}
	}
	return Count;
}





/** Verifies that every chest the city places has a valid facing meta (2 to 5); an invalid meta makes
the chest invisible and removes its collision box on the client. */
static void testChestMeta(void)
{
	LOG("Testing the End City chest facing...");

	cTestHeightGen HeightGen(TEST_SURFACE_Y, 0);
	int ChestCount = 0;
	for (int seed = 1; seed <= 2; seed++)
	{
		cEndCityGen Gen(seed, HeightGen);
		for (int chunkX = 55; chunkX <= 70; chunkX++)
		{
			for (int chunkZ = 55; chunkZ <= 70; chunkZ++)
			{
				cChunkDesc Chunk({chunkX, chunkZ});
				Gen.GenFinish(Chunk);
				for (int y = 0; y <= TEST_MAX_SCAN_Y; y++)
				{
					for (int z = 0; z < cChunkDef::Width; z++)
					{
						for (int x = 0; x < cChunkDef::Width; x++)
						{
							const BLOCKTYPE Type = Chunk.GetBlockType(x, y, z);
							if ((Type != E_BLOCK_CHEST) && (Type != E_BLOCK_ENDER_CHEST))
							{
								continue;
							}
							const NIBBLETYPE Meta = Chunk.GetBlockMeta(x, y, z);
							TEST_GREATER_THAN_OR_EQUAL(Meta, 2);
							TEST_LESS_THAN_OR_EQUAL(Meta, 5);
							ChestCount++;
						}
					}
				}
			}
		}
	}
	TEST_GREATER_THAN_OR_EQUAL(ChestCount, 1);
}





/** Verifies that every ladder the city places is attached to a solid block. A ladder's meta names
the side its support block is on, so a wrong meta leaves the ladder floating. */
static void testLadderAttachment(void)
{
	LOG("Testing the End City ladder attachment...");

	cTestHeightGen HeightGen(TEST_SURFACE_Y, 0);
	int LadderCount = 0;
	for (int seed = 1; seed <= 2; seed++)
	{
		cEndCityGen Gen(seed, HeightGen);
		for (int chunkX = 55; chunkX <= 70; chunkX++)
		{
			for (int chunkZ = 55; chunkZ <= 70; chunkZ++)
			{
				cChunkDesc Chunk({chunkX, chunkZ});
				Gen.GenFinish(Chunk);
				for (int y = 0; y <= TEST_MAX_SCAN_Y; y++)
				{
					for (int z = 0; z < cChunkDef::Width; z++)
					{
						for (int x = 0; x < cChunkDef::Width; x++)
						{
							if (Chunk.GetBlockType(x, y, z) != E_BLOCK_LADDER)
							{
								continue;
							}
							const NIBBLETYPE Meta = Chunk.GetBlockMeta(x, y, z);
							int Dx = 0;
							int Dz = 0;
							// The meta is the direction the ladder faces; it is attached to the opposite neighbour:
							switch (Meta)
							{
								case 2: { Dz = 1; break; }
								case 3: { Dz = -1; break; }
								case 4: { Dx = 1; break; }
								case 5: { Dx = -1; break; }
								default: { TEST_FAIL("Invalid ladder meta"); break; }
							}
							const int Nx = x + Dx;
							const int Nz = z + Dz;
							if ((Nx < 0) || (Nx >= cChunkDef::Width) || (Nz < 0) || (Nz >= cChunkDef::Width))
							{
								continue;
							}
							if (Chunk.GetBlockType(Nx, y, Nz) == E_BLOCK_AIR)
							{
								const auto At = [&Chunk](int rx, int rz, int ry) -> int
								{
									if ((rx < 0) || (rx >= cChunkDef::Width) || (rz < 0) || (rz >= cChunkDef::Width))
									{
										return -1;
									}
									return Chunk.GetBlockType(rx, ry, rz);
								};
								LOG("Ladder chunk (%d, %d) rel (%d, %d, %d) meta %d air; W=%d E=%d N=%d S=%d U=%d D=%d", chunkX, chunkZ, x, y, z, Meta, At(x - 1, z, y), At(x + 1, z, y), At(x, z - 1, y), At(x, z + 1, y), At(x, z, y + 1), At(x, z, y - 1));
							}
							TEST_NOTEQUAL(Chunk.GetBlockType(Nx, y, Nz), E_BLOCK_AIR);
							LadderCount++;
						}
					}
				}
			}
		}
	}
	TEST_GREATER_THAN_OR_EQUAL(LadderCount, 1);
}





/** Verifies the 20-chunk grid rule: every origin chunk lies in the [0 .. 8] range within its cell. */
static void testGridOrigin(void)
{
	LOG("Testing the End City grid origin...");

	const int Cells[] = {0, 320, 640, -320, -640, 1280, -1280};
	for (int seed = 1; seed <= 5; seed++)
	{
		for (const int CellX: Cells)
		{
			for (const int CellZ: Cells)
			{
				const Vector3i Origin = cEndCityGen::GetCellOrigin(seed, CellX, CellZ);
				const int OffsetChunkX = (Origin.x - CellX) / cChunkDef::Width;
				const int OffsetChunkZ = (Origin.z - CellZ) / cChunkDef::Width;
				TEST_GREATER_THAN_OR_EQUAL(OffsetChunkX, 0);
				TEST_LESS_THAN_OR_EQUAL(OffsetChunkX, TEST_ORIGIN_RANGE_CHUNKS - 1);
				TEST_GREATER_THAN_OR_EQUAL(OffsetChunkZ, 0);
				TEST_LESS_THAN_OR_EQUAL(OffsetChunkZ, TEST_ORIGIN_RANGE_CHUNKS - 1);

				// The origin block coords must be congruent to [0 .. 128] modulo 320:
				TEST_LESS_THAN_OR_EQUAL(Origin.x % TEST_CELL_BLOCKS, (TEST_ORIGIN_RANGE_CHUNKS - 1) * cChunkDef::Width);
				TEST_LESS_THAN_OR_EQUAL(Origin.z % TEST_CELL_BLOCKS, (TEST_ORIGIN_RANGE_CHUNKS - 1) * cChunkDef::Width);
			}
		}
	}
}





/** Verifies that a suitable outer island produces a city with only valid blocks. */
static void testGeneration(void)
{
	LOG("Testing the End City generation...");

	cTestHeightGen HeightGen(TEST_SURFACE_Y, 0);
	cEndCityGen Gen(1, HeightGen);
	TEST_GREATER_THAN_OR_EQUAL(countCityBlocks(Gen, 55, 70, 55, 70), 1);
}





/** Verifies that neither the void, nor the central island, nor uneven ground produces a city. */
static void testRejectedLocations(void)
{
	LOG("Testing the End City placement gates...");

	// The void (surface below the minimum land Y):
	{
		cTestHeightGen HeightGen(0, 0);
		cEndCityGen Gen(1, HeightGen);
		TEST_EQUAL(countCityBlocks(Gen, 55, 70, 55, 70), 0);
	}

	// The central island (within 1024 blocks of the centre):
	{
		cTestHeightGen HeightGen(TEST_SURFACE_Y, 0);
		cEndCityGen Gen(1, HeightGen);
		TEST_EQUAL(countCityBlocks(Gen, -3, 3, -3, 3), 0);
	}

	// Uneven ground (height difference greater than the allowed maximum):
	{
		cTestHeightGen HeightGen(TEST_SURFACE_Y, 1);
		cEndCityGen Gen(1, HeightGen);
		TEST_EQUAL(countCityBlocks(Gen, 55, 70, 55, 70), 0);
	}
}





/** Verifies that two generators with the same seed produce identical chunks. */
static void testDeterminism(void)
{
	LOG("Testing the End City determinism...");

	cTestHeightGen HeightGen(TEST_SURFACE_Y, 0);

	cEndCityGen GenA(1, HeightGen);
	cChunkDesc ChunkA({60, 60});
	GenA.GenFinish(ChunkA);

	// A second generator must regenerate the very same structure:
	cEndCityGen GenB(1, HeightGen);
	cChunkDesc ChunkB({60, 60});
	GenB.GenFinish(ChunkB);

	TEST_EQUAL(memcmp(ChunkA.GetBlockTypes(), ChunkB.GetBlockTypes(), sizeof(cChunkDef::BlockTypes)), 0);
}





/** Measures the horizontal extent of a blueprint layer's non-air content. */
static bool layerExtent(const sEndCityBlueprintLayer & a_Layer, int & a_MinX, int & a_MaxX, int & a_MinZ, int & a_MaxZ)
{
	a_MinX = a_MaxX = a_MinZ = a_MaxZ = -1;
	const auto Rows = StringSplit(a_Layer.m_Rows, "|");
	for (size_t z = 0; z < Rows.size(); z++)
	{
		for (size_t x = 0; x < Rows[z].size(); x++)
		{
			const char c = Rows[z][x];
			if ((c == ' ') || (c == '.') || (c == 'Y') || (c == 'N') || (c == 'B'))
			{
				continue;
			}
			if (a_MinX < 0) { a_MinX = static_cast<int>(x); }
			a_MaxX = static_cast<int>(x);
			if (a_MinZ < 0) { a_MinZ = static_cast<int>(z); }
			a_MaxZ = static_cast<int>(z);
		}
	}
	return (a_MinX >= 0);
}





/** Verifies that a room's roof overhangs its body symmetrically on both axes; the English blueprints
of the loot room and the fat tower top draw the body one block off centre, which shows in game as a
roof hanging three blocks over one edge and one over the opposite one. */
static void testRoomRoofCentered(void)
{
	LOG("Testing the End City room roof centring...");

	for (const AString Name: {"LootRoom1", "FatTowerTop"})
	{
		const sEndCityBlueprint * Blueprint = nullptr;
		for (int i = 0; i < g_NumEndCityBlueprints; i++)
		{
			if (Name == g_EndCityBlueprints[i].m_Name)
			{
				Blueprint = &g_EndCityBlueprints[i];
				break;
			}
		}
		TEST_TRUE(Blueprint != nullptr);

		// The roof is the layer with the most content; the body is the one just below it:
		int RoofIndex = 0;
		int RoofCount = -1;
		for (int i = 0; i < Blueprint->m_Height; i++)
		{
			int mnx, mxx, mnz, mxz;
			if (!layerExtent(Blueprint->m_Layers[i], mnx, mxx, mnz, mxz))
			{
				continue;
			}
			const int Count = (mxx - mnx + 1) * (mxz - mnz + 1);
			if (Count > RoofCount)
			{
				RoofCount = Count;
				RoofIndex = i;
			}
		}
		TEST_GREATER_THAN_OR_EQUAL(RoofIndex, 1);

		int rMnx, rMxx, rMnz, rMxz;
		TEST_TRUE(layerExtent(Blueprint->m_Layers[RoofIndex], rMnx, rMxx, rMnz, rMxz));
		int bMnx, bMxx, bMnz, bMxz;
		TEST_TRUE(layerExtent(Blueprint->m_Layers[RoofIndex - 1], bMnx, bMxx, bMnz, bMxz));

		TEST_EQUAL(bMnx - rMnx, rMxx - bMxx);
		TEST_EQUAL(bMnz - rMnz, rMxz - bMxz);
	}
}





IMPLEMENT_TEST_MAIN("EndCityTest",
	testGridOrigin();
	testGeneration();
	testChestMeta();
	testLadderAttachment();
	testRoomRoofCentered();
	testRejectedLocations();
	testDeterminism();
)
