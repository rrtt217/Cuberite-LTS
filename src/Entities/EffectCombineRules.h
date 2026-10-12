// EffectCombineRules.h

// Declares the pure rules that decide how a newly applied status effect combines with an effect of the
// same type that is already active, which effects are instantaneous rather than lasting, and the duration
// scaling a source such as a splash potion has to apply before handing an effect to
// cPawn::AddEntityEffect, so that all of it can be unit-tested without constructing a world or an entity.
// Behavior spec: "vanilla-1.12.2-status-effect-application.md" in the specs folder, sections 2 and 3.
// Primary source for the combination table: https://minecraft.wiki/w/Commands/effect ("if a target already
// has a status effect with the same id, a new effect only with a longer duration or a higher amplifier can
// be added ... Otherwise, the original active effect is replaced by the new effect").  1.12.2 has no hidden
// effects yet, so a weaker effect is simply ignored instead of hiding the active one.
// Primary source for the distance decay: https://minecraft.wiki/w/Potion (splash / lingering potions lose
// duration and potency with the distance from the splash point; the decay factor stays within [0, 1]).

#pragma once

#include "../Globals.h"
#include "EntityEffect.h"






/** Returns whether an effect is applied instantaneously instead of being carried for a duration: the
potions of healing, harming and saturation grant their effect the moment they are applied.
An instantaneous effect is never a lasting active effect that could be combined with a new one - every
application has to run its OnActivate right away - so cPawn::AddEntityEffect bypasses DecideCombine for
these types and lets the newest application replace the stored one. */
inline bool IsInstantEffect(cEntityEffect::eType a_Type)
{
	return (
		(a_Type == cEntityEffect::effInstantHealth) ||
		(a_Type == cEntityEffect::effInstantDamage) ||
		(a_Type == cEntityEffect::effSaturation)
	);
}






/** How a newly applied effect combines with the effect of the same type that is already active. */
enum class EffectCombine
{
	/** The new effect replaces the active one: both its amplifier and its remaining time win. */
	Replace,

	/** Nothing changes, the active effect keeps its amplifier and its remaining time. */
	Ignore,

	/** Only the active effect's remaining time is raised to the new duration; its amplifier and the ticks
	it has already run for stay as they are - the effect is extended, not restarted. */
	ExtendDuration,
};






/** Decides how a newly applied effect combines with an effect of the same type (spec 2):
a stronger effect always replaces the active one, a weaker one is ignored, and an equally strong one only
extends the active effect when it lasts strictly longer.
@param a_OldAmplifier      The stored amplifier of the active effect.
@param a_OldRemainingTicks For how many more ticks the active effect runs, i.e. its duration minus the
                           ticks it has already run for.
@param a_OldTicksElapsed   For how many ticks the active effect has already run.  It does not enter the
                           decision, but it belongs to the old effect's state and the caller needs it to
                           rebuild the total duration when this returns ExtendDuration.
@param a_NewAmplifier      The amplifier of the effect being applied.
@param a_NewDurationTicks  How long the effect being applied would last on its own.
@returns The combine decision; a non-positive a_NewDurationTicks is never treated as an extension. */
inline EffectCombine DecideCombine(int a_OldAmplifier, int a_OldRemainingTicks, int a_OldTicksElapsed,
	int a_NewAmplifier, int a_NewDurationTicks)
{
	ASSERT(a_OldTicksElapsed >= 0);

	// A stronger effect always wins, even when it would run for a shorter time than the active one:
	if (a_NewAmplifier > a_OldAmplifier)
	{
		return EffectCombine::Replace;
	}

	// A weaker effect can never push aside a stronger one:
	if (a_NewAmplifier < a_OldAmplifier)
	{
		return EffectCombine::Ignore;
	}

	// Same amplifier: only a strictly longer duration is accepted, and it extends the active effect instead
	// of restarting it.  A non-positive duration is not an extension - it would be an effect that is already
	// over, and extending to it would leave the effect with a negative remaining time (spec 4 invariant):
	if ((a_NewDurationTicks > a_OldRemainingTicks) && (a_NewDurationTicks > 0))
	{
		return EffectCombine::ExtendDuration;
	}

	return EffectCombine::Ignore;
}






/** Returns the total duration the active effect must be given so that it keeps the ticks it has already run
for and has exactly a_RemainingTicks left (spec 3, "只把时长延长到新值，不重置已进行 tick").
This is the arithmetic of an extension: the elapsed ticks are preserved, only the remaining time changes. */
inline int ExtendedTotalDuration(int a_ElapsedTicks, int a_RemainingTicks)
{
	return a_ElapsedTicks + a_RemainingTicks;
}






/** Applies the splash / lingering-potion distance decay to an effect's duration (spec 2 and 3).
The caller computes the decay factor from the distance - for a splashing potion entity that is the existing
1 - 0.25 * distance, clamped to at least 0 - and passes the already scaled duration to
cPawn::AddEntityEffect, which deliberately no longer scales the duration itself.  A factor of 0 therefore
legitimately means "no duration at all" (a target far enough from the splash point) and a factor of 1 leaves
the duration untouched; factors outside [0, 1] are caller errors and are rejected in debug builds. */
inline int ApplyDurationMultiplier(int a_Duration, double a_Multiplier)
{
	ASSERT(a_Duration >= 0);
	ASSERT(a_Multiplier >= 0.0);
	ASSERT(a_Multiplier <= 1.0);

	return static_cast<int>(a_Duration * a_Multiplier);
}
