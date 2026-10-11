// HungerRules.h

// Declares the pure hunger / saturation arithmetic of the Saturation effect, so that it can be
// unit-tested without constructing a world, a player or a client connection.
// Behavior spec: "vanilla-1.12.2-status-effect-saturation.md" in the specs folder, section 2.
// Primary sources: https://minecraft.wiki/w/Potion (status effect table, Saturation: "Restores 1 hunger
// point x level and 2 x level points of saturation"), https://minecraft.wiki/w/Saturation_(effect)
// ("instantly replenishes 1 hunger x level and 2 x level saturation") and
// https://minecraft.wiki/w/Food#Saturation (saturation "can not exceed the food level").
// The amplifier -> level conversion is the generic one from EffectSpeedRules.h, reused here so that an
// amplifier of 0 is level I for the Saturation effect as well.

#pragma once

#include "../Globals.h"
#include "EffectSpeedRules.h"





/** The highest food level a player can have, cPlayer::MAX_FOOD_LEVEL.  ClampFoodState clamps to it so
that this rule stays a pure function that does not depend on cPlayer. */
static constexpr int MAX_HUNGER_LEVEL = 20;

/** The signed change one settlement of the Saturation effect applies to a player's food state (spec 2):
Hunger is added to the food level, Saturation to the food saturation level. */
struct EffectHungerDelta
{
	int Hunger;
	double Saturation;
};





/** Returns the food-state change a single settlement of the Saturation effect applies for the given
amplifier (spec 2): +1 hunger and +2 saturation per level, where level = amplifier + 1. */
inline EffectHungerDelta SaturationEffectDelta(int a_Amplifier)
{
	const int Level = EffectLevelFromAmplifier(a_Amplifier);
	return {Level, 2.0 * Level};
}





/** Clamps a food state to its physically possible range (spec 2): hunger into [0, MAX_HUNGER_LEVEL] and
saturation into [0, hunger] - saturation can never exceed the current food level. */
inline void ClampFoodState(int & a_Hunger, double & a_Saturation)
{
	a_Hunger = Clamp(a_Hunger, 0, MAX_HUNGER_LEVEL);
	a_Saturation = Clamp(a_Saturation, 0.0, static_cast<double>(a_Hunger));
}
