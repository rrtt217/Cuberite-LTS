// FireproofRules.h

// Declares the pure decision table behind "is this entity fireproof", so that it can be unit-tested
// without constructing a world or an entity.
// Behavior spec: "vanilla-1.12.2-status-effect-fire-resistance.md" in the specs folder, section 2.
// Primary sources: https://minecraft.wiki/w/Potion (status effect table, Fire Resistance: "Immunity to
// all heat-related damage") and https://minecraft.wiki/w/Enchanted_Golden_Apple (gives 5:00 of Fire
// Resistance).  That a creative or spectator player never burns is a game-mode rule, not an effect, see
// https://minecraft.wiki/w/Creative_mode and https://minecraft.wiki/w/Spectator_mode.

#pragma once





/** Returns whether an entity is fireproof, i.e. takes no damage from fire, lava or being on fire, and
burns out immediately when it was on fire (spec 2).
An entity is fireproof when any one of these holds:
- a_IsHardcodedFireproof: the monster configuration says the mob is fireproof (blazes, magma
  cubes, wither skeletons, ... - cMonsterConfig::m_IsFireproof, set through cEntity::SetIsFireproof);
- a_HasFireResistanceEffect: a Fire Resistance effect is active on the pawn, at any amplifier - the
  effect is a plain immunity, its level changes nothing (wiki "Effect", the potency table lists Fire
  Resistance as "scales with potency: No");
- a_IsCreativeOrSpectator: it is a player in a game mode in which nothing burns.
@param a_IsCreativeOrSpectator Pass false for anything that is not a player; only players have game modes.
@warning Every place answering this question has to pass all three inputs.  Querying only the hardcoded
flag is the bug this header exists to prevent: the entity burns although it carries the effect. */
inline bool IsFireproofFor(bool a_IsHardcodedFireproof, bool a_HasFireResistanceEffect, bool a_IsCreativeOrSpectator)
{
	return a_IsHardcodedFireproof || a_HasFireResistanceEffect || a_IsCreativeOrSpectator;
}
