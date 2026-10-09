#include "Globals.h"
#include "Entities/EffectSpeedRules.h"
#include "../TestHelpers.h"

/** What this test verifies:
(1) The amplifier -> level conversion of specs/vanilla-1.12.2-status-effect-amplifier.md section 2 - an
    amplifier of 0 is level I, so the per-level scaling must never come out as zero (the defect this fix
    removes: Speed I used to add exactly +0.00);
(2) The potency table of that section: Speed = +20% per level (+0.2 / +0.4 / +0.6 / +0.8 / +1.0 for
    amplifiers 0..4, wiki "Speed", section "Effects"), Slowness = -15% per level (-0.15 / -0.30 / -0.45 /
    -0.60 / -0.75, wiki "Slowness", section "Effect");
(3) That sprinting keeps being a fixed 1.3x multiple of walking, so the sprint delta is the walk delta
    times 1.3 - the pre-existing 0.26 and 0.195 constants, now derived instead of hardcoded;
(4) That flying uses the plain walk delta (spec section 6, [needs-check]);
(5) That Slowness IV (amplifier 3) still leaves the entity able to move, and that the model reproduces the
    wiki's "levels 7 and above render the player unable to move" (wiki "Slowness", section "Notes");
(6) That the Bane of Arthropods enchantment inflicts Slowness IV, i.e. the -60% of the wiki's own example
    (wiki "Slowness", the "-60% Speed" tooltip of a Slowness IV potion).

Invariants:
- Level I of either effect is never a no-op;
- The deltas are linear in the level and Slowness is exactly the negative of Speed scaled by 0.75;
- An out-of-range amplifier is rejected by ASSERT (the release clamp behind it is not observable in a
  DEBUG test build, where the assertion fires first).

Why: the deltas are the numerically testable core of the two effects.  Applying them needs a cPawn, a
cWorld and a protocol client, none of which exists without a running server; they are therefore extracted
into pure functions in src/Entities/EffectSpeedRules.h and tested here. */

/** The tolerance for comparing the rule functions' doubles: every value compared below is below 1.5, so an
absolute tolerance of 1e-12 is far above the rounding of two multiplications and far below any difference
that would matter for a movement speed. */
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

/** The player's walking baseline, cPlayer::m_NormalMaxSpeed, and a monster's cMonster::m_RelativeWalkSpeed
baseline - both effects are deltas on top of this relative speed. */
static constexpr double BASE_RELATIVE_SPEED = 1.0;





/** Tests that the amplifier is converted to the level (potency) the spec is written in. */
static void testLevels(void)
{
	// Amplifier 0 is level I, the level the +20% / -15% figures are quoted for:
	TEST_EQUAL(EffectLevelFromAmplifier(0), 1);
	TEST_EQUAL(EffectLevelFromAmplifier(1), 2);
	TEST_EQUAL(EffectLevelFromAmplifier(2), 3);
	TEST_EQUAL(EffectLevelFromAmplifier(3), 4);
	TEST_EQUAL(EffectLevelFromAmplifier(4), 5);

	// The top of the range 1.12.2 stores, an amplifier of 255 = level 256:
	TEST_EQUAL(EffectLevelFromAmplifier(MAX_EFFECT_AMPLIFIER), MAX_EFFECT_AMPLIFIER + 1);

	// Anything outside the stored range is a caller error:
	TEST_ASSERTS(EffectLevelFromAmplifier(-1));
	TEST_ASSERTS(EffectLevelFromAmplifier(MAX_EFFECT_AMPLIFIER + 1));
}





/** Tests the Speed deltas, spec section 2: +20% of the relative speed per level. */
static void testSpeedDeltas(void)
{
	// Speed I - amplifier 0 - must add +20%, not nothing:
	TEST_NEAR(SpeedMaxSpeedDelta(0), 0.2);
	TEST_NEAR(SpeedMaxSpeedDelta(1), 0.4);
	TEST_NEAR(SpeedMaxSpeedDelta(2), 0.6);
	TEST_NEAR(SpeedMaxSpeedDelta(3), 0.8);
	TEST_NEAR(SpeedMaxSpeedDelta(4), 1.0);

	// Sprint is a fixed multiple of walking, so its delta is scaled by the same 1.3:
	TEST_NEAR(SpeedSprintingMaxSpeedDelta(0), 0.26);
	TEST_NEAR(SpeedSprintingMaxSpeedDelta(1), 0.52);
	TEST_NEAR(SpeedSprintingMaxSpeedDelta(2), 0.78);
	TEST_NEAR(SpeedSprintingMaxSpeedDelta(3), 1.04);
	TEST_NEAR(SpeedSprintingMaxSpeedDelta(4), 1.3);

	// Flying has no figure of its own in the sources, it takes the walk delta (spec section 6):
	TEST_NEAR(SpeedFlyingMaxSpeedDelta(0), SpeedMaxSpeedDelta(0));
	TEST_NEAR(SpeedFlyingMaxSpeedDelta(4), SpeedMaxSpeedDelta(4));

	// The delta is linear in the level, and level V is five times level I:
	TEST_NEAR(SpeedMaxSpeedDelta(4), 5 * SpeedMaxSpeedDelta(0));
}





/** Tests the Slowness deltas, spec section 2: -15% of the relative speed per level. */
static void testSlownessDeltas(void)
{
	// Slowness I - amplifier 0 - must take 15% away, not nothing:
	TEST_NEAR(SlownessMaxSpeedDelta(0), -0.15);
	TEST_NEAR(SlownessMaxSpeedDelta(1), -0.30);
	TEST_NEAR(SlownessMaxSpeedDelta(2), -0.45);
	TEST_NEAR(SlownessMaxSpeedDelta(3), -0.60);
	TEST_NEAR(SlownessMaxSpeedDelta(4), -0.75);

	// The same 1.3 sprint scaling as Speed, the pre-existing -0.195 for Slowness I:
	TEST_NEAR(SlownessSprintingMaxSpeedDelta(0), -0.195);
	TEST_NEAR(SlownessSprintingMaxSpeedDelta(1), -0.39);
	TEST_NEAR(SlownessSprintingMaxSpeedDelta(2), -0.585);
	TEST_NEAR(SlownessSprintingMaxSpeedDelta(3), -0.78);
	TEST_NEAR(SlownessSprintingMaxSpeedDelta(4), -0.975);

	// Flying takes the walk delta (spec section 6):
	TEST_NEAR(SlownessFlyingMaxSpeedDelta(0), SlownessMaxSpeedDelta(0));
	TEST_NEAR(SlownessFlyingMaxSpeedDelta(4), SlownessMaxSpeedDelta(4));

	// The delta is linear in the level, and level V is five times level I:
	TEST_NEAR(SlownessMaxSpeedDelta(4), 5 * SlownessMaxSpeedDelta(0));

	// Both effects scale the same quantity, so their per-level ratio is 0.15 / 0.2 = 0.75:
	TEST_NEAR(SlownessMaxSpeedDelta(0) / SpeedMaxSpeedDelta(0), -0.75);
}





/** Tests that a slowed entity keeps moving, and where the linear model meets the wiki's "levels 7 and above
render the player unable to move" (wiki "Slowness", section "Notes"). */
static void testSlownessStillMoves(void)
{
	// Slowness IV, the potency Bane of Arthropods inflicts, leaves 40% of the speed - the same -60% the
	// wiki quotes for a Slowness IV potion:
	TEST_NEAR(BASE_RELATIVE_SPEED + SlownessMaxSpeedDelta(3), 0.4);

	// Slowness VI is the last level that still moves; the wiki's note puts the cut-off at level VII:
	TEST_NEAR(BASE_RELATIVE_SPEED + SlownessMaxSpeedDelta(5), 0.1);

	// Level VII is where the linear model stops giving a positive speed, which is exactly the wiki's
	// "levels 7 and above render the player unable to move":
	TEST_LESS_THAN_OR_EQUAL(BASE_RELATIVE_SPEED + SlownessMaxSpeedDelta(6), 0.0);
}





/** Tests the amplifier the Bane of Arthropods enchantment inflicts (spec section 1, the one call site that
passed a level where an amplifier was expected). */
static void testBaneOfArthropods(void)
{
	// The enchantment inflicts Slowness IV, that is amplifier III:
	TEST_EQUAL(BANE_OF_ARTHROPODS_SLOWNESS_AMPLIFIER, 3);
	TEST_EQUAL(EffectLevelFromAmplifier(BANE_OF_ARTHROPODS_SLOWNESS_AMPLIFIER), 4);

	// Cross-check with the -60% the wiki quotes for Slowness IV:
	TEST_NEAR(SlownessMaxSpeedDelta(BANE_OF_ARTHROPODS_SLOWNESS_AMPLIFIER), -0.60);
}





/** Tests the named constants that the rules are built from. */
static void testConstants(void)
{
	TEST_NEAR(SPEED_SPEEDUP_PER_LEVEL, 0.2);
	TEST_NEAR(SLOWNESS_SLOWDOWN_PER_LEVEL, 0.15);
	TEST_NEAR(SPRINT_MULTIPLIER, 1.3);
	TEST_EQUAL(MAX_EFFECT_AMPLIFIER, 255);
}





IMPLEMENT_TEST_MAIN("EffectSpeedRulesTest",
	testLevels();
	testSpeedDeltas();
	testSlownessDeltas();
	testSlownessStillMoves();
	testBaneOfArthropods();
	testConstants();
)
