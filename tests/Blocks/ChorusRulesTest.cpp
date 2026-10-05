#include "Globals.h"
#include "Blocks/ChorusRules.h"
#include "../TestHelpers.h"

/** Convenience aliases for the block types used in the tests. */
static constexpr BLOCKTYPE Air = E_BLOCK_AIR;
static constexpr BLOCKTYPE Plant = E_BLOCK_CHORUS_PLANT;
static constexpr BLOCKTYPE Flower = E_BLOCK_CHORUS_FLOWER;
static constexpr BLOCKTYPE Stone = E_BLOCK_END_STONE;





/** Builds a plant neighbourhood; the four "below horizontal neighbour" fields default to air. */
static sChorusPlantNeighborhood MakePlantNeighborhood(
	BLOCKTYPE a_Up, BLOCKTYPE a_Down, BLOCKTYPE a_North, BLOCKTYPE a_South, BLOCKTYPE a_West, BLOCKTYPE a_East)
{
	sChorusPlantNeighborhood N;
	N.m_Up = a_Up;
	N.m_Down = a_Down;
	N.m_North = a_North;
	N.m_South = a_South;
	N.m_West = a_West;
	N.m_East = a_East;
	N.m_NorthBelow = Air;
	N.m_SouthBelow = Air;
	N.m_WestBelow = Air;
	N.m_EastBelow = Air;
	return N;
}





/** Tests the chorus plant survival rule. */
static void testPlantRules(void)
{
	// Sitting on End stone or on another plant:
	TEST_TRUE(ChorusPlantCanSurvive(MakePlantNeighborhood(Air, Stone, Air, Air, Air, Air)));
	TEST_TRUE(ChorusPlantCanSurvive(MakePlantNeighborhood(Air, Plant, Air, Air, Air, Air)));

	// Hanging below a plant:
	TEST_TRUE(ChorusPlantCanSurvive(MakePlantNeighborhood(Plant, Air, Air, Air, Air, Air)));

	// Side-attached to a plant that itself has End stone / a plant below:
	{
		auto N = MakePlantNeighborhood(Air, Air, Plant, Air, Air, Air);
		N.m_NorthBelow = Stone;
		TEST_TRUE(ChorusPlantCanSurvive(N));
		N.m_NorthBelow = Plant;
		TEST_TRUE(ChorusPlantCanSurvive(N));
	}

	// Side-attached to a plant whose below is air is not enough:
	TEST_FALSE(ChorusPlantCanSurvive(MakePlantNeighborhood(Air, Air, Plant, Air, Air, Air)));

	// No support whatsoever:
	TEST_FALSE(ChorusPlantCanSurvive(MakePlantNeighborhood(Air, Air, Air, Air, Air, Air)));

	// Too thick: touching a plant horizontally with no air above or below:
	TEST_FALSE(ChorusPlantCanSurvive(MakePlantNeighborhood(Plant, Stone, Plant, Air, Air, Air)));

	// The same, but with air below, is allowed:
	TEST_TRUE(ChorusPlantCanSurvive(MakePlantNeighborhood(Plant, Air, Plant, Air, Air, Air)));
}





/** Tests the chorus flower placement rule. */
static void testFlowerRules(void)
{
	// Sitting on End stone or on a plant:
	TEST_TRUE(ChorusFlowerCanSurvive(Stone, Air, 0));
	TEST_TRUE(ChorusFlowerCanSurvive(Plant, Air, 0));

	// Hanging in the air next to exactly one plant:
	TEST_TRUE(ChorusFlowerCanSurvive(Air, Air, 1));
	TEST_FALSE(ChorusFlowerCanSurvive(Air, Air, 0));
	TEST_FALSE(ChorusFlowerCanSurvive(Air, Air, 2));

	// Blocked above and no support below:
	TEST_FALSE(ChorusFlowerCanSurvive(Air, Plant, 1));
}





IMPLEMENT_TEST_MAIN("ChorusRulesTest",
	testPlantRules();
	testFlowerRules();
)
