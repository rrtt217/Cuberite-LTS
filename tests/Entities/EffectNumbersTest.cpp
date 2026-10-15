#include "Globals.h"
#include "Entities/EffectNumbers.h"
#include "../TestHelpers.h"

/** What this test verifies:
(1) The instant health amounts of specs/status-effects/vanilla-1.12.2-status-effect-numbers.md section 1: 4 * 2^amplifier -
    4 / 8 / 16 / 32 for levels I..IV (wiki "Potion": Potion of Healing "Restores 4 health", Healing II
    "Restores 8 health (doubles with every additional level)"; the History of "Potion of Healing" dates the
    33.3% nerf from 6 to 4 to 13w23a).  The defect this removes: healing used to share the Harming base of 6
    and was therefore always 1.5x too strong;
(2) The instant damage amounts of that section: 6 * 2^amplifier - 6 / 12 / 24 / 48 (wiki "Potion": Potion
    of Harming "Deals 6 health");
(3) That the two effects no longer share one base value, so the fix cannot silently regress to the shared
    constant;
(4) The Hunger exhaustion rate of section 2: 0.005 per tick per level, i.e. 3.0 over the 600 ticks of a
    Hunger I effect and 0.1 per second (wiki "Hunger (effect)": "increases food exhaustion by 0.005 x level
    per game tick", cross-checked by the same page's 30-second total of 3.0);
(5) The out-of-range policy of section 4: the doubling saturates at MAX_INSTANT_EFFECT_AMOUNT instead of
    shifting out of range, so an amplifier of 255 does not overflow and an out-of-range amplifier can never
    come back as a negative amount - which would turn a heal into damage.

Invariants:
- Every amount is strictly positive for every amplifier the protocol can carry (0..255);
- The amount is non-decreasing in the amplifier and exactly doubles per level while it does not saturate;
- Healing and damage differ by exactly the ratio of their base values, 4 : 6.

Why: these amounts are the numerically testable core of the three effects.  Applying them needs a cPawn, a
cWorld and a protocol client, and the final "do not heal above the maximum health" clamp is cEntity::Heal's
job (src/Entities/Entity.cpp), which a pure test cannot reach; the arithmetic is therefore extracted into
src/Entities/EffectNumbers.h and tested here. */

/** The tolerance for comparing the rule constants' doubles: the only values compared below are around 0.005
and 3.0, so an absolute tolerance of 1e-12 is far above the rounding of a couple of multiplications and far
below any difference that would matter for a food exhaustion rate. */
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

/** The tick count the cross-check of the Hunger rate is quoted for: a Hunger I effect from rotten flesh or
raw chicken lasts 600 ticks = 30 seconds (src/Items/ItemRottenFlesh.h, src/Items/ItemRawChicken.h). */
static constexpr int HUNGER_I_EFFECT_TICKS = 600;

/** The number of ticks in one second, for the per-second form of the same rate. */
static constexpr int TICKS_PER_SECOND = 20;

/** The highest amplifier the 1.12.2 protocol can carry for an effect, EffectSpeedRules.h
MAX_EFFECT_AMPLIFIER: the wiki's "Effect potency" gives "a maximum potency of 256, which is also
represented as an amplifier of 0 up to 255". */
static constexpr int TEST_MAX_AMPLIFIER = 255;





/** Tests the instant health sequence, spec section 1: 4 / 8 / 16 / 32 for amplifiers 0..3. */
static void testInstantHealthSequence(void)
{
	TEST_EQUAL(InstantHealthAmount(0), 4);
	TEST_EQUAL(InstantHealthAmount(1), 8);
	TEST_EQUAL(InstantHealthAmount(2), 16);
	TEST_EQUAL(InstantHealthAmount(3), 32);

	// Every level doubles the previous one:
	TEST_EQUAL(InstantHealthAmount(1), 2 * InstantHealthAmount(0));
	TEST_EQUAL(InstantHealthAmount(2), 2 * InstantHealthAmount(1));
	TEST_EQUAL(InstantHealthAmount(3), 2 * InstantHealthAmount(2));
}





/** Tests the instant damage sequence, spec section 1: 6 / 12 / 24 / 48 for amplifiers 0..3. */
static void testInstantDamageSequence(void)
{
	TEST_EQUAL(InstantDamageAmount(0), 6);
	TEST_EQUAL(InstantDamageAmount(1), 12);
	TEST_EQUAL(InstantDamageAmount(2), 24);
	TEST_EQUAL(InstantDamageAmount(3), 48);

	// Every level doubles the previous one:
	TEST_EQUAL(InstantDamageAmount(1), 2 * InstantDamageAmount(0));
	TEST_EQUAL(InstantDamageAmount(2), 2 * InstantDamageAmount(1));
	TEST_EQUAL(InstantDamageAmount(3), 2 * InstantDamageAmount(2));

	// Harming II is the 12 the wiki quotes for it:
	TEST_EQUAL(InstantDamageAmount(1), 12);
}





/** Tests that healing and harm no longer share one hardcoded base value - the defect of section 1. */
static void testHealingIsNotHarming(void)
{
	// Healing is 4 * 2^n, harm is 6 * 2^n, so their ratio is 4 / 6 at every (non-saturated) level:
	TEST_EQUAL(InstantHealthAmount(0), 4);
	TEST_EQUAL(InstantDamageAmount(0), 6);
	TEST_NOTEQUAL(InstantHealthAmount(0), InstantDamageAmount(0));

	TEST_EQUAL(3 * InstantHealthAmount(0), 2 * InstantDamageAmount(0));
	TEST_EQUAL(3 * InstantHealthAmount(1), 2 * InstantDamageAmount(1));
	TEST_EQUAL(3 * InstantHealthAmount(2), 2 * InstantDamageAmount(2));
}





/** Tests the out-of-range policy of spec section 4: saturating doubling, never an overflow or a negative. */
static void testSaturation(void)
{
	// A negative amplifier is a caller error; it is clamped to level I rather than asking for a fractional
	// amount:
	TEST_EQUAL(InstantHealthAmount(-1), INSTANT_HEALTH_BASE_RECOVERY);
	TEST_EQUAL(InstantDamageAmount(-1), INSTANT_DAMAGE_BASE_DAMAGE);

	// The largest amplifier the protocol can carry must not overflow into a negative amount:
	TEST_EQUAL(InstantHealthAmount(TEST_MAX_AMPLIFIER), MAX_INSTANT_EFFECT_AMOUNT);
	TEST_EQUAL(InstantDamageAmount(TEST_MAX_AMPLIFIER), MAX_INSTANT_EFFECT_AMOUNT);
	TEST_GREATER_THAN_OR_EQUAL(InstantHealthAmount(TEST_MAX_AMPLIFIER), 1);
	TEST_GREATER_THAN_OR_EQUAL(InstantDamageAmount(TEST_MAX_AMPLIFIER), 1);

	// Nothing the protocol can carry is ever negative, and the amount never decreases with the amplifier -
	// a wrapped negative heal would damage its target:
	int PreviousHealth = 0;
	int PreviousDamage = 0;
	for (int Amplifier = 0; Amplifier <= TEST_MAX_AMPLIFIER; ++Amplifier)
	{
		const int Health = InstantHealthAmount(Amplifier);
		const int Damage = InstantDamageAmount(Amplifier);
		TEST_GREATER_THAN_OR_EQUAL(Health, 1);
		TEST_GREATER_THAN_OR_EQUAL(Damage, 1);
		TEST_GREATER_THAN_OR_EQUAL(Health, PreviousHealth);
		TEST_GREATER_THAN_OR_EQUAL(Damage, PreviousDamage);
		PreviousHealth = Health;
		PreviousDamage = Damage;
	}

	// The doubling is exact right up to the last level that still fits into an int: level 29 (amplifier 28)
	// is 4 * 2^28 and level 30 (amplifier 29) would exceed it and saturates:
	TEST_EQUAL(InstantHealthAmount(27), 4 * (1 << 27));
	TEST_EQUAL(InstantHealthAmount(28), 1 << 30);
	TEST_EQUAL(InstantHealthAmount(29), MAX_INSTANT_EFFECT_AMOUNT);
}





/** Tests the Hunger exhaustion rate, spec section 2: 0.005 per tick per level. */
static void testHungerExhaustionRate(void)
{
	TEST_NEAR(HUNGER_EXHAUSTION_PER_TICK_PER_LEVEL, 0.005);

	// A Hunger I effect (amplifier 0, level 1) over its 600 ticks totals the 3.0 the wiki quotes:
	TEST_NEAR(HUNGER_EXHAUSTION_PER_TICK_PER_LEVEL * 1 * HUNGER_I_EFFECT_TICKS, 3.0);

	// Per second at level I, that is 0.005 * 20 = 0.1 - the code comment used to claim 0.5, five times too
	// much:
	TEST_NEAR(HUNGER_EXHAUSTION_PER_TICK_PER_LEVEL * TICKS_PER_SECOND, 0.1);

	// The rate scales with the level, so Hunger II (amplifier 1) doubles it:
	TEST_NEAR(HUNGER_EXHAUSTION_PER_TICK_PER_LEVEL * 2, 0.01);
}





/** Tests the named constants the rules are built from, so a typo in one of them is caught here. */
static void testConstants(void)
{
	TEST_EQUAL(INSTANT_HEALTH_BASE_RECOVERY, 4);
	TEST_EQUAL(INSTANT_DAMAGE_BASE_DAMAGE, 6);
	TEST_NEAR(HUNGER_EXHAUSTION_PER_TICK_PER_LEVEL, 0.005);
	TEST_EQUAL(TEST_MAX_AMPLIFIER, MAX_EFFECT_AMPLIFIER);
}





IMPLEMENT_TEST_MAIN("EffectNumbersTest",
	testInstantHealthSequence();
	testInstantDamageSequence();
	testHealingIsNotHarming();
	testSaturation();
	testHungerExhaustionRate();
	testConstants();
)
