// StructureLocatorTest.cpp

// Implements the test for the read-only structure locator used by the Eye of Ender

/*
The locator (cPieceStructuresGen::GetNearestStructureTarget) must find the same structure that the
generator would place, without generating anything. This test drives it against a brute-force search
that uses the same grid formula as cGridStructGen, and checks the returned target's coords and Y.
See the eye-of-ender spec in the specs directory (vanilla-1.12.2-eye-of-ender.md).
*/

#include "Globals.h"
#include "Generating/BioGen.h"
#include "Generating/HeiGen.h"
#include "Generating/PieceStructuresGen.h"
#include "Noise/Noise.h"
#include "../TestHelpers.h"

/** Seed used for the generator and for the brute-force noise. */
static constexpr int TEST_SEED = 1;

/** Sea level passed to the generator; the stronghold's vertical strategy ignores it. */
static constexpr int TEST_SEA_LEVEL = 64;

/** The stronghold cubeset's grid parameters (see its Metadata). */
static constexpr int TEST_GRID_SIZE = 512;
static constexpr int TEST_MAX_OFFSET = 256;

/** Lowest and highest Y the stronghold's starting pieces may be placed at (Range|20|40). */
static constexpr int TEST_MIN_START_Y = 20;
static constexpr int TEST_MAX_START_Y = 40;

/** How many cells around the queried one the brute force examines in each direction. */
static constexpr int TEST_BRUTE_FORCE_CELLS = 6;

/** Origin of the stronghold in the grid cell (0, 0) for TEST_SEED; StrongholdTest generates this
structure and finds its portal room, so its full Y range is known to be valid here. */
static const Vector3i TEST_VERIFIED_ORIGIN = {64, 0, -233};





/** Returns the origin of the structure in the specified grid cell, exactly as cGridStructGen computes it. */
static Vector3i cellOrigin(const cNoise & a_Noise, int a_CellX, int a_CellZ)
{
	const int GridX = a_CellX * TEST_GRID_SIZE;
	const int GridZ = a_CellZ * TEST_GRID_SIZE;
	const int OriginX = GridX + ((a_Noise.IntNoise2DInt(GridX + 3, GridZ + 5) / 7) % (TEST_MAX_OFFSET * 2)) - TEST_MAX_OFFSET;
	const int OriginZ = GridZ + ((a_Noise.IntNoise2DInt(GridX + 5, GridZ + 3) / 7) % (TEST_MAX_OFFSET * 2)) - TEST_MAX_OFFSET;
	return {OriginX, 0, OriginZ};
}





/** Returns the origin of the structure whose origin is nearest to a_Position in the XZ plane,
by scanning a window wider than the one the locator itself uses. */
static Vector3i bruteForceNearest(const cNoise & a_Noise, Vector3i a_Position)
{
	const int CenterX = FAST_FLOOR_DIV(a_Position.x, TEST_GRID_SIZE);
	const int CenterZ = FAST_FLOOR_DIV(a_Position.z, TEST_GRID_SIZE);
	Int64 BestDistanceSq = -1;
	Vector3i Best;
	for (int CellX = CenterX - TEST_BRUTE_FORCE_CELLS; CellX <= CenterX + TEST_BRUTE_FORCE_CELLS; CellX++)
	{
		for (int CellZ = CenterZ - TEST_BRUTE_FORCE_CELLS; CellZ <= CenterZ + TEST_BRUTE_FORCE_CELLS; CellZ++)
		{
			const Vector3i Origin = cellOrigin(a_Noise, CellX, CellZ);
			const Int64 DistanceX = static_cast<Int64>(Origin.x) - a_Position.x;
			const Int64 DistanceZ = static_cast<Int64>(Origin.z) - a_Position.z;
			const Int64 DistanceSq = (DistanceX * DistanceX) + (DistanceZ * DistanceZ);
			if ((BestDistanceSq < 0) || (DistanceSq < BestDistanceSq))
			{
				BestDistanceSq = DistanceSq;
				Best = Origin;
			}
		}
	}
	return Best;
}





/** Checks that the locator finds the brute-force nearest structure for the specified position. */
static void checkQuery(cPieceStructuresGen & a_Gen, const cNoise & a_Noise, Vector3i a_Position, bool a_CheckYRange)
{
	Vector3i Target;
	TEST_TRUE(a_Gen.GetNearestStructureTarget("Stronghold", a_Position, Target));

	const Vector3i Expected = bruteForceNearest(a_Noise, a_Position);
	TEST_EQUAL_MSG(Target.x, Expected.x, fmt::format(FMT_STRING("query ({}, {}): wrong origin X"), a_Position.x, a_Position.z));
	TEST_EQUAL_MSG(Target.z, Expected.z, fmt::format(FMT_STRING("query ({}, {}): wrong origin Z"), a_Position.x, a_Position.z));

	// The vertical strategy is Range|20|40; because the noise is truncated towards zero, its modulo can be
	// negative, so the lower bound is only guaranteed for the positions known to be inside a structure:
	TEST_LESS_THAN_OR_EQUAL(Target.y, TEST_MAX_START_Y);
	if (a_CheckYRange)
	{
		TEST_GREATER_THAN_OR_EQUAL(Target.y, TEST_MIN_START_Y);
	}

	// The target is a fixed point of the query - asking again from the target returns the same structure:
	Vector3i Again;
	TEST_TRUE(a_Gen.GetNearestStructureTarget("Stronghold", Target, Again));
	TEST_EQUAL(Again.x, Target.x);
	TEST_EQUAL(Again.z, Target.z);
}





/** Verifies the locator against a brute-force search and its API contract. */
static void testStructureLocator()
{
	cBioGenConstant BiomeGen;
	cHeiGenFlat HeightGen;
	cPieceStructuresGen Gen(TEST_SEED);
	TEST_TRUE(Gen.Initialize("Stronghold", TEST_SEA_LEVEL, BiomeGen, HeightGen));

	cNoise Noise(TEST_SEED);

	// The grid cell (0, 0) holds the structure that StrongholdTest verifies, so here its Y must also be
	// within the starting piece's Range|20|40 (elsewhere the truncated noise modulo can push it below):
	TEST_EQUAL(cellOrigin(Noise, 0, 0).x, TEST_VERIFIED_ORIGIN.x);
	TEST_EQUAL(cellOrigin(Noise, 0, 0).z, TEST_VERIFIED_ORIGIN.z);
	checkQuery(Gen, Noise, TEST_VERIFIED_ORIGIN, true);

	// The neighbouring cells' origins, computed with the same formula as the generator uses:
	for (int CellX = -1; CellX <= 1; CellX++)
	{
		for (int CellZ = -1; CellZ <= 1; CellZ++)
		{
			if ((CellX == 0) && (CellZ == 0))
			{
				continue;
			}
			checkQuery(Gen, Noise, cellOrigin(Noise, CellX, CellZ), false);
		}
	}

	// Arbitrary and far-away positions, including ones near a cell border:
	const Vector3i QueryPositions[] =
	{
		{0, 64, 0}, {255, 64, 255}, {256, 64, 256}, {1234, 64, -5678}, {500000, 64, -700000},
	};
	for (const auto & Position: QueryPositions)
	{
		checkQuery(Gen, Noise, Position, false);
	}

	// Structures this generator doesn't know are reported as not found:
	Vector3i Target;
	TEST_FALSE(Gen.GetNearestStructureTarget("NetherFort", Vector3i(0, 64, 0), Target));
}





IMPLEMENT_TEST_MAIN("StructureLocatorTest",
	testStructureLocator();
)
