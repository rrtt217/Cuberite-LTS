// CubesetBlockEntitiesTest.cpp

// Implements the tests for the cubeset BlockEntities field, which carries the non-loot block entity
// contents of a prefab (the mob spawner's mob type, the flower pot's contents).
// See specs/vanilla-1.12.2-cubeset-block-entities.md for the format and the data sources.

/*
COVERAGE BOUNDARY - read before extending this file.

These tests run in the generator test build, which deliberately does not link the block entity and
item systems: tests/Generating/Stubs.cpp makes cBlockEntity::CreateByBlockType() return nullptr and
cBlockEntity::IsBlockEntityBlockType() return false, so a prefab's image never materialises a block
entity here. Consequently this file can only verify the *parsing* contract:

	1. a piece without a BlockEntities table keeps loading (the pre-existing cubesets are unaffected),
	2. a well-formed entry is accepted, even though this build has no block entity to fill - the entry
	   is reported and skipped, and the rest of the piece stays valid,
	3. an entry that cannot be applied at all (missing coords) fails the load,
	4. the migrated production cubesets still load.

What this file explicitly does NOT verify, and cannot verify here:

	* that the contents actually reach the block entity (that needs the real block entity and item
	  systems),
	* that the values are semantically right (a blaze spawner in the Nether fortress, a silverfish
	  spawner in the stronghold) - those come from the sources listed in the spec, not from a test.

The behaviour of the contents themselves has to be verified against a real server; see the
"Verification" section of the spec.
*/

#include "Globals.h"
#include "Generating/PrefabPiecePool.h"
#include "StringUtils.h"
#include "../TestHelpers.h"

/** The cubeset used by the tests: a 5 x 3 x 5 stone piece with a mob spawner at (1, 1, 2).
The BlockEntities marker is replaced by the individual tests. */
static const char * TEST_CUBESET = R"(
Cubeset =
{
	Metadata =
	{
		CubesetFormatVersion = 1,
		["IntendedUse"] = "PieceStructures",
	},

	Pieces =
	{
		{
			Size =
			{
				x = 5,
				y = 3,
				z = 5,
			},
			Hitbox =
			{
				MinX = 0,
				MinY = 0,
				MinZ = 0,
				MaxX = 4,
				MaxY = 2,
				MaxZ = 4,
			},
			BlockDefinitions =
			{
				".:  0: 0",  -- air
				"a:  1: 0",  -- stone
				"s: 52: 0",  -- mob spawner
			},
			BlockData =
			{
				-- Level 0
				"aaaaa",  --  0
				"aaaaa",  --  1
				"aaaaa",  --  2
				"aaaaa",  --  3
				"aaaaa",  --  4

				-- Level 1, the spawner at (1, 1, 2)
				"aaaaa",  --  0
				"aaaaa",  --  1
				"asaaa",  --  2
				"aaaaa",  --  3
				"aaaaa",  --  4

				-- Level 2
				"aaaaa",  --  0
				"aaaaa",  --  1
				"aaaaa",  --  2
				"aaaaa",  --  3
				"aaaaa",  --  4
			},
			Connectors =
			{
				{
					Type = 1,
					RelX = 0,
					RelY = 0,
					RelZ = 0,
					Direction = 2,  -- Z-
				},
			},
			-- __BLOCK_ENTITIES__
			Metadata =
			{
				["DefaultWeight"] = "100",
				["AllowedRotations"] = "0",
				["MergeStrategy"] = "msOverwrite",
				["IsStarting"] = "0",
			},
		},
	},
}
)";





/** Returns the test cubeset with the marker replaced by the specified BlockEntities definition. */
static AString WithBlockEntities(const AString & a_Definition)
{
	AString res = TEST_CUBESET;
	ReplaceString(res, "-- __BLOCK_ENTITIES__", a_Definition);
	return res;
}





/** Verifies that a piece without a BlockEntities table keeps loading. */
static void testNoBlockEntitiesTable()
{
	cPrefabPiecePool Pool;
	TEST_TRUE(Pool.LoadFromCubeset(TEST_CUBESET, "BlockEntitiesTest.cubeset", true));
}





/** Verifies that a well-formed entry is accepted, even though this build cannot materialise the entity. */
static void testWellFormedEntryAccepted()
{
	cPrefabPiecePool Pool;
	const AString Contents = WithBlockEntities(
		"BlockEntities =\n"
		"\t\t\t{\n"
		"\t\t\t\t{ X = 1, Y = 1, Z = 2, Entity = \"silverfish\" },\n"
		"\t\t\t},"
	);
	TEST_TRUE(Pool.LoadFromCubeset(Contents, "BlockEntitiesTest.cubeset", true));
}





/** Verifies that an entry that cannot be applied at all fails the load. */
static void testEntryWithoutCoordsRejected()
{
	cPrefabPiecePool Pool;
	const AString Contents = WithBlockEntities(
		"BlockEntities =\n"
		"\t\t\t{\n"
		"\t\t\t\t{ Entity = \"silverfish\" },\n"
		"\t\t\t},"
	);
	TEST_FALSE(Pool.LoadFromCubeset(Contents, "BlockEntitiesTest.cubeset", false));
}





/** Verifies that the migrated production cubesets still load. */
static void testProductionCubesets()
{
	cPrefabPiecePool NetherFort;
	TEST_TRUE(NetherFort.LoadFromFile("Prefabs/PieceStructures/NetherFort.cubeset", true));

	cPrefabPiecePool Stronghold;
	TEST_TRUE(Stronghold.LoadFromFile("Prefabs/PieceStructures/Stronghold.cubeset", true));
}





IMPLEMENT_TEST_MAIN("CubesetBlockEntitiesTest",
	testNoBlockEntitiesTable();
	testWellFormedEntryAccepted();
	testEntryWithoutCoordsRejected();
	testProductionCubesets();
)
