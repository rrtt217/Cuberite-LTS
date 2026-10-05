#include "Globals.h"
#include "Generating/ChunkDesc.h"
#include "Generating/ComposableGenerator.h"
#include "Generating/EnderDragonReturnGatewayGen.h"
#include "../TestHelpers.h"

/** Y of the fake outer-island surface used by the tests. */
static constexpr int TEST_SURFACE_Y = 60;

/** Horizontal distance from the centre at or below which no return gateway may generate. */
static constexpr double TEST_MIN_DISTANCE = 1024.0;

/** Lowest / highest offset of the gateway block above the surface (heightmap + vanilla offset). */
static constexpr int TEST_MIN_GATEWAY_OFFSET = 4;
static constexpr int TEST_MAX_GATEWAY_OFFSET = 10;

/** Keeps the gateway structure away from the chunk border, same as the generator. */
static constexpr int TEST_GATEWAY_RADIUS = 2;





/** Fills the chunk's heightmap with a flat surface, leaving every block as air. */
static void prepareFlatChunk(cChunkDesc & a_ChunkDesc, int a_SurfaceY)
{
	for (int z = 0; z < cChunkDef::Width; z++)
	{
		for (int x = 0; x < cChunkDef::Width; x++)
		{
			a_ChunkDesc.SetHeight(x, z, a_SurfaceY);
		}
	}
}





/** Checks that a gateway block found at (a_X, a_Y, a_Z) is a valid End gateway in an outer chunk. */
static void checkGatewayBlock(const cChunkDesc & a_ChunkDesc, int a_ChunkX, int a_ChunkZ, int a_X, int a_Y, int a_Z, int a_SurfaceY)
{
	// Gateways are only ever placed in the outer islands:
	const int WorldX = (a_ChunkX * cChunkDef::Width) + a_X;
	const int WorldZ = (a_ChunkZ * cChunkDef::Width) + a_Z;
	TEST_TRUE(Vector3d(WorldX, 0, WorldZ).Length() > TEST_MIN_DISTANCE);

	// The whole structure fits inside this chunk:
	TEST_GREATER_THAN_OR_EQUAL(a_X, TEST_GATEWAY_RADIUS);
	TEST_LESS_THAN_OR_EQUAL(a_X, cChunkDef::Width - 1 - TEST_GATEWAY_RADIUS);
	TEST_GREATER_THAN_OR_EQUAL(a_Z, TEST_GATEWAY_RADIUS);
	TEST_LESS_THAN_OR_EQUAL(a_Z, cChunkDef::Width - 1 - TEST_GATEWAY_RADIUS);

	// The gateway block sits above the surface and has bedrock caps above and below:
	TEST_GREATER_THAN_OR_EQUAL(a_Y, a_SurfaceY + TEST_MIN_GATEWAY_OFFSET);
	TEST_LESS_THAN_OR_EQUAL(a_Y, a_SurfaceY + TEST_MAX_GATEWAY_OFFSET);
	TEST_EQUAL(a_ChunkDesc.GetBlockType(a_X, a_Y - 2, a_Z), E_BLOCK_BEDROCK);
	TEST_EQUAL(a_ChunkDesc.GetBlockType(a_X, a_Y + 2, a_Z), E_BLOCK_BEDROCK);
	TEST_EQUAL(a_ChunkDesc.GetBlockType(a_X - 1, a_Y, a_Z), E_BLOCK_AIR);
	TEST_EQUAL(a_ChunkDesc.GetBlockType(a_X + 1, a_Y, a_Z), E_BLOCK_AIR);
}





/** Generates the chunks in the rectangle and returns the number of valid gateway blocks they contain. */
static int generateAndCheck(cFinishGen & a_Finisher, int a_SurfaceY, int a_FromChunkX, int a_ToChunkX, int a_FromChunkZ, int a_ToChunkZ)
{
	int Count = 0;
	for (int chunkX = a_FromChunkX; chunkX <= a_ToChunkX; chunkX++)
	{
		for (int chunkZ = a_FromChunkZ; chunkZ <= a_ToChunkZ; chunkZ++)
		{
			cChunkDesc chd({chunkX, chunkZ});
			prepareFlatChunk(chd, a_SurfaceY);
			a_Finisher.GenFinish(chd);

			// The gateway only ever sits within a few blocks of the surface, so only that band needs scanning:
			const int MaxY = std::min(static_cast<int>(cChunkDef::Height) - 1, a_SurfaceY + TEST_MAX_GATEWAY_OFFSET + 2);
			for (int y = a_SurfaceY; y <= MaxY; y++)
			{
				for (int z = 0; z < cChunkDef::Width; z++)
				{
					for (int x = 0; x < cChunkDef::Width; x++)
					{
						if (chd.GetBlockType(x, y, z) != E_BLOCK_END_GATEWAY)
						{
							continue;
						}

						checkGatewayBlock(chd, chunkX, chunkZ, x, y, z, a_SurfaceY);
						Count++;
					}
				}
			}
		}
	}
	return Count;
}





/** Returns true if every column of the chunk is within TEST_MIN_DISTANCE of the centre. */
static bool isFullyInnerChunk(int a_ChunkX, int a_ChunkZ)
{
	for (int x = 0; x < cChunkDef::Width; x += cChunkDef::Width - 1)
	{
		for (int z = 0; z < cChunkDef::Width; z += cChunkDef::Width - 1)
		{
			const double Distance = Vector3d((a_ChunkX * cChunkDef::Width) + x, 0, (a_ChunkZ * cChunkDef::Width) + z).Length();
			if (Distance > TEST_MIN_DISTANCE)
			{
				return false;
			}
		}
	}
	return true;
}





/** Tests the natural return gateways: they appear on outer-island land and match the structure blueprint. */
static void testReturnGateways(void)
{
	LOG("Testing the End return gateway generator...");

	// A chance of 1 attempts every chunk, so the test is fast and deterministic:
	cEnderDragonReturnGatewayGen Gen(1, 1);
	cFinishGen & Finisher = Gen;

	// In the outer islands every attempted chunk places a valid gateway:
	TEST_GREATER_THAN_OR_EQUAL(generateAndCheck(Finisher, TEST_SURFACE_Y, 65, 80, -5, 5), 1);

	// Chunks fully inside the central island must never contain a gateway:
	int InnerCount = 0;
	for (int chunkX = -12; chunkX <= 12; chunkX++)
	{
		for (int chunkZ = -12; chunkZ <= 12; chunkZ++)
		{
			if (!isFullyInnerChunk(chunkX, chunkZ))
			{
				continue;
			}

			cChunkDesc chd({chunkX, chunkZ});
			prepareFlatChunk(chd, TEST_SURFACE_Y);
			Finisher.GenFinish(chd);
			for (int y = TEST_SURFACE_Y; y <= TEST_SURFACE_Y + TEST_MAX_GATEWAY_OFFSET + 2; y++)
			{
				for (int z = 0; z < cChunkDef::Width; z++)
				{
					for (int x = 0; x < cChunkDef::Width; x++)
					{
						if (chd.GetBlockType(x, y, z) == E_BLOCK_END_GATEWAY)
						{
							InnerCount++;
						}
					}
				}
			}
		}
	}
	TEST_EQUAL(InnerCount, 0);

	// In the void (no surface) no gateway may be placed:
	TEST_EQUAL(generateAndCheck(Finisher, 0, 65, 80, -3, 3), 0);
}





IMPLEMENT_TEST_MAIN("ReturnGatewayTest",
	testReturnGateways();
)
