// EffectNumbers.h

// Declares the pure numeric rules of the two status effects whose 1.12.2 values were wrong: the amounts of
// the instant health / instant damage potions and the food exhaustion rate of the Hunger effect, so that
// they can be unit-tested without constructing a world or an entity.
// Behavior spec: "vanilla-1.12.2-status-effect-numbers.md" in the specs folder, sections 1 and 2.
// Primary sources: https://minecraft.wiki/w/Potion (status effect table: Potion of Healing "Restores
// 4 health", Healing II "Restores 8 health (doubles with every additional level)", Potion of Harming
// "Deals 6 health"), https://minecraft.wiki/w/Potion_of_Healing (its History records the 13w23a nerf of
// 33.3% that took Healing from 6 to 4 and lists no later numeric change, so 4 is the 1.12.2 value) and
// https://minecraft.wiki/w/Hunger_(effect) ("Hunger increases food exhaustion by 0.005 x level per game
// tick", cross-checked by the same page's figure of 3.0 exhaustion over the 30 seconds of a Hunger I
// effect).  The stored-amplifier range is the generic one from EffectSpeedRules.h.

#pragma once

#include "../Globals.h"
#include "EffectSpeedRules.h"





/** How much health one level-I Potion of Healing restores (spec 1, wiki "Potion": "Restores 4 health");
every further level doubles it (Healing II: 8, Healing III: 16, Healing IV: 32). */
static constexpr int INSTANT_HEALTH_BASE_RECOVERY = 4;

/** How much damage one level-I Potion of Harming deals (spec 1, wiki "Potion": "Deals 6 health"); every
further level doubles it (Harming II: 12).  The two instant effects do not share a base value: healing used
to reuse this 6 and was therefore always 1.5x too strong. */
static constexpr int INSTANT_DAMAGE_BASE_DAMAGE = 6;

/** How much food exhaustion one level of the Hunger effect adds per game tick (spec 2, wiki "Hunger
(effect)": "0.005 x level per game tick"; a Hunger I effect over the 600 ticks of a rotten flesh / raw
chicken effect therefore totals 0.005 x 600 = 3.0, the figure that page quotes). */
static constexpr double HUNGER_EXHAUSTION_PER_TICK_PER_LEVEL = 0.005;

/** The highest shift the doubling below performs.  A shift of 32 or more is undefined behaviour for a
32-bit int, and any amount past this point is already beyond any entity's health, so it saturates instead. */
static constexpr int MAX_DOUBLING_SHIFT = 31;

/** The value the doubling below saturates at - the largest amount of health / damage an int can carry. */
static constexpr int MAX_INSTANT_EFFECT_AMOUNT = std::numeric_limits<int>::max();





/** Returns a_Base doubled a_Amplifier times, saturating at MAX_INSTANT_EFFECT_AMOUNT.
The amplifier reaches this code as a protocol Byte (0..255, EffectSpeedRules.h MAX_EFFECT_AMPLIFIER) or, via
the Lua API, as an arbitrary short, so it is clamped into the stored range first: a negative amplifier would
ask for a fractional amount, and a shift of 32 or more is undefined behaviour.  Spec 4 left this
out-of-range policy to spec review ("std::clamp or a saturating value"); this implementation saturates,
because any amount past an int is far beyond any entity's health anyway, and because saturating cannot
abort a Debug server the way an ASSERT on a plugin-supplied amplifier would. */
inline int DoublingAmount(int a_Base, int a_Amplifier)
{
	const int Amplifier = Clamp(a_Amplifier, 0, MAX_EFFECT_AMPLIFIER);
	const long long Amount = static_cast<long long>(a_Base) << std::min(Amplifier, MAX_DOUBLING_SHIFT);
	return (Amount > MAX_INSTANT_EFFECT_AMOUNT) ? MAX_INSTANT_EFFECT_AMOUNT : static_cast<int>(Amount);
}





/** Returns the health restored by a Potion of Healing applied with the given amplifier (spec 1):
4 * 2^amplifier, that is 4 / 8 / 16 / 32 for levels I..IV.  The final clamp to the target's maximum health
is cEntity::Heal's job. */
inline int InstantHealthAmount(int a_Amplifier)
{
	return DoublingAmount(INSTANT_HEALTH_BASE_RECOVERY, a_Amplifier);
}





/** Returns the damage dealt by a Potion of Harming applied with the given amplifier (spec 1):
6 * 2^amplifier, that is 6 / 12 / 24 / 48 for levels I..IV. */
inline int InstantDamageAmount(int a_Amplifier)
{
	return DoublingAmount(INSTANT_DAMAGE_BASE_DAMAGE, a_Amplifier);
}
