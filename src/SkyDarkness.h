// SkyDarkness.h

// Interfaces to the free functions calculating the vanilla sky darkness, the amount subtracted from the
// stored skylight to obtain the internal skylight used by mob spawning, plant growth and the like.
// Kept separate from cWorld so that the calculation is unit-testable without constructing a world.

#pragma once

#include "ChunkDef.h"
#include "Defines.h"





/*
Behavior spec: the lighting spec, file vanilla-1.12.2-lighting.md in the repo's specs folder, section 4
(sourced from the Minecraft Wiki "Light" page, 2018-11-24 revision, and the "Internal light level" section
of the current page):
The stored skylight never changes with time, the darkness is subtracted at the consumers.
Clear weather: darkness 0 during day, ramping up through dusk to 11 at night (midnight internal skylight
15 - 11 = 4, the moonlight) and ramping down through dawn back to 0.
Rain: noon internal skylight 12, i.e. darkness 3; thunderstorm: noon 10, i.e. darkness 5.
The exact composition of the weather darkness with the time darkness is not documented by the sources
(assumption, marked as such in the spec): we add both parts and cap the sum at the documented maximum of
11. That reproduces every documented point (rain noon 3, storm noon 5, clear midnight 11) and keeps the
moonlight floor of 4 even in the darkest weather.
*/

/** The tick of day when dusk begins and the sky darkness starts growing. */
static constexpr Int64 SKY_TIME_SUNSET_TICKS = 12000;

/** The tick of day when night begins and the sky darkness has reached its maximum. */
static constexpr Int64 SKY_TIME_NIGHT_START_TICKS = 13187;

/** The tick of day when dawn begins and the sky darkness starts shrinking again. */
static constexpr Int64 SKY_TIME_NIGHT_END_TICKS = 22812;

/** The last tick before the day wraps around, the sky darkness has reached zero again. */
static constexpr Int64 SKY_TIME_SUNRISE_TICKS = 23999;

/** The highest sky darkness vanilla ever reaches, on clear midnight (spec section 4, rule 1). */
static constexpr int SKY_DARKNESS_MAX = 11;

/** The weather driven part of the sky darkness during rain (spec section 4, rule 2). */
static constexpr int SKY_DARKNESS_RAIN = 3;

/** The weather driven part of the sky darkness during thunderstorm (spec section 4, rule 2). */
static constexpr int SKY_DARKNESS_STORM = 5;

/** Number of dusk / dawn ticks per one level of sky darkness.
Both ramps span 1187 ticks (13187 - 12000, mirrored 23999 - 22812) while the darkness moves through
SKY_DARKNESS_MAX (11) levels, that is 1187 / 11 = 107.9 ticks per level; truncating to 107 makes the ramp
reach the maximum at tick 12000 + 107 * 11 = 13177, still inside dusk, so the growing ramp is continuous
with the constant nighttime darkness (using 108 would only reach 11 after night start and leave a hole). */
static constexpr Int64 SKY_DARKNESS_TICKS_PER_LEVEL = 107;





/** Returns the time-of-day driven part of the sky darkness, 0 .. SKY_DARKNESS_MAX, for the given tick of day.
The tick is expected in the range 0 .. 23999 as returned by cWorld::GetTimeOfDay(), larger values clamp to 0. */
inline int GetTimeSkyDarkness(cTickTime a_TimeOfDay)
{
	const Int64 Tick = a_TimeOfDay.count();
	if (Tick <= SKY_TIME_SUNSET_TICKS)
	{
		return 0;
	}
	if (Tick <= SKY_TIME_NIGHT_START_TICKS)
	{
		return static_cast<int>((Tick - SKY_TIME_SUNSET_TICKS) / SKY_DARKNESS_TICKS_PER_LEVEL);
	}
	if (Tick <= SKY_TIME_NIGHT_END_TICKS)
	{
		return SKY_DARKNESS_MAX;
	}
	if (Tick >= SKY_TIME_SUNRISE_TICKS)
	{
		// The last tick of dawn already has darkness zero again, everything beyond is out of range:
		return 0;
	}
	return static_cast<int>((SKY_TIME_SUNRISE_TICKS - Tick) / SKY_DARKNESS_TICKS_PER_LEVEL);
}





/** Returns the weather driven part of the sky darkness: sunny 0, rain 3, thunderstorm 5. */
inline int GetWeatherSkyDarkness(eWeather a_Weather)
{
	switch (a_Weather)
	{
		case wRain:  return SKY_DARKNESS_RAIN;
		case wStorm: return SKY_DARKNESS_STORM;
		default:     return 0;
	}
}





/** Returns the total sky darkness for the given time of day and weather.
Both parts are added together and capped at SKY_DARKNESS_MAX, so the result is always 0 .. 11 and the cast
to the unsigned NIBBLETYPE can never wrap around (cf. cChunk::GetTimeAlteredLight() clamping on the consumer
side). */
inline NIBBLETYPE CalcSkyDarkness(cTickTime a_TimeOfDay, eWeather a_Weather)
{
	const int TimePart = GetTimeSkyDarkness(a_TimeOfDay);
	const int WeatherPart = GetWeatherSkyDarkness(a_Weather);
	return static_cast<NIBBLETYPE>(std::min(TimePart + WeatherPart, SKY_DARKNESS_MAX));
}
