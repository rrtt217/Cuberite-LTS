// EndermanTeleportRules.h

// Declares the pure geometry and probabilities of the enderman's teleportation,
// so that they can be unit-tested without constructing a world.
// Behavior spec: "vanilla-1.12.2-enderman.md" in the specs folder, section 3.6.
// Primary sources: https://zh.minecraft.wiki/w/%E6%9C%AB%E5%BD%B1%E4%BA%BA (section "Teleportation")
// and https://minecraft.wiki/w/Enderman#Teleportation

#pragma once

#include "../Vector3.h"





/* NOTE on the chase blink's distance gate (spec 3.6.2): the English wiki restricts the closing teleport to
"the player at least 16 blocks away" (Needs-testing), while the Chinese wiki - our primary source - gives the
behaviour WITHOUT any distance gate: a player-provoked enderman repeatedly attempts to teleport to within some
distance of the provoking player ("behind" them; not while fighting other mobs).  We follow the Chinese wiki:
no gate.  At distance the snapped aim point closes in; up close the box lands around the player, which also
bridges the pathfinder standstills of the 3-block-clear enderman exactly like Vanilla does.  An earlier
implementation gated this at 16 blocks (en wiki reading) and the enderman visibly froze while chasing up close. */

/** The teleport attempt aims at a point this far horizontally / vertically towards the target;
a closer target snaps the aim point onto its own XZ / Y coordinates (spec 3.6.2). */
static constexpr double TELEPORT_CHASE_HORIZONTAL_RANGE = 16.0;
static constexpr double TELEPORT_CHASE_VERTICAL_RANGE = 17.0;

/** The random destination is picked within a 9x11x9 box around the aim point (spec 3.6.2, values marked Needs-testing on the wiki): */
static constexpr int TELEPORT_CHASE_BOX_HALF_XZ = 4;
static constexpr int TELEPORT_CHASE_BOX_HALF_Y = 5;

/** Time between teleport attempts towards the target: 1.5 - 2 seconds (spec 3.6.2): */
static constexpr int TELEPORT_CHASE_COOLDOWN_MIN_MS = 1500;
static constexpr int TELEPORT_CHASE_COOLDOWN_MAX_MS = 2000;

/** Being provoked BY A STARE teleports the enderman away immediately, once, at the moment of the provocation
(spec 3.2 - behavior until 1.13; afterwards it charges even while being looked at. The later 4-16 block
freezing rule is a 1.14 change and not implemented here). */

/** Damage, water and sunlight teleports pick a random destination in a 64x64x64 cube centered on the current position
(spec 3.6.1 / 3.6.3 / 3.6.4, wiki "Teleportation": "64x64x64 cube in Java Edition"). */
static constexpr int TELEPORT_RANDOM_HALF_CUBE = 32;

/** Sunlight teleports only happen at internal light level at or above this, measured at the enderman's eyes
(spec 3.6.3, Chinese wiki "Teleportation" formula condition "internal light level >= 13"): */
static constexpr int TELEPORT_SUNLIGHT_MIN_LIGHT = 13;

/** The sunlight teleport additionally requires that the enderman has been without a target for this long -
600 game ticks at 20 ticks per second (spec 3.6.3, Chinese wiki "Teleportation"): */
static constexpr int TELEPORT_SUNLIGHT_NO_TARGET_MS = 30000;

/** The eye block used for the sunlight light-level check is this many blocks above the feet block;
endermen are 2.9 blocks tall, no allowed source gives the exact eye offset - guess (spec 6, marked [guess]): */
static constexpr int TELEPORT_SUNLIGHT_EYE_BLOCK_OFFSET = 2;

/** Endermen are 2.9 blocks tall and need at least three non-solid blocks above the landing surface (spec 3.6): */
static constexpr int TELEPORT_LANDING_HEIGHT = 3;

/** Amount of portal particles shown at each end of a teleport, no allowed source gives the amount - guess (spec 3.6, marked [guess]): */
static constexpr int TELEPORT_PARTICLE_AMOUNT = 64;





/** Returns the per-tick probability of a sunlight teleport attempt for the given internal light level Li at the
enderman's eyes: P = (11 * Li - 120) / (225 * (20 - Li)), and 0 below TELEPORT_SUNLIGHT_MIN_LIGHT
(spec 3.6.3, Chinese wiki "Teleportation" formula; Li never exceeds 15, so the denominator cannot vanish). */
inline double EndermanSunlightTeleportChance(const double a_InternalLightLevel)
{
	if (a_InternalLightLevel < TELEPORT_SUNLIGHT_MIN_LIGHT)
	{
		return 0.0;
	}
	return (11.0 * a_InternalLightLevel - 120.0) / (225.0 * (20.0 - a_InternalLightLevel));
}





/** Returns the center of the random-destination box for a chasing enderman's teleport attempt:
the point 16 blocks horizontally and 17 blocks vertically towards the target,
snapped to the target's own XZ / Y coordinates when the target is closer than that range.
Horizontal and vertical clamping are independent (spec 3.6.2). */
inline Vector3d EndermanChaseTeleportCenter(const Vector3d a_From, const Vector3d a_Target)
{
	const Vector3d Difference = a_Target - a_From;
	Vector3d Center = a_From;

	// Horizontal: move at most TELEPORT_CHASE_HORIZONTAL_RANGE towards the target, on the XZ plane:
	const double HorizontalDistance = std::sqrt(Difference.x * Difference.x + Difference.z * Difference.z);
	if (HorizontalDistance > TELEPORT_CHASE_HORIZONTAL_RANGE)
	{
		const double Scale = TELEPORT_CHASE_HORIZONTAL_RANGE / HorizontalDistance;
		Center.x = a_From.x + Difference.x * Scale;
		Center.z = a_From.z + Difference.z * Scale;
	}
	else
	{
		Center.x = a_Target.x;
		Center.z = a_Target.z;
	}

	// Vertical: move at most TELEPORT_CHASE_VERTICAL_RANGE towards the target:
	if (Difference.y > TELEPORT_CHASE_VERTICAL_RANGE)
	{
		Center.y = a_From.y + TELEPORT_CHASE_VERTICAL_RANGE;
	}
	else if (Difference.y < -TELEPORT_CHASE_VERTICAL_RANGE)
	{
		Center.y = a_From.y - TELEPORT_CHASE_VERTICAL_RANGE;
	}
	else
	{
		Center.y = a_Target.y;
	}

	return Center;
}
