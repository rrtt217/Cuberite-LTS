#include "Globals.h"
#include "Mobs/EndermanBlockRules.h"
#include "../TestHelpers.h"

/** What this test verifies:
(1) The 1.12.2 enderman "holdable" block list of specs/vanilla-1.12.2-enderman.md section 3.7 -
the current wiki list minus its 1.13+ additions, plus netherrack (1.10 / 16w20a, removed 1.16 / 20w07a);
(2) The metadata discrimination the list needs: dirt / coarse dirt / podzol share one block, and only the
small flowers are holdable, not every flower-metadata value;
(3) The placement condition of section 3.7: the target must be air and the block below it a full block;
(4) The sizes and vertical placement of the pickup (4x3x4) and placement (2x2x2) regions.

Invariants:
- Every holdable block of the spec is accepted, and blocks that only look similar (stone, saplings, tall
  grass, big mushrooms, jack o'lantern, ...) are rejected;
- The pickup region never samples the block below the enderman's feet, so a completely flat floor cannot
  be picked up (wiki, "Moving blocks");
- The placement region starts at the enderman's own level.

Why: the block list and the region geometry are the numerically testable parts of the block-carrying
behaviour without a world harness, so they are extracted into pure functions. */

/** Convenience aliases for the block types used in the tests. */
static constexpr BLOCKTYPE Air = E_BLOCK_AIR;
static constexpr BLOCKTYPE Stone = E_BLOCK_STONE;
static constexpr BLOCKTYPE Dirt = E_BLOCK_DIRT;
static constexpr BLOCKTYPE Grass = E_BLOCK_GRASS;
static constexpr BLOCKTYPE Cactus = E_BLOCK_CACTUS;





/** Tests that the whole 1.12.2 holdable list is accepted. */
static void testHoldableBlocks(void)
{
	TEST_TRUE(IsEndermanHoldableBlock(Grass, 0));

	// Dirt, coarse dirt and podzol share the dirt block:
	TEST_TRUE(IsEndermanHoldableBlock(Dirt, E_META_DIRT_NORMAL));
	TEST_TRUE(IsEndermanHoldableBlock(Dirt, E_META_DIRT_COARSE));
	TEST_TRUE(IsEndermanHoldableBlock(Dirt, E_META_DIRT_PODZOL));

	// Sand and red sand:
	TEST_TRUE(IsEndermanHoldableBlock(E_BLOCK_SAND, E_META_SAND_NORMAL));
	TEST_TRUE(IsEndermanHoldableBlock(E_BLOCK_SAND, E_META_SAND_RED));

	TEST_TRUE(IsEndermanHoldableBlock(E_BLOCK_GRAVEL, 0));
	TEST_TRUE(IsEndermanHoldableBlock(E_BLOCK_CLAY, 0));
	TEST_TRUE(IsEndermanHoldableBlock(E_BLOCK_BROWN_MUSHROOM, 0));
	TEST_TRUE(IsEndermanHoldableBlock(E_BLOCK_RED_MUSHROOM, 0));

	// Small flowers - the dandelion and every red-flower metadata up to the oxeye daisy:
	TEST_TRUE(IsEndermanHoldableBlock(E_BLOCK_DANDELION, 0));
	TEST_TRUE(IsEndermanHoldableBlock(E_BLOCK_FLOWER, E_META_FLOWER_POPPY));
	TEST_TRUE(IsEndermanHoldableBlock(E_BLOCK_FLOWER, E_META_FLOWER_BLUE_ORCHID));
	TEST_TRUE(IsEndermanHoldableBlock(E_BLOCK_FLOWER, E_META_FLOWER_ALLIUM));
	TEST_TRUE(IsEndermanHoldableBlock(E_BLOCK_FLOWER, 3));  // Azure bluet, unnamed in this codebase
	TEST_TRUE(IsEndermanHoldableBlock(E_BLOCK_FLOWER, E_META_FLOWER_RED_TULIP));
	TEST_TRUE(IsEndermanHoldableBlock(E_BLOCK_FLOWER, E_META_FLOWER_ORANGE_TULIP));
	TEST_TRUE(IsEndermanHoldableBlock(E_BLOCK_FLOWER, E_META_FLOWER_WHITE_TULIP));
	TEST_TRUE(IsEndermanHoldableBlock(E_BLOCK_FLOWER, E_META_FLOWER_PINK_TULIP));
	TEST_TRUE(IsEndermanHoldableBlock(E_BLOCK_FLOWER, E_META_FLOWER_OXEYE_DAISY));

	TEST_TRUE(IsEndermanHoldableBlock(E_BLOCK_PUMPKIN, 0));
	TEST_TRUE(IsEndermanHoldableBlock(E_BLOCK_MELON, 0));
	TEST_TRUE(IsEndermanHoldableBlock(E_BLOCK_TNT, 0));
	TEST_TRUE(IsEndermanHoldableBlock(E_BLOCK_CACTUS, 0));
	TEST_TRUE(IsEndermanHoldableBlock(E_BLOCK_MYCELIUM, 0));

	// Netherrack was introduced to the list in 1.10 (16w20a) and removed again in 1.16 (20w07a):
	TEST_TRUE(IsEndermanHoldableBlock(E_BLOCK_NETHERRACK, 0));
}





/** Tests that the blocks outside the 1.12.2 holdable list are rejected. */
static void testNonHoldableBlocks(void)
{
	TEST_FALSE(IsEndermanHoldableBlock(Air, 0));
	TEST_FALSE(IsEndermanHoldableBlock(Stone, 0));
	TEST_FALSE(IsEndermanHoldableBlock(E_BLOCK_COBBLESTONE, 0));
	TEST_FALSE(IsEndermanHoldableBlock(E_BLOCK_BEDROCK, 0));
	TEST_FALSE(IsEndermanHoldableBlock(E_BLOCK_OBSIDIAN, 0));
	TEST_FALSE(IsEndermanHoldableBlock(E_BLOCK_SANDSTONE, 0));
	TEST_FALSE(IsEndermanHoldableBlock(E_BLOCK_LOG, 0));
	TEST_FALSE(IsEndermanHoldableBlock(E_BLOCK_LEAVES, 0));
	TEST_FALSE(IsEndermanHoldableBlock(E_BLOCK_SAPLING, 0));
	TEST_FALSE(IsEndermanHoldableBlock(E_BLOCK_TALL_GRASS, 0));
	TEST_FALSE(IsEndermanHoldableBlock(E_BLOCK_DEAD_BUSH, 0));
	TEST_FALSE(IsEndermanHoldableBlock(E_BLOCK_FARMLAND, 0));

	// The big mushroom blocks are not the small mushrooms of the list:
	TEST_FALSE(IsEndermanHoldableBlock(E_BLOCK_HUGE_BROWN_MUSHROOM, 0));
	TEST_FALSE(IsEndermanHoldableBlock(E_BLOCK_HUGE_RED_MUSHROOM, 0));

	// The jack o'lantern is not a pumpkin for this list:
	TEST_FALSE(IsEndermanHoldableBlock(E_BLOCK_JACK_O_LANTERN, 0));

	// The tall double plants are not "small flowers":
	TEST_FALSE(IsEndermanHoldableBlock(E_BLOCK_BIG_FLOWER, 0));

	// Metadata variants of the shared blocks that must not be picked up:
	TEST_FALSE(IsEndermanHoldableBlock(Dirt, 3));  // Not a valid dirt variant
	TEST_FALSE(IsEndermanHoldableBlock(E_BLOCK_FLOWER, 9));  // Past the oxeye daisy
}





/** Tests the placement condition: air above, full block below. */
static void testPlacement(void)
{
	// Typical cases - air above a full block:
	TEST_TRUE(EndermanCanPlaceBlockAt(Air, Dirt));
	TEST_TRUE(EndermanCanPlaceBlockAt(Air, Grass));
	TEST_TRUE(EndermanCanPlaceBlockAt(Air, Stone));
	TEST_TRUE(EndermanCanPlaceBlockAt(Air, E_BLOCK_MYCELIUM));

	// The target must be air:
	TEST_FALSE(EndermanCanPlaceBlockAt(Stone, Stone));
	TEST_FALSE(EndermanCanPlaceBlockAt(E_BLOCK_WATER, Stone));

	// The support must be a full block:
	TEST_FALSE(EndermanCanPlaceBlockAt(Air, Air));
	TEST_FALSE(EndermanCanPlaceBlockAt(Air, E_BLOCK_WATER));
	TEST_FALSE(EndermanCanPlaceBlockAt(Air, E_BLOCK_TALL_GRASS));
	TEST_FALSE(EndermanCanPlaceBlockAt(Air, Cactus));  // Only 15/16 of a block

	// Bedrock is a full block and endermen could place onto it until 1.16.2 (pre1):
	TEST_TRUE(EndermanCanPlaceBlockAt(Air, E_BLOCK_BEDROCK));
}





/** Tests the region sizes and their offsets. */
static void testRegions(void)
{
	// Pickup: 4 blocks horizontally, 3 vertically, 4 horizontally:
	TEST_EQUAL(ENDERMAN_PICKUP_MAX_HORIZONTAL_OFFSET - ENDERMAN_PICKUP_MIN_HORIZONTAL_OFFSET + 1, 4);
	TEST_EQUAL(ENDERMAN_PICKUP_MAX_VERTICAL_OFFSET - ENDERMAN_PICKUP_MIN_VERTICAL_OFFSET + 1, 3);
	TEST_EQUAL(ENDERMAN_PICKUP_MAX_HORIZONTAL_OFFSET - ENDERMAN_PICKUP_MIN_HORIZONTAL_OFFSET + 1, 4);

	// The pickup region starts at the feet block, so the floor below them is never sampled:
	TEST_EQUAL(ENDERMAN_PICKUP_MIN_VERTICAL_OFFSET, 0);
	TEST_EQUAL(ENDERMAN_PICKUP_MAX_VERTICAL_OFFSET, 2);

	// Placement: 2 blocks in each direction:
	TEST_EQUAL(ENDERMAN_PLACE_MAX_HORIZONTAL_OFFSET - ENDERMAN_PLACE_MIN_HORIZONTAL_OFFSET + 1, 2);
	TEST_EQUAL(ENDERMAN_PLACE_MAX_VERTICAL_OFFSET - ENDERMAN_PLACE_MIN_VERTICAL_OFFSET + 1, 2);
	TEST_EQUAL(ENDERMAN_PLACE_MAX_HORIZONTAL_OFFSET - ENDERMAN_PLACE_MIN_HORIZONTAL_OFFSET + 1, 2);

	// The placement region also starts at the enderman's own level:
	TEST_EQUAL(ENDERMAN_PLACE_MIN_VERTICAL_OFFSET, 0);

	// The per-tick chances are 1/20 for a pickup and 1/2000 for a placement:
	TEST_EQUAL(ENDERMAN_PICKUP_CHANCE_DENOMINATOR, 20);
	TEST_EQUAL(ENDERMAN_PLACE_CHANCE_DENOMINATOR, 2000);
}





IMPLEMENT_TEST_MAIN("EndermanBlockRulesTest",
	testHoldableBlocks();
	testNonHoldableBlocks();
	testPlacement();
	testRegions();
)
