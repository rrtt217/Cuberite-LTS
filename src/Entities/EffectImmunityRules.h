// EffectImmunityRules.h

// Declares the pure decision table behind "may this effect be applied to this entity", so that the
// immunity rules can be unit-tested without constructing an entity.
// Behavior spec: "vanilla-1.12.2-status-effect-immunity.md" in the specs folder, section 2.
// Primary source: https://minecraft.wiki/w/Status_effect (section "Immunity"):
// - the ender dragon and the wither are immune to all effects;
// - undead mobs are immune to Regeneration and Poison;
// - wither skeletons are immune to Wither in addition to Regeneration and Poison;
// - spiders and cave spiders are immune to Poison.
// The wiki section also lists immunities that were added after 1.12.2 (trial chambers, illagers, slimes,
// silverfish, breezes) and effects that do not exist in 1.12.2 (Trial Omen, Raid Omen, Oozing, Infested,
// Wind Charged); those rows are deliberately not part of this table.

#pragma once

#include "EntityEffect.h"
#include "../Mobs/MonsterTypes.h"





/** Returns whether a mob of this type is one of the two 1.12.2 bosses that are immune to every effect
(spec 2, wiki "Immunity": "The ender dragon, the wither ... are immune to all effects").
The check is a plain mob-type lookup rather than a virtual on cMonster: the two bosses are the only mobs
with this property in 1.12.2, and adding a virtual for them would touch the whole class hierarchy.
@param a_MobType The mob type; every non-boss type, including mtInvalidType for a non-mob, returns false. */
inline bool IsBossImmuneToAllEffects(eMonsterType a_MobType)
{
	return (a_MobType == mtWither) || (a_MobType == mtEnderDragon);
}





/** Returns whether an effect may be applied to the entity described by the arguments (spec 2).  Returning
false means the entity is immune: the effect must be rejected as a whole, not merely left without an
effect while its icon is broadcast.
@param a_Type     The effect type to apply.
@param a_MobType  The mob type of the entity, or mtInvalidType for anything that is not a mob (players).
@param a_IsUndead Whether the entity is undead (cMonster::IsUndead); false for players.
@param a_IsBoss   Whether the entity is a boss immune to every effect (IsBossImmuneToAllEffects above).
@warning This is the single source of truth for effect immunity.  The per-effect OnTick code no longer
decides on its own who is immune, it only asserts that the rejection here really happened. */
inline bool IsEffectApplicable(cEntityEffect::eType a_Type, eMonsterType a_MobType, bool a_IsUndead, bool a_IsBoss)
{
	// The wither and the ender dragon are immune to every effect:
	if (a_IsBoss)
	{
		return false;
	}

	switch (a_Type)
	{
		case cEntityEffect::effRegeneration:
		{
			// Undead mobs are immune to Regeneration:
			return !a_IsUndead;
		}
		case cEntityEffect::effPoison:
		{
			// Undead mobs, spiders and cave spiders are immune to Poison:
			return !a_IsUndead && (a_MobType != mtSpider) && (a_MobType != mtCaveSpider);
		}
		case cEntityEffect::effWither:
		{
			// Wither skeletons are immune to Wither:
			return (a_MobType != mtWitherSkeleton);
		}
		default:
		{
			return true;
		}
	}
}
