#pragma once

#include "ProjectileEntity.h"





/** The fireball fired by the ender dragon. It deals no damage on impact with entities and, after a
short fuse, bursts into a purple area effect cloud of Instant Damage II.
Ref: https://minecraft.wiki/w/Ender_Dragon#Dragon_Fireball */
class cDragonFireballEntity :
	public cProjectileEntity
{
	using Super = cProjectileEntity;

public:  // tolua_export

	CLASS_PROTODEF(cDragonFireballEntity)

	cDragonFireballEntity(cEntity * a_Creator, Vector3d a_Pos, Vector3d a_Speed);

	// cEntity overrides:
	virtual void Tick(std::chrono::milliseconds a_Dt, cChunk & a_Chunk) override;

protected:

	/** Ticks left before the fireball bursts; -1 while it is still flying. */
	int m_BurstTicksLeft;

	/** Creates the area effect cloud and removes the fireball. */
	void Burst(void);

	// cProjectileEntity overrides:
	virtual void OnHitSolidBlock(Vector3d a_HitPos, eBlockFace a_HitFace) override;
	virtual void OnHitEntity(cEntity & a_EntityHit, Vector3d a_HitPos) override;
};
