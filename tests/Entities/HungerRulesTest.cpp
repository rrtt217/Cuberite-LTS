#include "Globals.h"
#include "Entities/HungerRules.h"
#include "../TestHelpers.h"

/** What this test verifies:
The hunger / saturation arithmetic of specs/status-effects/vanilla-1.12.2-status-effect-saturation.md section 2:
(1) The per-level scaling - amplifier 0 is level I, so the deltas are +1 hunger and +2 saturation per
    level (sources: https://minecraft.wiki/w/Potion, "Saturation ... Restores 1 hunger point x level and
    2 x level points of saturation", and https://minecraft.wiki/w/Saturation_(effect)); the pre-existing
    code added a flat 1 + amplifier saturation and no hunger at all;
(2) The clamp invariants of that section (https://minecraft.wiki/w/Food#Saturation, saturation "can not
    exceed the food level"): hunger stays in [0, 20] and saturation in [0, hunger];
(3) That both deltas stay positive for every amplifier the server can store, i.e. the effect never turns
    into a drain (spec section 5 leaves negative amplifiers unimplemented).

Invariants:
- The deltas are linear in the level, and Saturation is exactly twice Hunger;
- After ClampFoodState, 0 <= saturation <= hunger <= MAX_HUNGER_LEVEL, for any input;
- A settlement at hunger 0 always leaves at least 1 hunger (the clamp cannot eat the gain).

Why: cEntityEffectSaturation::OnTick applies the effect to a live cPlayer with a world and a client
connection, none of which exists without a running server; the arithmetic is therefore extracted into
pure functions in src/Entities/HungerRules.h and tested here. */

/** The tolerance for comparing the rule functions' doubles: every value compared below is a whole number
below 600, so 1e-12 is far above double rounding and far below any difference that would matter. */
static constexpr double TEST_DOUBLE_TOLERANCE = 1e-12;

/** Checks that the two values are doubles within TEST_DOUBLE_TOLERANCE of each other. */
#define TEST_NEAR(VAL1, VAL2) \
	do \
	{ \
		const double Actual = (VAL1); \
		const double Expected = (VAL2); \
		if (std::abs(Actual - Expected) > TEST_DOUBLE_TOLERANCE) \
		{ \
			throw TestException( \
				__FILE__, __LINE__, __FUNCTION__, \
				fmt::format(FMT_STRING("Near-equality test failed: {} != {}"), Actual, Expected) \
			); \
		} \
	} while (false)





/** Tests that the amplifier is converted to a level and the delta scales with it (spec 2). */
static void testLevelScaling(void)
{
	// Amplifier 0 is level I: +1 hunger and +2 saturation, never zero:
	const EffectHungerDelta Level1 = SaturationEffectDelta(0);
	TEST_EQUAL(Level1.Hunger, 1);
	TEST_NEAR(Level1.Saturation, 2.0);

	// Level II:
	const EffectHungerDelta Level2 = SaturationEffectDelta(1);
	TEST_EQUAL(Level2.Hunger, 2);
	TEST_NEAR(Level2.Saturation, 4.0);

	// Level V:
	const EffectHungerDelta Level5 = SaturationEffectDelta(4);
	TEST_EQUAL(Level5.Hunger, 5);
	TEST_NEAR(Level5.Saturation, 10.0);

	// Saturation is exactly twice the hunger at every level, and both are linear in the level:
	TEST_NEAR(Level5.Saturation, 2 * static_cast<double>(Level5.Hunger));
	TEST_EQUAL(Level5.Hunger, 5 * Level1.Hunger);

	// The top of the range 1.12.2 stores, amplifier 255 = level 256:
	TEST_EQUAL(SaturationEffectDelta(MAX_EFFECT_AMPLIFIER).Hunger, MAX_EFFECT_AMPLIFIER + 1);
}





/** Tests the clamp invariants of spec 2 for the states the effect can settle from. */
static void testClampInvariants(void)
{
	// Saturation is capped at the hunger value - not at the old flat 1 + amplifier figure:
	int Hunger = 6;
	double Saturation = 100.0;
	ClampFoodState(Hunger, Saturation);
	TEST_EQUAL(Hunger, 6);
	TEST_NEAR(Saturation, 6.0);
	TEST_LESS_THAN_OR_EQUAL(Saturation, static_cast<double>(Hunger));

	// Hunger is capped at MAX_HUNGER_LEVEL, and saturation at a full bar stays at that cap:
	Hunger = 100;
	Saturation = 100.0;
	ClampFoodState(Hunger, Saturation);
	TEST_EQUAL(Hunger, MAX_HUNGER_LEVEL);
	TEST_NEAR(Saturation, static_cast<double>(MAX_HUNGER_LEVEL));

	// Neither value can go negative:
	Hunger = -5;
	Saturation = -5.0;
	ClampFoodState(Hunger, Saturation);
	TEST_EQUAL(Hunger, 0);
	TEST_NEAR(Saturation, 0.0);
}





/** Tests that a settlement at hunger 0 raises the hunger to at least 1 (spec 4). */
static void testSettlementRaisesZeroHunger(void)
{
	const EffectHungerDelta Delta = SaturationEffectDelta(0);
	int Hunger = 0;
	double Saturation = 0.0;
	Hunger += Delta.Hunger;
	Saturation += Delta.Saturation;
	ClampFoodState(Hunger, Saturation);

	TEST_EQUAL(Hunger, 1);
	TEST_NEAR(Saturation, 1.0);
	TEST_GREATER_THAN_OR_EQUAL(Hunger, 1);
}





/** Tests that a settlement at a nearly full bar can fill both hunger and saturation to the cap. */
static void testSettlementAtFullHunger(void)
{
	const EffectHungerDelta Delta = SaturationEffectDelta(0);
	int Hunger = MAX_HUNGER_LEVEL - 1;
	double Saturation = static_cast<double>(MAX_HUNGER_LEVEL - 1);
	Hunger += Delta.Hunger;
	Saturation += Delta.Saturation;
	ClampFoodState(Hunger, Saturation);

	TEST_EQUAL(Hunger, MAX_HUNGER_LEVEL);
	TEST_NEAR(Saturation, static_cast<double>(MAX_HUNGER_LEVEL));
}





/** Tests that the effect only ever adds, for every amplifier the server stores (spec 5). */
static void testDeltaDirectionIsStable(void)
{
	for (int Amplifier = 0; Amplifier <= MAX_EFFECT_AMPLIFIER; ++Amplifier)
	{
		const EffectHungerDelta Delta = SaturationEffectDelta(Amplifier);
		TEST_GREATER_THAN_OR_EQUAL(Delta.Hunger, 1);
		TEST_GREATER_THAN_OR_EQUAL(Delta.Saturation, 2.0);
	}
}





/** Tests the named constants the rules are built from. */
static void testConstants(void)
{
	TEST_EQUAL(MAX_HUNGER_LEVEL, 20);
}





IMPLEMENT_TEST_MAIN("HungerRulesTest",
	testLevelScaling();
	testClampInvariants();
	testSettlementRaisesZeroHunger();
	testSettlementAtFullHunger();
	testDeltaDirectionIsStable();
	testConstants();
)
