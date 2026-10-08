
#pragma once

#include "PassiveAggressiveMonster.h"





class cEnderman:
	public cPassiveAggressiveMonster
{
	using Super = cPassiveAggressiveMonster;

public:

	cEnderman();

	CLASS_PROTODEF(cEnderman)

	virtual void GetDrops(cItems & a_Drops, cEntity * a_Killer = nullptr) override;
	virtual void CheckEventSeePlayer(cChunk & a_Chunk) override;
	virtual void EventLosePlayer(void) override;
	virtual void Tick(std::chrono::milliseconds a_Dt, cChunk & a_Chunk) override;
	virtual void InStateChasing(std::chrono::milliseconds a_Dt, cChunk & a_Chunk) override;
	virtual bool DoTakeDamage(TakeDamageInfo & a_TDI) override;

	bool IsScreaming(void) const {return m_bIsScreaming; }
	BLOCKTYPE GetCarriedBlock(void) const {return m_CarriedBlock; }
	NIBBLETYPE GetCarriedMeta(void) const {return m_CarriedMeta; }

private:

	/** Makes a teleport attempt to a random destination within the 64x64x64 cube around the current position.
	Returns true on success. a_NumTries is the number of destination candidates to try. */
	bool TeleportRandomly(unsigned int a_NumTries);

	/** Makes a teleport attempt towards the current target, within the box around the chase center.
	Returns true on success. */
	bool TeleportTowardsTarget(void);

	/** Moves the enderman to the destination and shows the teleport effects
	(sound at the destination, portal particles at both ends). */
	void DoTeleport(Vector3d a_Destination);

	bool m_bIsScreaming;
	BLOCKTYPE m_CarriedBlock;
	NIBBLETYPE m_CarriedMeta;

	/** Time until the next teleport attempt towards the chase target (spec 3.6.2). */
	std::chrono::milliseconds m_ChaseTeleportCooldown;

	/** Time since the enderman lost its target, the sunlight teleport requires it to exceed 600 game ticks (spec 3.6.3). */
	std::chrono::milliseconds m_TimeWithoutTarget;

} ;
