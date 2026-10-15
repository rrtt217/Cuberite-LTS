// CommandBlockRulesTest.cpp

// Tests for CommandBlockRules.h - the pure 1.12.2 command block type / metadata / activation rules.
// Spec: specs/commands/vanilla-1.12.2-command-blocks.md, sections 2.1 (types), 2.4 (metadata), 2.5 (timing).
//
// Invariants verified here:
//   1. Each command block ID maps to its own type (impulse 137, repeating 210, chain 211).
//   2. The metadata encoding: facing = meta & 0x7 with the piston encoding, conditional = meta & 0x8.
//      Three facings are anchored on a real 1.12.2 save + F3 (meta 1 = up, 3 = south, 5 = east); the
//      other three follow the same encoding.
//   3. Chain blocks default to "Always Active", impulse/repeat to "Needs Redstone".
//   4. A conditional block only runs when the block behind it succeeded.
//   5. The comparator signal is the success count clamped to 0-15.
//
// Why: these are the numerically testable parts of the command block behaviour and need no world.

#include "Globals.h"
#include "BlockEntities/CommandBlockRules.h"
#include "../TestHelpers.h"

/** Convenience aliases. */
static constexpr BLOCKTYPE Impulse = E_BLOCK_COMMAND_BLOCK;
static constexpr BLOCKTYPE Repeating = E_BLOCK_REPEATING_COMMAND_BLOCK;
static constexpr BLOCKTYPE Chain = E_BLOCK_CHAIN_COMMAND_BLOCK;

/** Returns an enum class value as an int, so that TEST_EQUAL can print it. */
static int AsInt(eCommandBlockType a_Type)
{
	return static_cast<int>(a_Type);
}





/** Tests the block ID -> type mapping. */
static void testTypes(void)
{
	TEST_EQUAL(AsInt(CommandBlockTypeFromBlockType(Impulse)),   AsInt(eCommandBlockType::Impulse));
	TEST_EQUAL(AsInt(CommandBlockTypeFromBlockType(Repeating)), AsInt(eCommandBlockType::Repeat));
	TEST_EQUAL(AsInt(CommandBlockTypeFromBlockType(Chain)),     AsInt(eCommandBlockType::Chain));
}





/** Tests the metadata -> facing / conditional decoding, including the three empirically anchored values. */
static void testMetadata(void)
{
	// Anchored on the 1.12.2 save + F3 readings (spec 2.4):
	TEST_EQUAL(CommandBlockFacingFromMeta(0x1), 1);  // up
	TEST_EQUAL(CommandBlockFacingFromMeta(0x3), 3);  // south
	TEST_EQUAL(CommandBlockFacingFromMeta(0x5), 5);  // east

	// The remaining values of the same encoding:
	TEST_EQUAL(CommandBlockFacingFromMeta(0x0), 0);  // down
	TEST_EQUAL(CommandBlockFacingFromMeta(0x2), 2);  // north
	TEST_EQUAL(CommandBlockFacingFromMeta(0x4), 4);  // west

	// The conditional bit is independent of the facing:
	TEST_FALSE(CommandBlockIsConditional(0x3));
	TEST_TRUE (CommandBlockIsConditional(0x3 | 0x8));
	TEST_EQUAL(CommandBlockFacingFromMeta(0x3 | 0x8), 3);

	// Facing offsets: one unit vector per direction.
	const Vector3i Up    = CommandBlockFacingOffset(0x1);
	TEST_EQUAL(Up.x, 0); TEST_EQUAL(Up.y,  1); TEST_EQUAL(Up.z,  0);
	const Vector3i South = CommandBlockFacingOffset(0x3);
	TEST_EQUAL(South.x, 0); TEST_EQUAL(South.y, 0); TEST_EQUAL(South.z,  1);
	const Vector3i East  = CommandBlockFacingOffset(0x5);
	TEST_EQUAL(East.x, 1); TEST_EQUAL(East.y, 0); TEST_EQUAL(East.z, 0);
	const Vector3i Down  = CommandBlockFacingOffset(0x0);
	TEST_EQUAL(Down.x, 0); TEST_EQUAL(Down.y, -1); TEST_EQUAL(Down.z, 0);
	const Vector3i North = CommandBlockFacingOffset(0x2);
	TEST_EQUAL(North.x, 0); TEST_EQUAL(North.y, 0); TEST_EQUAL(North.z, -1);
	const Vector3i West  = CommandBlockFacingOffset(0x4);
	TEST_EQUAL(West.x, -1); TEST_EQUAL(West.y, 0); TEST_EQUAL(West.z, 0);
}





/** Tests the per-type "Always Active" default. */
static void testDefaultAuto(void)
{
	TEST_TRUE (CommandBlockDefaultAuto(eCommandBlockType::Chain));
	TEST_FALSE(CommandBlockDefaultAuto(eCommandBlockType::Impulse));
	TEST_FALSE(CommandBlockDefaultAuto(eCommandBlockType::Repeat));
}





/** Tests the activation / conditional gate. */
static void testShouldExecute(void)
{
	// Not activated -> never runs:
	TEST_FALSE(CommandBlockShouldExecute(false, false, true));
	TEST_FALSE(CommandBlockShouldExecute(false, true, true));

	// Unconditional + activated -> runs:
	TEST_TRUE(CommandBlockShouldExecute(true, false, false));
	TEST_TRUE(CommandBlockShouldExecute(true, false, true));

	// Conditional + activated -> only when the block behind succeeded:
	TEST_FALSE(CommandBlockShouldExecute(true, true, false));
	TEST_TRUE(CommandBlockShouldExecute(true, true, true));
}





/** Tests the comparator signal clamp. */
static void testComparatorSignal(void)
{
	TEST_EQUAL(CommandBlockComparatorSignal(-1), 0);
	TEST_EQUAL(CommandBlockComparatorSignal(0),  0);
	TEST_EQUAL(CommandBlockComparatorSignal(1),  1);
	TEST_EQUAL(CommandBlockComparatorSignal(15), 15);
	TEST_EQUAL(CommandBlockComparatorSignal(16), 15);
	TEST_EQUAL(CommandBlockComparatorSignal(999), 15);
}





/** Tests the block ID <-> type helpers and the MC|AutoCmd "Mode" mapping.
The Mode mapping is anchored on a live 1.12.2 client (changing the type to chain sent "SEQUENCE",
to repeating sent "AUTO"; "REDSTONE" is the impulse default). */
static void testTypeHelpers(void)
{
	TEST_TRUE (IsCommandBlockType(E_BLOCK_COMMAND_BLOCK));
	TEST_TRUE (IsCommandBlockType(E_BLOCK_CHAIN_COMMAND_BLOCK));
	TEST_TRUE (IsCommandBlockType(E_BLOCK_REPEATING_COMMAND_BLOCK));
	TEST_FALSE(IsCommandBlockType(E_BLOCK_STONE));
	TEST_FALSE(IsCommandBlockType(E_BLOCK_AIR));

	TEST_EQUAL(CommandBlockBlockTypeFromType(eCommandBlockType::Impulse), E_BLOCK_COMMAND_BLOCK);
	TEST_EQUAL(CommandBlockBlockTypeFromType(eCommandBlockType::Chain),   E_BLOCK_CHAIN_COMMAND_BLOCK);
	TEST_EQUAL(CommandBlockBlockTypeFromType(eCommandBlockType::Repeat),  E_BLOCK_REPEATING_COMMAND_BLOCK);

	TEST_EQUAL(AsInt(CommandBlockTypeFromMode("SEQUENCE")), AsInt(eCommandBlockType::Chain));
	TEST_EQUAL(AsInt(CommandBlockTypeFromMode("AUTO")),     AsInt(eCommandBlockType::Repeat));
	TEST_EQUAL(AsInt(CommandBlockTypeFromMode("REDSTONE")), AsInt(eCommandBlockType::Impulse));
	TEST_EQUAL(AsInt(CommandBlockTypeFromMode("")),         AsInt(eCommandBlockType::Impulse));
}





IMPLEMENT_TEST_MAIN("CommandBlockRules",
	testTypes();
	testMetadata();
	testDefaultAuto();
	testShouldExecute();
	testComparatorSignal();
	testTypeHelpers();
)
