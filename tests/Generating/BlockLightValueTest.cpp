#include "Globals.h"
#include "BlockType.h"
#include "BlockInfo.h"
#include "../TestHelpers.h"

// BlockLightValueTest
//
// Verifies the vanilla 1.12.2 block light-emission table as specified in specs/vanilla-1.12.2-lighting.md,
// section 2 (sourced from the Minecraft Wiki "Light" page, 2018-11-24 revision, the last revision covering
// 1.12.2 before the 1.14 lighting rewrite).
//
// Invariants checked:
//   - every block listed as emissive in the spec table returns exactly its documented light value
//     (fixes spec deviation 1, the end gateway was missing its value of 15);
//   - blocks that vanilla lists as non-emissive return zero light
//     (fixes spec deviation 2, the active comparator and the powered repeater no longer emit 9);
//   - the spread-light falloff of the de-emissived blocks is unchanged, removing emission must not turn
//     a light-transparent block into an opaque one (spec section 1.2).
//
// Why verified this way: cBlockInfo::GetLightValue() is a pure compile-time lookup switch, so a direct
// table comparison is the strongest oracle available without a live vanilla server (spec section 7).





/** The light levels used in the spec's section-2 table, named so that the assertions carry no magic numbers. */
static constexpr NIBBLETYPE TEST_LIGHT_15 = 15;
static constexpr NIBBLETYPE TEST_LIGHT_14 = 14;
static constexpr NIBBLETYPE TEST_LIGHT_13 = 13;
static constexpr NIBBLETYPE TEST_LIGHT_11 = 11;
static constexpr NIBBLETYPE TEST_LIGHT_9  = 9;
static constexpr NIBBLETYPE TEST_LIGHT_7  = 7;
static constexpr NIBBLETYPE TEST_LIGHT_3  = 3;
static constexpr NIBBLETYPE TEST_LIGHT_1  = 1;
static constexpr NIBBLETYPE TEST_LIGHT_0  = 0;

/** The spread-light falloff of a plain light-transparent block, light passes through with the regular -1 decay only. */
static constexpr NIBBLETYPE TEST_FALLOFF_TRANSPARENT = 1;





/** Checks every emissive block from the spec's section-2 table against its documented light value. */
static void testEmissiveTable(void)
{
	// Light 15, spec row: beacon, end gateway, end portal, fire, glowstone, lit pumpkin, lava, lit redstone lamp, sea lantern:
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_BEACON), TEST_LIGHT_15);
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_END_GATEWAY), TEST_LIGHT_15);
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_END_PORTAL), TEST_LIGHT_15);
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_FIRE), TEST_LIGHT_15);
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_GLOWSTONE), TEST_LIGHT_15);
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_JACK_O_LANTERN), TEST_LIGHT_15);
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_LAVA), TEST_LIGHT_15);
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_STATIONARY_LAVA), TEST_LIGHT_15);
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_REDSTONE_LAMP_ON), TEST_LIGHT_15);
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_SEA_LANTERN), TEST_LIGHT_15);

	// Light 14, spec row: end rod, torch:
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_END_ROD), TEST_LIGHT_14);
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_TORCH), TEST_LIGHT_14);

	// Light 13, spec row: burning furnace:
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_BURNING_FURNACE), TEST_LIGHT_13);

	// Light 11, spec row: nether portal:
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_NETHER_PORTAL), TEST_LIGHT_11);

	// Light 9, spec row: triggered redstone ore (vanilla emits only while triggered, this lookup emits always, spec marks it as accepted):
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_REDSTONE_ORE_GLOWING), TEST_LIGHT_9);

	// Light 7, spec row: ender chest, redstone torch:
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_ENDER_CHEST), TEST_LIGHT_7);
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_REDSTONE_TORCH_ON), TEST_LIGHT_7);

	// Light 3, spec row: magma:
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_MAGMA), TEST_LIGHT_3);

	// Light 1, spec row: brewing stand, brown mushroom, dragon egg, end portal frame:
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_BREWING_STAND), TEST_LIGHT_1);
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_BROWN_MUSHROOM), TEST_LIGHT_1);
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_DRAGON_EGG), TEST_LIGHT_1);
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_END_PORTAL_FRAME), TEST_LIGHT_1);
}





/** Checks that blocks which vanilla 1.12.2 lists as non-emissive return zero light. */
static void testNonEmissive(void)
{
	// Java Edition gives comparator and repeater "light = No" in either state, only Bedrock Edition emits 7 from a powered comparator:
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_ACTIVE_COMPARATOR), TEST_LIGHT_0);
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_INACTIVE_COMPARATOR), TEST_LIGHT_0);
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_REDSTONE_REPEATER_ON), TEST_LIGHT_0);
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_REDSTONE_REPEATER_OFF), TEST_LIGHT_0);

	// Spot-checks of plain blocks that must never emit:
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_AIR), TEST_LIGHT_0);
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_REDSTONE_ORE), TEST_LIGHT_0);
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_REDSTONE_TORCH_OFF), TEST_LIGHT_0);
	TEST_EQUAL(cBlockInfo::GetLightValue(E_BLOCK_STONE), TEST_LIGHT_0);
}





/** Checks that removing the emission left the spread falloff of the affected blocks intact. */
static void testFalloffUnchanged(void)
{
	// Comparator and repeater stay in the "spread blocks" falloff list, light passes through them with the usual -1 decay:
	TEST_EQUAL(cBlockInfo::GetSpreadLightFalloff(E_BLOCK_ACTIVE_COMPARATOR), TEST_FALLOFF_TRANSPARENT);
	TEST_EQUAL(cBlockInfo::GetSpreadLightFalloff(E_BLOCK_INACTIVE_COMPARATOR), TEST_FALLOFF_TRANSPARENT);
	TEST_EQUAL(cBlockInfo::GetSpreadLightFalloff(E_BLOCK_REDSTONE_REPEATER_ON), TEST_FALLOFF_TRANSPARENT);
	TEST_EQUAL(cBlockInfo::GetSpreadLightFalloff(E_BLOCK_REDSTONE_REPEATER_OFF), TEST_FALLOFF_TRANSPARENT);
}





IMPLEMENT_TEST_MAIN("BlockLightValueTest",
	testEmissiveTable();
	testNonEmissive();
	testFalloffUnchanged();
)
