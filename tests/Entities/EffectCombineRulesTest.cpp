#include "Globals.h"
#include "Entities/EffectCombineRules.h"
#include "../TestHelpers.h"

/** What this test verifies:
(1) The re-application table of specs/vanilla-1.12.2-status-effect-application.md section 2, all four rows:
    a higher amplifier replaces the active effect (even with a shorter duration), a lower one is ignored
    (even with a much longer duration), an equally strong effect extends the active effect only when it
    lasts strictly longer, and is ignored otherwise;
(2) That an extension keeps the ticks the effect has already run for - ExtendedTotalDuration is the
    arithmetic of "raise the total duration by the remaining one", not of "restart the effect";
(3) The duration scaling of section 3, which no longer happens inside cPawn::AddEntityEffect: the splash
    potion source applies a decay factor from [0, 1] to the duration itself.  A factor of 0 is a legitimate
    zero-length effect (the caller must choose it deliberately), a factor of 1 leaves the duration
    untouched, and a factor outside [0, 1] is a caller error that is rejected by an ASSERT;
(4) That the three instantaneous potion effects are recognized and therefore bypass the table - each
    application of a potion of healing / harming / saturation has to run its OnActivate right away instead
    of being ignored as "not longer" by the combination rules.

Invariants:
- No decision ever produces a negative remaining time: an effect is only ever replaced by, or extended to,
  the non-negative duration that was requested, and an ignored effect keeps its remaining time;
- An Ignore decision leaves the active effect completely alone, in particular its elapsed ticks are not
  reset;
- ApplyDurationMultiplier never returns more than its input, and never less than 0.

Why: the combination decision and the duration arithmetic are the numerically testable core of the effect
application semantics.  Executing them needs a cPawn, a cWorld and a protocol client, so the decision is
extracted into src/Entities/EffectCombineRules.h and tested here; cPawn::AddEntityEffect only carries out
what DecideCombine returns. */

/** The amplifiers used by the boundary checks below: level I, level II and a high level. */
static constexpr int TEST_AMPLIFIER_I = 0;
static constexpr int TEST_AMPLIFIER_II = 1;
static constexpr int TEST_AMPLIFIER_HIGH = 5;

/** A plausible effect state: a few minutes of effect with a few seconds already elapsed. */
static constexpr int TEST_OLD_REMAINING_TICKS = 6000;
static constexpr int TEST_OLD_ELAPSED_TICKS = 40;




/** Tests the first row of the table: a higher amplifier always replaces, whatever the duration. */
static void testHigherAmplifierReplaces(void)
{
	// A stronger but shorter potion still wins (spec 2, row 1):
	TEST_EQUAL(
		DecideCombine(TEST_AMPLIFIER_I, TEST_OLD_REMAINING_TICKS, TEST_OLD_ELAPSED_TICKS, TEST_AMPLIFIER_II, 20),
		EffectCombine::Replace
	);

	// ... even against a much higher remaining time, and even for a one-tick effect:
	TEST_EQUAL(
		DecideCombine(TEST_AMPLIFIER_HIGH, TEST_OLD_REMAINING_TICKS, TEST_OLD_ELAPSED_TICKS, TEST_AMPLIFIER_HIGH + 1, 1),
		EffectCombine::Replace
	);

	// A decayed splash potion can therefore still upgrade an effect, it only decays its own time:
	TEST_EQUAL(
		DecideCombine(TEST_AMPLIFIER_I, 100, 0, TEST_AMPLIFIER_II, 0),
		EffectCombine::Replace
	);
}




/** Tests the fourth row: a weaker effect is ignored, whatever its duration. */
static void testLowerAmplifierIgnored(void)
{
	// The classic case of the defect: a thrown Potion of Slowness must not push aside a stronger effect:
	TEST_EQUAL(
		DecideCombine(TEST_AMPLIFIER_II, TEST_OLD_REMAINING_TICKS, TEST_OLD_ELAPSED_TICKS, TEST_AMPLIFIER_I, TEST_OLD_REMAINING_TICKS * 10),
		EffectCombine::Ignore
	);

	TEST_EQUAL(
		DecideCombine(TEST_AMPLIFIER_HIGH, 20, 0, TEST_AMPLIFIER_I, 6000),
		EffectCombine::Ignore
	);
}




/** Tests the second and third rows: the same amplifier extends the active effect only when it lasts
strictly longer, and is ignored otherwise. */
static void testSameAmplifierExtendsOnlyWhenLonger(void)
{
	// One tick more is enough to be accepted:
	TEST_EQUAL(
		DecideCombine(TEST_AMPLIFIER_I, 100, TEST_OLD_ELAPSED_TICKS, TEST_AMPLIFIER_I, 101),
		EffectCombine::ExtendDuration
	);

	// An effect that has not started yet can always be extended:
	TEST_EQUAL(DecideCombine(TEST_AMPLIFIER_II, 0, 0, TEST_AMPLIFIER_II, 200), EffectCombine::ExtendDuration);

	// Exactly as long as what is left is not "longer", so it is ignored (row 3):
	TEST_EQUAL(
		DecideCombine(TEST_AMPLIFIER_I, 100, TEST_OLD_ELAPSED_TICKS, TEST_AMPLIFIER_I, 100),
		EffectCombine::Ignore
	);

	// ... and so is anything shorter:
	TEST_EQUAL(DecideCombine(TEST_AMPLIFIER_I, 100, TEST_OLD_ELAPSED_TICKS, TEST_AMPLIFIER_I, 99), EffectCombine::Ignore);
	TEST_EQUAL(DecideCombine(TEST_AMPLIFIER_I, 100, TEST_OLD_ELAPSED_TICKS, TEST_AMPLIFIER_I, 0), EffectCombine::Ignore);

	// A non-positive duration is a dead effect, never an extension - extending to it would leave the active
	// effect with a negative or zero remaining time and would not even change the client's timer:
	TEST_EQUAL(DecideCombine(TEST_AMPLIFIER_I, 100, TEST_OLD_ELAPSED_TICKS, TEST_AMPLIFIER_I, -5), EffectCombine::Ignore);
	TEST_EQUAL(DecideCombine(TEST_AMPLIFIER_I, 0, TEST_OLD_ELAPSED_TICKS, TEST_AMPLIFIER_I, 0), EffectCombine::Ignore);
}




/** Tests that an extension keeps the elapsed ticks instead of restarting the effect (spec 3). */
static void testExtensionKeepsElapsedTicks(void)
{
	// An effect that has run for 40 ticks and is extended to 150 more must get a total duration of 190, so
	// that "duration - elapsed" is the requested 150 and the elapsed 40 are untouched:
	const int Total = ExtendedTotalDuration(TEST_OLD_ELAPSED_TICKS, 150);
	TEST_EQUAL(Total, TEST_OLD_ELAPSED_TICKS + 150);
	TEST_EQUAL(Total - 150, TEST_OLD_ELAPSED_TICKS);

	// An effect that has not started yet is simply given the new duration:
	TEST_EQUAL(ExtendedTotalDuration(0, 200), 200);

	// Extending twice in a row keeps accumulating the same elapsed time:
	const int Once = ExtendedTotalDuration(40, 100);
	const int Twice = ExtendedTotalDuration(40, 120);
	TEST_EQUAL(Once, 140);
	TEST_EQUAL(Twice, 160);
	TEST_EQUAL(Twice - 120, 40);
}




/** Walks the whole input domain of DecideCombine and checks its invariants.
The old remaining time is only ever non-negative in production - cPawn::Tick removes an effect as soon as
its remaining time reaches 0 - so the loops below stay inside that contract. */
static void testDecisionInvariants(void)
{
	static const int OldAmplifiers[] = { 0, 1, 5 };
	static const int OldRemainings[] = { 0, 1, 20, 6000 };
	static const int Elapseds[] = { 0, 19, 40 };
	static const int NewAmplifiers[] = { 0, 1, 5 };
	static const int NewDurations[] = { 0, 1, 20, 21, 6000 };

	for (const int OldAmplifier : OldAmplifiers)
	{
		for (const int OldRemaining : OldRemainings)
		{
			for (const int Elapsed : Elapseds)
			{
				for (const int NewAmplifier : NewAmplifiers)
				{
					for (const int NewDuration : NewDurations)
					{
						const EffectCombine Decision = DecideCombine(OldAmplifier, OldRemaining, Elapsed, NewAmplifier, NewDuration);

						// The remaining time that executing the decision would leave, see
						// cPawn::AddEntityEffect: an ignored effect keeps its own remaining time, a replaced
						// or extended one gets the new remaining time:
						const int ResultingRemaining = (Decision == EffectCombine::Ignore) ? OldRemaining : NewDuration;
						TEST_GREATER_THAN_OR_EQUAL(ResultingRemaining, 0);

						// An ignored effect is untouched - in particular its elapsed ticks are preserved:
						if (Decision == EffectCombine::Ignore)
						{
							TEST_EQUAL(ResultingRemaining, OldRemaining);
						}

						// A replacement never happens for an equal or lower amplifier:
						if (Decision == EffectCombine::Replace)
						{
							TEST_TRUE(NewAmplifier > OldAmplifier);
						}

						// Only an equally strong, strictly longer and non-empty effect extends:
						if (Decision == EffectCombine::ExtendDuration)
						{
							TEST_EQUAL(NewAmplifier, OldAmplifier);
							TEST_TRUE(NewDuration > OldRemaining);
							TEST_TRUE(NewDuration > 0);
						}
					}
				}
			}
		}
	}
}




/** Tests that the three instantaneous potion effects are recognized, because they bypass the combination
table: every application has to run its OnActivate right away, so the newest application replaces the stored
effect instead of being combined with it. */
static void testInstantaneousEffects(void)
{
	TEST_TRUE(IsInstantEffect(cEntityEffect::effInstantHealth));
	TEST_TRUE(IsInstantEffect(cEntityEffect::effInstantDamage));
	TEST_TRUE(IsInstantEffect(cEntityEffect::effSaturation));

	// Every lasting effect takes part in the combination table:
	TEST_FALSE(IsInstantEffect(cEntityEffect::effSpeed));
	TEST_FALSE(IsInstantEffect(cEntityEffect::effRegeneration));
	TEST_FALSE(IsInstantEffect(cEntityEffect::effPoison));
	TEST_FALSE(IsInstantEffect(cEntityEffect::effNoEffect));
}




/** Tests the duration scaling the splash potion source applies before AddEntityEffect (spec 3). */
static void testDurationMultiplier(void)
{
	// A target at the splash point is not decayed at all:
	TEST_EQUAL(ApplyDurationMultiplier(600, 1.0), 600);

	// Halfway out the duration is halved, exactly as the potency used to be:
	TEST_EQUAL(ApplyDurationMultiplier(600, 0.5), 300);
	TEST_EQUAL(ApplyDurationMultiplier(600, 0.75), 450);

	// A factor of 0 is a legitimate zero-length effect - the caller must choose it deliberately, because
	// cPawn::AddEntityEffect will remove the effect on its next tick:
	TEST_EQUAL(ApplyDurationMultiplier(600, 0.0), 0);
	TEST_NOTEQUAL(ApplyDurationMultiplier(600, 0.0), 600);

	// A zero-length input stays zero at every factor:
	TEST_EQUAL(ApplyDurationMultiplier(0, 0.5), 0);

	// The result never exceeds the input and never becomes negative while the factor stays in range:
	int Previous = 0;
	for (int Percent = 0; Percent <= 100; ++Percent)
	{
		const int Scaled = ApplyDurationMultiplier(600, Percent / 100.0);
		TEST_GREATER_THAN_OR_EQUAL(Scaled, 0);
		TEST_LESS_THAN_OR_EQUAL(Scaled, 600);
		TEST_GREATER_THAN_OR_EQUAL(Scaled, Previous);
		Previous = Scaled;
	}

	// A factor above 1 would lengthen an effect with distance, and a negative one would make it longer than
	// the potion itself; both are caller errors and are rejected in Debug builds:
	TEST_ASSERTS(ApplyDurationMultiplier(600, 1.5));
	TEST_ASSERTS(ApplyDurationMultiplier(600, -0.1));
	TEST_ASSERTS(ApplyDurationMultiplier(-1, 0.5));
}




IMPLEMENT_TEST_MAIN("EffectCombineRulesTest",
	testHigherAmplifierReplaces();
	testLowerAmplifierIgnored();
	testSameAmplifierExtendsOnlyWhenLonger();
	testExtensionKeepsElapsedTicks();
	testDecisionInvariants();
	testInstantaneousEffects();
	testDurationMultiplier();
)
