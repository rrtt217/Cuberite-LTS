// ThrownEnderEyeEntity.h

// Declares the cThrownEnderEyeEntity class representing an eye of ender thrown towards a structure

#pragma once

#include "ProjectileEntity.h"

// tolua_begin

class cThrownEnderEyeEntity :
	public cProjectileEntity
{
	// tolua_end

	using Super = cProjectileEntity;

public:  // tolua_export

	CLASS_PROTODEF(cThrownEnderEyeEntity)

	/** Creates an eye of ender flying towards a_Target.
	The eye is not affected by gravity nor by block collisions - it computes its own flight and ends it
	after a fixed lifetime, by either dropping as an item or shattering (see the eye-of-ender spec in the
	specs directory, vanilla-1.12.2-eye-of-ender.md). */
	cThrownEnderEyeEntity(cEntity * a_Creator, Vector3d a_Pos, Vector3d a_Target);

private:

	/** How many ticks the eye flies towards the target before it starts hovering.
	Vanilla: two to three seconds of travel (Minecraft Wiki: Eye of Ender). */
	static constexpr int TRAVEL_TICKS = 48;

	/** How many ticks after being thrown the eye drops or shatters, i.e. the travel plus the short
	hover at the end of the flight. */
	static constexpr int LIFETIME_TICKS = 80;

	/** Chance, in percent, that the eye shatters instead of dropping as an item (vanilla: 20%). */
	static constexpr int SHATTER_CHANCE = 20;

	/** Horizontal speed towards the target, in blocks per second (the same unit that cEntity::SetSpeed uses).
	48 ticks at 5 blocks per second cover the ~12 blocks an eye of ender travels in vanilla. */
	static constexpr double HORIZONTAL_SPEED = 5.0;

	/** How fast the eye climbs while far from the target, in blocks per second. */
	static constexpr double CLIMB_SPEED = 4.0;

	/** How fast the eye dives once close to the target, in blocks per second. */
	static constexpr double DIVE_SPEED = 4.0;

	/** While further than this many blocks from the target (horizontally), the eye climbs; once closer, it dives. */
	static constexpr double HIKE_DISTANCE = 12.0;

	/** The position of the structure the eye flies towards. */
	Vector3d m_Target;

	// cProjectileEntity overrides:
	virtual void OnHitEntity(cEntity & a_EntityHit, Vector3d a_HitPos) override;
	virtual void OnHitSolidBlock(Vector3d a_HitPos, eBlockFace a_HitFace) override;

	// cEntity overrides:
	virtual void Tick(std::chrono::milliseconds a_Dt, cChunk & a_Chunk) override;
	virtual void HandlePhysics(std::chrono::milliseconds a_Dt, cChunk & a_Chunk) override;

	/** Ends the flight: either drops the eye as a pickup or shatters it. */
	void EndOfLife(void);
} ;  // tolua_export
