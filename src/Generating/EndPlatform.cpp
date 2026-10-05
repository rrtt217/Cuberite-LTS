#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "EndPlatform.h"
#include "../World.h"





/** Centre of the End's arrival platform (its base block). */
static constexpr int END_PLATFORM_X = 100;

/** Y of the End's arrival platform. */
static constexpr int END_PLATFORM_Y = 48;

/** Z of the End's arrival platform. */
static constexpr int END_PLATFORM_Z = 0;

/** Half-size of the square obsidian platform. */
static constexpr int END_PLATFORM_RADIUS = 2;

/** Number of blocks above the platform cleared when it is (re)generated. */
static constexpr int END_PLATFORM_CLEAR_HEIGHT = 3;





/** Places the part of the platform that lives in one chunk, once that chunk is ready. */
class cEndPlatformCallback:
	public cChunkCoordCallback
{
public:

	cEndPlatformCallback(cWorld * a_World):
		m_World(a_World)
	{
	}

	virtual void Call(cChunkCoords a_Coords, bool a_IsSuccess) override
	{
		if (!a_IsSuccess)
		{
			LOGD("Couldn't prepare chunk (%d, %d) for the end obsidian platform!", a_Coords.m_ChunkX, a_Coords.m_ChunkZ);
			return;
		}

		for (int x = -END_PLATFORM_RADIUS; x <= END_PLATFORM_RADIUS; x++)
		{
			for (int z = -END_PLATFORM_RADIUS; z <= END_PLATFORM_RADIUS; z++)
			{
				const Vector3i PlatformPos(END_PLATFORM_X + x, END_PLATFORM_Y, END_PLATFORM_Z + z);
				const auto Chunk = cChunkDef::BlockToChunk(PlatformPos);
				if ((Chunk.m_ChunkX != a_Coords.m_ChunkX) || (Chunk.m_ChunkZ != a_Coords.m_ChunkZ))
				{
					continue;  // The other chunk's callback handles this column
				}

				m_World->FastSetBlock(PlatformPos, E_BLOCK_OBSIDIAN, 0);

				// Clear the blocks above the platform, dropping non-end-stone ones as pickups:
				for (int y = 1; y <= END_PLATFORM_CLEAR_HEIGHT; y++)
				{
					const Vector3i ClearPos = PlatformPos.addedY(y);
					const BLOCKTYPE ClearType = m_World->GetBlock(ClearPos);
					if (ClearType == E_BLOCK_AIR)
					{
						continue;
					}

					if (ClearType == E_BLOCK_END_STONE)
					{
						// The platform spawned underground, so don't drop the end stone:
						m_World->DigBlock(ClearPos);
					}
					else
					{
						m_World->DropBlockAsPickups(ClearPos);
					}
				}
			}
		}
	}

private:

	cWorld * m_World;
};





void cEndPlatform::Generate(cWorld * a_EndWorld)
{
	// Move the world spawn onto the platform: entities entering the End (and respawning) arrive there,
	// instead of on the main island:
	if ((a_EndWorld->GetSpawnX() != END_PLATFORM_X) || (a_EndWorld->GetSpawnZ() != END_PLATFORM_Z))
	{
		a_EndWorld->SetSpawn(END_PLATFORM_X, END_PLATFORM_Y + 1, END_PLATFORM_Z);
	}

	// The platform straddles the chunk border at z = 0, so both chunks must be ready first:
	a_EndWorld->PrepareChunk(6, -1, std::make_unique<cEndPlatformCallback>(a_EndWorld));
	a_EndWorld->PrepareChunk(6, 0, std::make_unique<cEndPlatformCallback>(a_EndWorld));
}
