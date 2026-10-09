// EffectSpeedRules.h

// Declares the pure level arithmetic of the Speed and Slowness movement-speed effects, so that it can be
// unit-tested without constructing a world or an entity.
// Behavior spec: "vanilla-1.12.2-status-effect-amplifier.md" in the specs folder, section 2.
// Primary sources: https://minecraft.wiki/w/Speed (section "Effects", "increases an entity's speed while
// walking or sprinting by +20% multiplied by the effect level"), https://minecraft.wiki/w/Slowness
// (section "Effect", "decreases movement speed by 15% x level") and https://minecraft.wiki/w/Effect
// (section "Effect potency", "a minimum potency of 1 ... represented as an amplifier of 0 up to 255").

#pragma once

#include "../Globals.h"





/** How much one level of Speed adds to the relative walk speed (spec 2, wiki "Speed": "+20% multiplied by
the effect level"; the potion tooltip of a Speed I potion reads "+20% Speed"). */
static constexpr double SPEED_SPEEDUP_PER_LEVEL = 0.2;

/** How much one level of Slowness takes off the relative walk speed (spec 2, wiki "Slowness": "decreases
movement speed by 15% x level"; a Slowness IV potion reads "-60% Speed"). */
static constexpr double SLOWNESS_SLOWDOWN_PER_LEVEL = 0.15;

/** Sprinting speed is this much of walking speed, so the speed delta has to be scaled by the same factor
to keep sprint as a fixed multiple of walk.  Cuberite's player sprint baseline is 1.3 against a normal
baseline of 1.0 (cPlayer::cPlayer), and the pre-existing effect code used 0.26 = 0.2 * 1.3 for Speed. */
static constexpr double SPRINT_MULTIPLIER = 1.3;

/** The highest amplifier 1.12.2 stores for an effect (wiki "Effect", "Effect potency": "a maximum potency
of 256, which is also represented as an amplifier of 0 up to 255"). */
static constexpr int MAX_EFFECT_AMPLIFIER = 255;

/** The amplifier of the Slowness that the Bane of Arthropods enchantment inflicts: the enchantment gives
Slowness IV (https://minecraft.wiki/w/Bane_of_Arthropods and https://minecraft.wiki/w/Slowness#Notes), and
potency IV is amplifier III. */
static constexpr int BANE_OF_ARTHROPODS_SLOWNESS_AMPLIFIER = 3;





/** Converts the stored amplifier (what the protocol, the entity effects' intensity and /effect carry) into
the effect level / potency the behavior spec is written in (spec 2): level = amplifier + 1.
An amplifier of 0 is level I, which is the whole point of this header - the effects that scale per level
must not treat level I as "no effect at all". */
inline int EffectLevelFromAmplifier(int a_Amplifier)
{
	ASSERT((a_Amplifier >= 0) && (a_Amplifier <= MAX_EFFECT_AMPLIFIER));

	// ASSERT is compiled out in release builds, and a plugin or a hand-crafted packet may push an
	// out-of-range intensity through cPawn::AddEntityEffect; clamp so that the +1 below cannot overflow:
	if (a_Amplifier < 0)
	{
		return 1;
	}
	if (a_Amplifier > MAX_EFFECT_AMPLIFIER)
	{
		return MAX_EFFECT_AMPLIFIER + 1;
	}
	return a_Amplifier + 1;
}





/** Returns the delta to add to a walking / normal relative speed for the given Speed amplifier (spec 2):
+20% of the baseline per level, so Speed I adds +0.2 and Speed V adds +1.0. */
inline double SpeedMaxSpeedDelta(int a_Amplifier)
{
	return SPEED_SPEEDUP_PER_LEVEL * EffectLevelFromAmplifier(a_Amplifier);
}





/** Returns the delta to add to a walking / normal relative speed for the given Slowness amplifier
(spec 2): -15% of the baseline per level, so Slowness I takes -0.15 and Slowness IV -0.6. */
inline double SlownessMaxSpeedDelta(int a_Amplifier)
{
	return -SLOWNESS_SLOWDOWN_PER_LEVEL * EffectLevelFromAmplifier(a_Amplifier);
}





/** Returns the delta to add to a sprinting relative speed for the given Speed amplifier (spec 2): sprint is
a fixed multiple of walking, so the walking delta is scaled by the same factor (0.26 for Speed I). */
inline double SpeedSprintingMaxSpeedDelta(int a_Amplifier)
{
	return SpeedMaxSpeedDelta(a_Amplifier) * SPRINT_MULTIPLIER;
}





/** Returns the delta to add to a sprinting relative speed for the given Slowness amplifier (spec 2), see
SpeedSprintingMaxSpeedDelta (-0.195 for Slowness I). */
inline double SlownessSprintingMaxSpeedDelta(int a_Amplifier)
{
	return SlownessMaxSpeedDelta(a_Amplifier) * SPRINT_MULTIPLIER;
}





/** Returns the delta to add to a flying relative speed for the given Speed amplifier.
[needs-check] (spec 6): the allowed sources give one movement-speed figure and do not split flying out of
walking, so flying keeps using the plain walking delta, as the pre-existing code did. */
inline double SpeedFlyingMaxSpeedDelta(int a_Amplifier)
{
	return SpeedMaxSpeedDelta(a_Amplifier);
}





/** Returns the delta to add to a flying relative speed for the given Slowness amplifier, see
SpeedFlyingMaxSpeedDelta. */
inline double SlownessFlyingMaxSpeedDelta(int a_Amplifier)
{
	return SlownessMaxSpeedDelta(a_Amplifier);
}
