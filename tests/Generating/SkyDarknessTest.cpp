#include "Globals.h"
#include "SkyDarkness.h"
#include "../TestHelpers.h"

// SkyDarknessTest
//
// Verifies the vanilla 1.12.2 sky darkness (the amount subtracted from stored skylight to get the internal
// skylight) as specified in specs/lighting/vanilla-1.12.2-lighting.md, section 4 (sourced from the Minecraft Wiki
// "Light" page, 2018-11-24 revision, and the "Internal light level" section of the current page).
//
// Invariants checked:
//   - clear weather: darkness 0 at day, growing 0 -> 11 over dusk (12000 -> 13187), constant 11 through the
//     night (13187 -> 22812, midnight internal skylight 15 - 11 = 4, the moonlight), shrinking 11 -> 0 over
//     dawn (22812 -> 23999) - fixes spec deviation 6 (constant 8 and reversed dusk);
//   - the ramps are monotonic and continuous with the flat segments, no jumps at the segment boundaries;
//   - rain adds darkness 3 and thunderstorm adds 5 (noon internal skylight 12 / 10), capped at 11, so rain
//     at noon is exactly 3 - fixes spec deviation 7 (no weather darkness at all);
//   - the total always fits a nibble, so the unsigned NIBBLETYPE result can never wrap around.
//
// Why verified this way: the darkness calculation is a pure function of (tick of day, weather) extracted
// into SkyDarkness.h, so direct point, monotonicity and boundary assertions are the strongest oracle
// available without a live vanilla server (spec section 7).





/** Ticks of day used by the assertions, named so that they carry no magic numbers. */
static constexpr cTickTime TEST_TICKS_DAY_START(0);
static constexpr cTickTime TEST_TICKS_NOON(6000);
static constexpr cTickTime TEST_TICKS_SUNSET(12000);
static constexpr cTickTime TEST_TICKS_DUSK_START(12001);
static constexpr cTickTime TEST_TICKS_DUSK_REACHES_MAX(13177);
static constexpr cTickTime TEST_TICKS_NIGHT_START(13187);
static constexpr cTickTime TEST_TICKS_NIGHT_START_NEXT(13188);
static constexpr cTickTime TEST_TICKS_MIDNIGHT(18000);
static constexpr cTickTime TEST_TICKS_NIGHT_END(22812);
static constexpr cTickTime TEST_TICKS_DAWN_START(22813);
static constexpr cTickTime TEST_TICKS_SUNRISE(23999);
static constexpr cTickTime TEST_TICKS_MID_DUSK(12535);

/** The sky darkness levels used by the assertions. */
static constexpr NIBBLETYPE TEST_DARKNESS_NONE = 0;
static constexpr NIBBLETYPE TEST_DARKNESS_MAX = 11;
static constexpr NIBBLETYPE TEST_DARKNESS_RAIN = 3;
static constexpr NIBBLETYPE TEST_DARKNESS_STORM = 5;
static constexpr NIBBLETYPE TEST_DARKNESS_MIDDUSK = 5;
static constexpr NIBBLETYPE TEST_DARKNESS_RAIN_MIDDUSK = 8;
static constexpr NIBBLETYPE TEST_DARKNESS_STORM_MIDDUSK = 10;

/** Boundary ticks of the dusk / night / dawn segments, mirrors the spec's section-4 table. */
static constexpr Int64 TEST_BOUNDARY_SUNSET = 12000;
static constexpr Int64 TEST_BOUNDARY_NIGHT_START = 13187;
static constexpr Int64 TEST_BOUNDARY_NIGHT_END = 22812;
static constexpr Int64 TEST_BOUNDARY_SUNRISE = 23999;





/** Wraps a raw tick-of-day count into the cTickTime type the darkness functions expect. */
static cTickTime MakeTime(Int64 a_Tick)
{
	return cTickTime(static_cast<int>(a_Tick));
}





/** Checks the clear-weather darkness of the documented time points, spec deviation 6. */
static void testTimePointsSunny(void)
{
	// Day: no darkness at all:
	TEST_EQUAL(CalcSkyDarkness(TEST_TICKS_DAY_START, wSunny), TEST_DARKNESS_NONE);
	TEST_EQUAL(CalcSkyDarkness(TEST_TICKS_NOON, wSunny), TEST_DARKNESS_NONE);
	TEST_EQUAL(CalcSkyDarkness(TEST_TICKS_SUNSET, wSunny), TEST_DARKNESS_NONE);

	// Dusk starts at zero (the old code started at 8 and went down) and grows:
	TEST_EQUAL(CalcSkyDarkness(TEST_TICKS_DUSK_START, wSunny), TEST_DARKNESS_NONE);
	TEST_EQUAL(CalcSkyDarkness(TEST_TICKS_MID_DUSK, wSunny), TEST_DARKNESS_MIDDUSK);
	TEST_EQUAL(CalcSkyDarkness(TEST_TICKS_DUSK_REACHES_MAX, wSunny), TEST_DARKNESS_MAX);

	// Night: maximum darkness 11 all through (midnight internal skylight 15 - 11 = 4, the moonlight):
	TEST_EQUAL(CalcSkyDarkness(TEST_TICKS_NIGHT_START, wSunny), TEST_DARKNESS_MAX);
	TEST_EQUAL(CalcSkyDarkness(TEST_TICKS_NIGHT_START_NEXT, wSunny), TEST_DARKNESS_MAX);
	TEST_EQUAL(CalcSkyDarkness(TEST_TICKS_MIDNIGHT, wSunny), TEST_DARKNESS_MAX);
	TEST_EQUAL(CalcSkyDarkness(TEST_TICKS_NIGHT_END, wSunny), TEST_DARKNESS_MAX);

	// Dawn: starts at the maximum and shrinks back to zero by the day wrap:
	TEST_EQUAL(CalcSkyDarkness(TEST_TICKS_DAWN_START, wSunny), TEST_DARKNESS_MAX);
	TEST_EQUAL(CalcSkyDarkness(TEST_TICKS_SUNRISE, wSunny), TEST_DARKNESS_NONE);
}





/** Checks that the dusk and dawn ramps are monotonic, continuous with the flat segments and within bounds. */
static void testRampsMonotonicAndContinuous(void)
{
	// Dusk, ticks 12000 .. 13187: the darkness must never decrease and never exceed the maximum:
	int Prev = GetTimeSkyDarkness(MakeTime(TEST_BOUNDARY_SUNSET));
	for (int Tick = static_cast<int>(TEST_BOUNDARY_SUNSET) + 1; Tick <= static_cast<int>(TEST_BOUNDARY_NIGHT_START); Tick += 1)
	{
		const int Cur = GetTimeSkyDarkness(MakeTime(Tick));
		TEST_GREATER_THAN_OR_EQUAL(Cur, Prev);
		TEST_LESS_THAN_OR_EQUAL(Cur, TEST_DARKNESS_MAX);
		Prev = Cur;
	}

	// Dawn, ticks 22812 .. 23999: the darkness must never increase and never go below zero:
	Prev = GetTimeSkyDarkness(MakeTime(TEST_BOUNDARY_NIGHT_END));
	for (int Tick = static_cast<int>(TEST_BOUNDARY_NIGHT_END) + 1; Tick <= static_cast<int>(TEST_BOUNDARY_SUNRISE); Tick += 1)
	{
		const int Cur = GetTimeSkyDarkness(MakeTime(Tick));
		TEST_LESS_THAN_OR_EQUAL(Cur, Prev);
		TEST_GREATER_THAN_OR_EQUAL(Cur, TEST_DARKNESS_NONE);
		Prev = Cur;
	}

	// Continuity at the segment boundaries and at the day wrap, no jumps:
	TEST_EQUAL(GetTimeSkyDarkness(MakeTime(TEST_BOUNDARY_NIGHT_START)), GetTimeSkyDarkness(MakeTime(TEST_BOUNDARY_NIGHT_START + 1)));
	TEST_EQUAL(GetTimeSkyDarkness(MakeTime(TEST_BOUNDARY_NIGHT_END)), GetTimeSkyDarkness(MakeTime(TEST_BOUNDARY_NIGHT_END + 1)));
	TEST_EQUAL(GetTimeSkyDarkness(MakeTime(TEST_BOUNDARY_SUNRISE)), GetTimeSkyDarkness(TEST_TICKS_DAY_START));
}





/** Checks the weather darkness on top of the time darkness, spec deviation 7. */
static void testWeatherDarkness(void)
{
	// Noon, the documented point: sunny 0 (internal 15), rain 3 (internal 12), thunderstorm 5 (internal 10):
	TEST_EQUAL(CalcSkyDarkness(TEST_TICKS_NOON, wSunny), TEST_DARKNESS_NONE);
	TEST_EQUAL(CalcSkyDarkness(TEST_TICKS_NOON, wRain), TEST_DARKNESS_RAIN);
	TEST_EQUAL(CalcSkyDarkness(TEST_TICKS_NOON, wStorm), TEST_DARKNESS_STORM);

	// Mid-dusk the time part is 5, so rain adds exactly 3 and thunderstorm exactly 5:
	TEST_EQUAL(CalcSkyDarkness(TEST_TICKS_MID_DUSK, wRain), TEST_DARKNESS_RAIN_MIDDUSK);
	TEST_EQUAL(CalcSkyDarkness(TEST_TICKS_MID_DUSK, wStorm), TEST_DARKNESS_STORM_MIDDUSK);

	// At night the time part already sits at the maximum, so the total stays capped at 11 (moonlight floor 4):
	TEST_EQUAL(CalcSkyDarkness(TEST_TICKS_MIDNIGHT, wRain), TEST_DARKNESS_MAX);
	TEST_EQUAL(CalcSkyDarkness(TEST_TICKS_MIDNIGHT, wStorm), TEST_DARKNESS_MAX);
}





/** Checks that with any weather the darkness stays within a nibble across a whole day. */
static void testBoundsFullDay(void)
{
	for (int Tick = 0; Tick <= static_cast<int>(TEST_BOUNDARY_SUNRISE); Tick += 1)
	{
		const cTickTime TimeOfDay = MakeTime(Tick);
		TEST_LESS_THAN_OR_EQUAL(CalcSkyDarkness(TimeOfDay, wSunny), TEST_DARKNESS_MAX);
		TEST_LESS_THAN_OR_EQUAL(CalcSkyDarkness(TimeOfDay, wRain), TEST_DARKNESS_MAX);
		TEST_LESS_THAN_OR_EQUAL(CalcSkyDarkness(TimeOfDay, wStorm), TEST_DARKNESS_MAX);

		// Weather never brightens the sky:
		TEST_GREATER_THAN_OR_EQUAL(CalcSkyDarkness(TimeOfDay, wRain), CalcSkyDarkness(TimeOfDay, wSunny));
		TEST_GREATER_THAN_OR_EQUAL(CalcSkyDarkness(TimeOfDay, wStorm), CalcSkyDarkness(TimeOfDay, wRain));
	}
}





IMPLEMENT_TEST_MAIN("SkyDarknessTest",
	testTimePointsSunny();
	testRampsMonotonicAndContinuous();
	testWeatherDarkness();
	testBoundsFullDay();
)
