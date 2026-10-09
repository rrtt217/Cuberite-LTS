
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

	/** Sets the carried block without broadcasting the change; used when loading from NBT.
	Runtime changes go through the pickup / placement code, which broadcasts the entity metadata. */
	void SetCarriedBlock(BLOCKTYPE a_BlockType, NIBBLETYPE a_BlockMeta);

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

	/** Runs the block-carrying behaviour for this tick: an enderman without a carried block may pick one
	up, one that carries a block may place it (spec 3.7). */
	void TickBlockCarrying(void);

	/** Makes a pickup attempt in the 4x3x4 region around the enderman, if the random chance passes (spec 3.7). */
	void TryPickUpBlock(void);

	/** Makes a placement attempt in the 2x2x2 region around the enderman, if the random chance passes (spec 3.7). */
	void TryPlaceCarriedBlock(void);

	/** Returns whether the line of sight from the enderman's head to the center of the block is not
	obstructed by a solid block (spec 3.7: the enderman must be able to directly see the block). */
	bool CanSeeBlock(Vector3i a_BlockPos) const;

	bool m_bIsScreaming;
	BLOCKTYPE m_CarriedBlock;
	NIBBLETYPE m_CarriedMeta;

	/** Time until the next teleport attempt towards the chase target (spec 3.6.2). */
	std::chrono::milliseconds m_ChaseTeleportCooldown;

	/** Time since the enderman lost its target, the sunlight teleport requires it to exceed 600 game ticks (spec 3.6.3). */
	std::chrono::milliseconds m_TimeWithoutTarget;

} ;
