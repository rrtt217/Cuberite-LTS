// EnderDragonReturnGatewayGen.cpp

// Implements the natural return gateways (end_gateway_return) scattered through the End's outer islands.

#include "Globals.h"

#include "EnderDragonReturnGatewayGen.h"
#include "EndGateway.h"

#include <random>

/** Lowest extra height at which the gateway block is placed above the surface (vanilla offset y min). */
static constexpr int ENDER_DRAGON_RETURN_GATEWAY_MIN_Y_OFFSET = 3;

/** Highest extra height at which the gateway block is placed above the surface (vanilla offset y max). */
static constexpr int ENDER_DRAGON_RETURN_GATEWAY_MAX_Y_OFFSET = 9;

/** Difference between a MOTION_BLOCKING heightmap value and the highest non-air block it describes. */
static constexpr int ENDER_DRAGON_RETURN_GATEWAY_HEIGHTMAP_OFFSET = 1;

/** Horizontal distance from the centre beyond which the outer islands (vanilla's End Highlands) lie. */
static constexpr double ENDER_DRAGON_RETURN_GATEWAY_MIN_DISTANCE = 1024.0;

/** Highest surface Y that still counts as the void instead of land. */
static constexpr int ENDER_DRAGON_RETURN_GATEWAY_MIN_LAND_Y = 15;

/** Half-width of the widest layer of the gateway structure; keeps the structure inside one chunk. */
static constexpr int ENDER_DRAGON_RETURN_GATEWAY_RADIUS = 2;





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





cEnderDragonReturnGatewayGen::cEnderDragonReturnGatewayGen(int a_Seed, int a_Chance) :
	m_Seed(a_Seed),
	m_Chance(a_Chance)
{
	ASSERT(m_Chance > 0);
}





void cEnderDragonReturnGatewayGen::GenFinish(cChunkDesc & a_ChunkDesc)
{
	const auto Coords = a_ChunkDesc.GetChunkCoords();
	auto Rng = MakeChunkRng(m_Seed, Coords.m_ChunkX, Coords.m_ChunkZ);

	// One attempt per chunk; the vanilla rarity_filter uses a 1 in 700 chance:
	if ((Rng() % m_Chance) != 0)
	{
		return;
	}

	// Keep the whole structure inside this chunk, as a finisher can only write to its own chunk:
	const int Range = cChunkDef::Width - (2 * ENDER_DRAGON_RETURN_GATEWAY_RADIUS);
	const int RelX = ENDER_DRAGON_RETURN_GATEWAY_RADIUS + (Rng() % Range);
	const int RelZ = ENDER_DRAGON_RETURN_GATEWAY_RADIUS + (Rng() % Range);

	const int WorldX = (Coords.m_ChunkX * cChunkDef::Width) + RelX;
	const int WorldZ = (Coords.m_ChunkZ * cChunkDef::Width) + RelZ;
	if (Vector3d(WorldX, 0, WorldZ).Length() <= ENDER_DRAGON_RETURN_GATEWAY_MIN_DISTANCE)
	{
		return;
	}

	// The gateway only ever sits on land, never in the void:
	const int SurfaceY = a_ChunkDesc.GetHeight(RelX, RelZ);
	if (SurfaceY < ENDER_DRAGON_RETURN_GATEWAY_MIN_LAND_Y)
	{
		return;
	}

	const int YOffset = ENDER_DRAGON_RETURN_GATEWAY_MIN_Y_OFFSET + (Rng() % (ENDER_DRAGON_RETURN_GATEWAY_MAX_Y_OFFSET - ENDER_DRAGON_RETURN_GATEWAY_MIN_Y_OFFSET + 1));

	const Vector3i GatewayPos(WorldX, SurfaceY + ENDER_DRAGON_RETURN_GATEWAY_HEIGHTMAP_OFFSET + YOffset, WorldZ);
	PlaceEndGatewayStructure(a_ChunkDesc, GatewayPos);
}
