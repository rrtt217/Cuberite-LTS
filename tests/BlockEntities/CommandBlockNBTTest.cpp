// CommandBlockNBTTest.cpp

// Tests for the 1.12.2 command block NBT field mapping (CommandBlockNBT.h).
// Spec: specs/vanilla-1.12.2-command-blocks.md, section 2.7 (and the round-trip requirement in section 4).
//
// Invariants verified here:
//   1. Writing every field and reading it back yields the same values (including the 15 comparator
//      boundary and a multi-byte LastExecution).
//   2. Reading a compound that has none of the fields leaves the vanilla defaults untouched, which is
//      what the Anvil loader relies on.
//
// Why: the field mapping is the numerically testable part of the save format and needs no world. It is
// kept in free functions (used by both the chunk serializer and the Anvil loader) so that it can be
// tested against the real FastNBT writer / parser.

#include "Globals.h"
#include "WorldStorage/CommandBlockNBT.h"
#include "WorldStorage/FastNBT.h"
#include "../TestHelpers.h"





/** Tests that every field survives a write -> parse -> read round trip. */
static void testRoundTrip(void)
{
	sCommandBlockData Data;
	Data.m_Command = "say hello \"world\"";
	Data.m_SuccessCount = 15;
	Data.m_LastOutput = "{\"text\":\"hi\"}";
	Data.m_TrackOutput = false;
	Data.m_Powered = true;
	Data.m_Auto = true;
	Data.m_ConditionMet = false;
	Data.m_UpdateLastExecution = false;
	Data.m_LastExecution = 1234567890123LL;

	cFastNBTWriter Writer;
	WriteCommandBlockNBT(Writer, Data);
	Writer.Finish();

	cParsedNBT NBT(Writer.GetResult());
	TEST_TRUE(NBT.IsValid());

	sCommandBlockData Read;
	ReadCommandBlockNBT(NBT, NBT.GetRoot(), Read);

	TEST_EQUAL(Read.m_Command, Data.m_Command);
	TEST_EQUAL(Read.m_SuccessCount, Data.m_SuccessCount);
	TEST_EQUAL(Read.m_LastOutput, Data.m_LastOutput);
	TEST_EQUAL(Read.m_TrackOutput, Data.m_TrackOutput);
	TEST_EQUAL(Read.m_Powered, Data.m_Powered);
	TEST_EQUAL(Read.m_Auto, Data.m_Auto);
	TEST_EQUAL(Read.m_ConditionMet, Data.m_ConditionMet);
	TEST_EQUAL(Read.m_UpdateLastExecution, Data.m_UpdateLastExecution);
	TEST_EQUAL(Read.m_LastExecution, Data.m_LastExecution);
}





/** Tests that an empty compound leaves the vanilla defaults in place. */
static void testDefaults(void)
{
	cFastNBTWriter Writer;
	Writer.Finish();

	cParsedNBT NBT(Writer.GetResult());
	TEST_TRUE(NBT.IsValid());

	sCommandBlockData Read;
	ReadCommandBlockNBT(NBT, NBT.GetRoot(), Read);

	TEST_EQUAL(Read.m_Command, AString());
	TEST_EQUAL(Read.m_SuccessCount, 0);
	TEST_EQUAL(Read.m_LastOutput, AString());
	TEST_TRUE(Read.m_TrackOutput);
	TEST_FALSE(Read.m_Powered);
	TEST_FALSE(Read.m_Auto);
	TEST_TRUE(Read.m_ConditionMet);
	TEST_TRUE(Read.m_UpdateLastExecution);
	TEST_EQUAL(Read.m_LastExecution, 0LL);
}





IMPLEMENT_TEST_MAIN("CommandBlockNBT",
	testRoundTrip();
	testDefaults();
)
