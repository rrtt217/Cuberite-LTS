// StrongholdTest.cpp

// Implements the test for the Stronghold cubeset's structural invariants

/*
The vanilla 1.12.2 stronghold must generate underground and must contain an activatable End portal.
This test checks both properties against the real data and the real generator:

	1. The piece pool loads; only the two portal-room pieces are starting pieces, and each of them
	places the structure underground (VerticalStrategy = Range|20|40). Any other starting piece would
	get the cPrefabPiecePool "Fixed|150" fallback and the stronghold would float in the air.
	2. The overworld's default finisher list registers the stronghold.
	3. The stronghold finisher really generates a portal room: driving cPieceStructuresGen with the
	Stronghold data at a known structure origin produces a complete End portal frame ring underground.
	4. Each portal-room piece contains exactly one complete ring - 12 frames in a 5x5 footprint
	without the corners - with every frame facing the interior of the ring. That is the only
	orientation accepted by cBlockEndPortalFrameHandler::FindAndSetPortal; a frame facing elsewhere
	can still receive an eye, but the portal never forms. See the stronghold spec in the specs
	directory (vanilla-1.12.2-stronghold.md) for the exhaustive check behind that claim.
*/

#include "Globals.h"
#include "Generating/BioGen.h"
#include "Generating/ChunkDesc.h"
#include "Generating/ComposableGenerator.h"
#include "Generating/HeiGen.h"
#include "Generating/PieceStructuresGen.h"
#include "Generating/Prefab.h"
#include "Generating/PrefabPiecePool.h"
#include "IniFile.h"
#include "Noise/Noise.h"
#include "../TestHelpers.h"

/** Number of frames in a complete End portal ring. */
static constexpr size_t TEST_FRAME_COUNT = 12;

/** The ring spans 5 blocks in each axis, so its extremes differ by this much. */
static constexpr int TEST_RING_SPAN = 4;

/** The Y at which a portal room is drawn by the piece test; any Y inside the chunk works. */
static constexpr int TEST_DRAW_Y = 30;

/** Lowest and highest Y at which the stronghold's starting pieces may be placed (Range|20|40). */
static constexpr int TEST_MIN_START_Y = 20;
static constexpr int TEST_MAX_START_Y = 40;

/** The Y of the frames inside the portal-room pieces (see the cubeset's BlockData). */
static constexpr int TEST_FRAME_LOCAL_Y = 3;

/** Seed used for the generation test. */
static constexpr int TEST_SEED = 1;

/** Sea level passed to the generator; the stronghold's vertical strategy ignores it. */
static constexpr int TEST_SEA_LEVEL = 64;

/** Maps each frame of a ring, keyed by its XZ coords, to its direction meta. */
using cFrameRing = std::map<std::pair<int, int>, int>;





/** Checks whether the frames form one complete ring with all frames facing the interior, and sets the
reason in a_Error if they do not.
If a_AllowMetaShift is set, a uniform rotation of the direction values is accepted as well; that is
what a prefab rotation produces in this test target, because cBlockHandler is stubbed here and thus
the metas are not rotated along with the blocks (see testStrongholdGeneration). */
static bool isValidFrameRing(const cFrameRing & a_Frames, bool a_AllowMetaShift, AString & a_Error)
{
	if (a_Frames.size() != TEST_FRAME_COUNT)
	{
		a_Error = fmt::format(FMT_STRING("expected {} frames, found {}"), TEST_FRAME_COUNT, a_Frames.size());
		return false;
	}

	int MinX = a_Frames.begin()->first.first;
	int MaxX = MinX;
	int MinZ = a_Frames.begin()->first.second;
	int MaxZ = MinZ;
	for (const auto & Frame: a_Frames)
	{
		MinX = std::min(MinX, Frame.first.first);
		MaxX = std::max(MaxX, Frame.first.first);
		MinZ = std::min(MinZ, Frame.first.second);
		MaxZ = std::max(MaxZ, Frame.first.second);
	}
	if ((MaxX - MinX != TEST_RING_SPAN) || (MaxZ - MinZ != TEST_RING_SPAN))
	{
		a_Error = fmt::format(FMT_STRING("the ring must span 5 blocks in each axis, found {} by {}"),
			MaxX - MinX + 1, MaxZ - MinZ + 1
		);
		return false;
	}

	// The 4 corners are missing, so each frame sits on exactly one edge of the ring:
	for (const auto & Frame: a_Frames)
	{
		const int X = Frame.first.first;
		const int Z = Frame.first.second;
		const int Edges =
			((X == MinX) ? 1 : 0) + ((X == MaxX) ? 1 : 0) +
			((Z == MinZ) ? 1 : 0) + ((Z == MaxZ) ? 1 : 0);
		if (Edges != 1)
		{
			a_Error = fmt::format(FMT_STRING("frame ({}, {}) does not sit on exactly one edge"), X, Z);
			return false;
		}
	}

	// All frames must face the interior; any other orientation makes FindAndSetPortal bail out:
	for (int Shift = 0; Shift < (a_AllowMetaShift ? 4 : 1); Shift++)
	{
		bool AllMatch = true;
		for (const auto & Frame: a_Frames)
		{
			const int X = Frame.first.first;
			const int Z = Frame.first.second;
			const int Expected = (
				(Z == MinZ) ? static_cast<int>(E_META_END_PORTAL_FRAME_ZP) :
				(Z == MaxZ) ? static_cast<int>(E_META_END_PORTAL_FRAME_ZM) :
				(X == MinX) ? static_cast<int>(E_META_END_PORTAL_FRAME_XP) :
				static_cast<int>(E_META_END_PORTAL_FRAME_XM)
			) + Shift;
			if (Frame.second != (Expected & 0x03))
			{
				a_Error = fmt::format(FMT_STRING("frame ({}, {}) faces {} instead of {}"),
					X, Z, Frame.second, Expected & 0x03
				);
				AllMatch = false;
				break;
			}
		}
		if (AllMatch)
		{
			return true;
		}
	}
	return false;
}





/** Checks that the frames form one complete ring, throws TestException if they do not. */
static void checkFrameRing(const cFrameRing & a_Frames, bool a_AllowMetaShift)
{
	AString Error;
	if (!isValidFrameRing(a_Frames, a_AllowMetaShift, Error))
	{
		TEST_FAIL(Error);
	}
}





/** Picks one valid ring out of all drawn frames. Needed when two structures overlap.
Throws TestException if no complete ring can be found. */
static void checkAnyFrameRing(const cFrameRing & a_Drawn, bool a_AllowMetaShift)
{
	AString Error = fmt::format(FMT_STRING("no complete End portal frame ring among the {} drawn frames"), a_Drawn.size());
	for (const auto & Seed: a_Drawn)
	{
		for (int dx = 0; dx <= TEST_RING_SPAN; dx++)
		{
			for (int dz = 0; dz <= TEST_RING_SPAN; dz++)
			{
				const int MinX = Seed.first.first - dx;
				const int MinZ = Seed.first.second - dz;
				cFrameRing Ring;
				for (const auto & Frame: a_Drawn)
				{
					if (
						(Frame.first.first >= MinX) && (Frame.first.first <= MinX + TEST_RING_SPAN) &&
						(Frame.first.second >= MinZ) && (Frame.first.second <= MinZ + TEST_RING_SPAN)
					)
					{
						Ring.insert(Frame);
					}
				}
				if (isValidFrameRing(Ring, a_AllowMetaShift, Error))
				{
					return;
				}
			}
		}
	}
	TEST_FAIL(Error);
}





/** Draws the prefab into an empty chunk and returns all End portal frames it wrote. */
static cFrameRing drawFrameRing(const cPrefab & a_Prefab)
{
	cChunkDesc ChunkDesc(cChunkCoords(0, 0));
	a_Prefab.Draw(ChunkDesc, Vector3i(0, TEST_DRAW_Y, 0), 0);

	cFrameRing Frames;
	for (int y = 0; y < cChunkDef::Height; y++)
	{
		for (int z = 0; z < cChunkDef::Width; z++)
		{
			for (int x = 0; x < cChunkDef::Width; x++)
			{
				if (ChunkDesc.GetBlockType(x, y, z) == E_BLOCK_END_PORTAL_FRAME)
				{
					// Only the two direction bits are of interest here:
					Frames[{x, z}] = ChunkDesc.GetBlockMeta(x, y, z) & 0x03;
				}
			}
		}
	}
	return Frames;
}





/** Verifies the structure of the cubeset's portal rooms. */
static void testPortalRoomPieces()
{
	// The test's working directory is the server directory, so that both the direct load and the
	// generator's own lookup use the production path:
	cPrefabPiecePool Pool;
	TEST_TRUE(Pool.LoadFromFile("Prefabs/PieceStructures/Stronghold.cubeset", true));

	const cPieces StartingPieces = Pool.GetStartingPieces();
	TEST_EQUAL_MSG(StartingPieces.size(), 2, "only the two portal rooms may be starting pieces");

	for (auto * Piece: StartingPieces)
	{
		// A starting piece without a VerticalStrategy gets the Fixed|150 fallback, which would make
		// the stronghold generate floating in the air instead of underground:
		const int StartY = Piece->GetStartingPieceHeight(0, 0);
		TEST_GREATER_THAN_OR_EQUAL(StartY, TEST_MIN_START_Y);
		TEST_LESS_THAN_OR_EQUAL(StartY, TEST_MAX_START_Y);

		const cPrefab * Prefab = dynamic_cast<const cPrefab *>(Piece);
		TEST_TRUE(Prefab != nullptr);

		// The unrotated piece holds the ring the cubeset was fixed to; no meta shift is involved here:
		checkFrameRing(drawFrameRing(*Prefab), false);
	}
}





/** Verifies that the overworld's default finishers register the stronghold. */
static void testOverworldRegistration()
{
	cIniFile Ini;
	cComposableGenerator::InitializeGeneratorDefaults(Ini, dimOverworld);
	const AString Finishers = Ini.GetValue("Generator", "Finishers");

	// The stronghold must be one of the overworld's default finishers:
	TEST_NOTEQUAL(Finishers.find("PieceStructures: Stronghold"), AString::npos);
}





/** Verifies that the finisher really generates an underground portal room.
The structure origin is derived with the same expression that cGridStructGen uses for the grid cell
(0, 0), so that only a handful of chunks need to be generated.
Note that this test target stubs cBlockHandler::For, so a rotated prefab keeps its metas unchanged
here; the drawn ring is therefore accepted up to a uniform rotation of the direction values. */
static void testStrongholdGeneration()
{
	cBioGenConstant BiomeGen;
	cHeiGenFlat HeightGen;
	cPieceStructuresGen Gen(TEST_SEED);
	TEST_TRUE(Gen.Initialize("Stronghold", TEST_SEA_LEVEL, BiomeGen, HeightGen));

	// Mirrors cGridStructGen::GetStructuresForChunk() with the cubeset's GridSize / MaxOffset, scaled
	// down to the grid cell (0, 0); its origin is where the starting piece (a portal room) is placed:
	const int MaxOffset = 256;
	cNoise Noise(TEST_SEED);
	const int GridX = 0;
	const int GridZ = 0;
	const int OriginX = GridX + ((Noise.IntNoise2DInt(GridX + 3, GridZ + 5) / 7) % (MaxOffset * 2)) - MaxOffset;
	const int OriginZ = GridZ + ((Noise.IntNoise2DInt(GridX + 5, GridZ + 3) / 7) % (MaxOffset * 2)) - MaxOffset;

	// The portal room is inside the piece's first 16 blocks in each axis, no matter its rotation:
	const cChunkCoords ChunkMin = cChunkDef::BlockToChunk(Vector3i(OriginX - 2, 0, OriginZ - 2));
	const cChunkCoords ChunkMax = cChunkDef::BlockToChunk(Vector3i(OriginX + 18, 0, OriginZ + 18));

	cFrameRing Frames;
	int MinFrameY = cChunkDef::Height;
	int MaxFrameY = -1;
	for (int ChunkZ = ChunkMin.m_ChunkZ; ChunkZ <= ChunkMax.m_ChunkZ; ChunkZ++)
	{
		for (int ChunkX = ChunkMin.m_ChunkX; ChunkX <= ChunkMax.m_ChunkX; ChunkX++)
		{
			cChunkDesc ChunkDesc(cChunkCoords(ChunkX, ChunkZ));
			Gen.GenFinish(ChunkDesc);
			for (int y = 0; y < cChunkDef::Height; y++)
			{
				for (int z = 0; z < cChunkDef::Width; z++)
				{
					for (int x = 0; x < cChunkDef::Width; x++)
					{
						if (ChunkDesc.GetBlockType(x, y, z) != E_BLOCK_END_PORTAL_FRAME)
						{
							continue;
						}
						Frames[{(ChunkX * cChunkDef::Width) + x, (ChunkZ * cChunkDef::Width) + z}] =
							ChunkDesc.GetBlockMeta(x, y, z) & 0x03;
						MinFrameY = std::min(MinFrameY, y);
						MaxFrameY = std::max(MaxFrameY, y);
					}
				}
			}
		}
	}

	checkAnyFrameRing(Frames, true);
	TEST_GREATER_THAN_OR_EQUAL(MinFrameY, TEST_MIN_START_Y + TEST_FRAME_LOCAL_Y);
	TEST_LESS_THAN_OR_EQUAL(MaxFrameY, TEST_MAX_START_Y + TEST_FRAME_LOCAL_Y);
}





IMPLEMENT_TEST_MAIN("StrongholdTest",
	testPortalRoomPieces();
	testOverworldRegistration();
	testStrongholdGeneration();
)
