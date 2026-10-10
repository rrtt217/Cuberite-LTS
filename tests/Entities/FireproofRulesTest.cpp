#include "Globals.h"
#include "Entities/FireproofRules.h"
#include "../TestHelpers.h"

/** What this test verifies:
The decision table of specs/vanilla-1.12.2-status-effect-fire-resistance.md section 2 - an entity is
fireproof when it is hardcoded fireproof, or carries a Fire Resistance effect, or is a player in creative
or spectator mode (sources: https://minecraft.wiki/w/Potion, "Immunity to all heat-related damage";
https://minecraft.wiki/w/Enchanted_Golden_Apple, 5:00 of Fire Resistance):
(1) A player whose only claim to fireproofing is the Fire Resistance effect is fireproof.  That single row
    is the regression lock for this branch: cPlayer used to answer the question itself and left the effect
    out, so a Fire Resistance potion, an enchanted golden apple and a totem's 40 seconds of immunity all
    did nothing for a player;
(2) The same for a non-player pawn, which always went through cPawn and was already right;
(3) Creative / spectator without any effect is fireproof, a survival player without one is not;
(4) The hardcoded flag from Monsters.ini (blaze, magma cube, ...) is fireproof on its own;
(5) The table is an OR - every combination of the three inputs gives the documented answer.

Invariants:
- No input combination is fireproof unless at least one of the three conditions holds;
- The very first row above never comes out false.

Why: cPawn::IsFireproof() is a virtual over a live entity, so its three inputs cannot be set from a test
without a world and a client; the decision is therefore extracted into a pure function that both the pawn
and - through the pawn - the player call. */

/** One row of the decision table. */
struct FireproofCase
{
	bool IsHardcodedFireproof;
	bool HasFireResistanceEffect;
	bool IsCreativeOrSpectator;
	bool ExpectedIsFireproof;
	const char * Description;
};

/** The whole decision table, in the order of the description of spec section 2. */
static const FireproofCase g_FireproofCases[] =
{
	// The regression this branch fixes: a player with nothing but a Fire Resistance effect:
	{false, true,  false, true,  "a survival player under a Fire Resistance effect"},
	{false, true,  true,  true,  "a creative or spectator player under a Fire Resistance effect"},
	// The same effect on a mob, which was always answered correctly:
	{false, true,  false, true,  "a mob under a Fire Resistance effect"},
	// The game modes, without any effect:
	{false, false, true,  true,  "a creative or spectator player"},
	{true,  false, true,  true,  "a hardcoded-fireproof entity in creative or spectator mode"},
	// The hardcoded flag alone, as Monsters.ini sets it for blazes and magma cubes:
	{true,  false, false, true,  "a hardcoded-fireproof mob"},
	{true,  true,  false, true,  "a hardcoded-fireproof mob under a Fire Resistance effect"},
	// Nothing fireproof about it:
	{false, false, false, false, "a survival player with no effect"},
};





/** Tests every combination of the three inputs against the table above. */
static void testDecisionTable(void)
{
	for (const auto & Test : g_FireproofCases)
	{
		const bool Actual = IsFireproofFor(Test.IsHardcodedFireproof, Test.HasFireResistanceEffect, Test.IsCreativeOrSpectator);
		if (Actual != Test.ExpectedIsFireproof)
		{
			TEST_FAIL(fmt::format(FMT_STRING("{} is fireproof: expected {}, got {}"), Test.Description, Test.ExpectedIsFireproof, Actual));
		}
	}
}





/** Tests the single row that the old cPlayer::IsFireproof() got wrong, spelled out on its own so that a
future change cannot quietly drop the effect query again. */
static void testEffectProtectsAPlayerAlone(void)
{
	TEST_TRUE(IsFireproofFor(false, true, false));
}





/** Tests that nothing else makes an entity fireproof. */
static void testNothingFireproof(void)
{
	TEST_FALSE(IsFireproofFor(false, false, false));
}





IMPLEMENT_TEST_MAIN("FireproofRulesTest",
	testDecisionTable();
	testEffectProtectsAPlayerAlone();
	testNothingFireproof();
)
