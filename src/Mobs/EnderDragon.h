
#pragma once

#include "AggressiveMonster.h"





class cEnderDragon:
	public cAggressiveMonster
{
	using Super = cAggressiveMonster;

public:

	/** Number of separate parts the vanilla client creates for an ender dragon.
	The client derives their entity IDs from the dragon's own ID, so this number and their
	placement in the ID space must match the client's expectation. */
	static constexpr UInt32 PART_COUNT = 8;

	cEnderDragon();

	CLASS_PROTODEF(cEnderDragon)

	/** Returns whether a_ID is one of the entity IDs that the vanilla client uses for this dragon's parts.
	The parts are not represented by server-side entities; the client derives their IDs as
	dragonID + 1 .. dragonID + PART_COUNT and sends them back when a player attacks a part. */
	bool IsPartID(UInt32 a_ID) const;

	/** Applies damage dealt by a player hitting one of the dragon's parts.
	Vanilla reduces all damage taken to a quarter (plus a small constant) unless the hit lands on
	the head; the head exception is not implemented yet, see the feature spec document for this branch. */
	void TakeDamageFromPart(cEntity & a_Attacker);

	virtual bool DoTakeDamage(TakeDamageInfo & a_TDI) override;
	virtual void GetDrops(cItems & a_Drops, cEntity * a_Killer = nullptr) override;
	virtual void SpawnOn(cClientHandle & a_Client) override;
} ;
