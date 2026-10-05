
#pragma once

#include "AggressiveMonster.h"





class cEnderDragon:
	public cAggressiveMonster
{
	using Super = cAggressiveMonster;

public:

	/** The dragon's behaviour states, matching the vanilla DragonPhase values (0-10). */
	enum class eDragonPhase : UInt8
	{
		Circling = 0,
		Strafing = 1,
		FlyingToPortal = 2,
		Landing = 3,
		TakingOff = 4,
		LandedBreath = 5,
		LandedSearching = 6,
		LandedRoar = 7,
		Charging = 8,
		Dying = 9,
		Hovering = 10,
	};

	/** Number of separate parts the vanilla client creates for an ender dragon.
	The client derives their entity IDs from the dragon's own ID, so this number and their
	placement in the ID space must match the client's expectation. */
	static constexpr UInt32 PART_COUNT = 8;

	/** Zero-based index of the head within the dragon's parts, i.e. the only part that takes full
	damage in vanilla. Measured in-game with a 1.12.2 client: hitting the head sent partID = dragonID + 1. */
	static constexpr UInt32 HEAD_PART_INDEX = 0;

	cEnderDragon();

	CLASS_PROTODEF(cEnderDragon)

	/** Returns the dragon's current behaviour phase. */
	eDragonPhase GetDragonPhase(void) const { return m_DragonPhase; }

	/** Sets the dragon's behaviour phase and syncs it to the clients if it changed. */
	void SetDragonPhase(eDragonPhase a_Phase);

	/** Returns whether a_ID is one of the entity IDs that the vanilla client uses for this dragon's parts.
	The parts are not represented by server-side entities; the client derives their IDs as
	dragonID + 1 .. dragonID + PART_COUNT and sends them back when a player attacks a part. */
	bool IsPartID(UInt32 a_ID) const;

	/** Applies damage dealt by a player hitting one of the dragon's parts.
	Vanilla reduces all damage taken to a quarter (plus a small constant) unless the hit lands on
	the head, which takes full damage. a_IsHead tells whether the hit part is the head. */
	void TakeDamageFromPart(cEntity & a_Attacker, bool a_IsHead);

	virtual bool Attack(std::chrono::milliseconds a_Dt) override;
	virtual bool DoTakeDamage(TakeDamageInfo & a_TDI) override;
	virtual void GetDrops(cItems & a_Drops, cEntity * a_Killer = nullptr) override;
	virtual void KilledBy(TakeDamageInfo & a_TDI) override;
	virtual void HandlePhysics(std::chrono::milliseconds a_Dt, cChunk & a_Chunk) override;
	virtual void SpawnOn(cClientHandle & a_Client) override;
	virtual void Tick(std::chrono::milliseconds a_Dt, cChunk & a_Chunk) override;

protected:

	/** The dragon's current behaviour phase. */
	eDragonPhase m_DragonPhase;

	/** Number of End crystals around the arena; recounted periodically. */
	UInt32 m_CrystalCount;

	/** Ticks until the End crystal count is refreshed again. */
	int m_CrystalCountCooldown;

	/** Ticks left in the current strafing run. */
	int m_StrafingTicksLeft;

	/** Ticks left until the dragon takes off from its perch. */
	int m_LandedTicksLeft;

	/** Ticks left in the current take-off. */
	int m_TakeoffTicksLeft;

	/** Angle at the previous circling tick, used to detect a completed orbit. */
	double m_LastOrbitAngle;

	/** Damage type of the blow that started the Dying phase, replayed when the dragon actually dies. */
	eDamageType m_DyingDamageType;

	/** Entity ID of the attacker that started the Dying phase, or cEntity::INVALID_ID. */
	UInt32 m_DyingAttackerID;

	/** Moves the dragon along its circling path around the world centre. */
	void Circling(double a_Dt);

	/** Flies the dragon towards its target while strafing. */
	void Strafe(double a_Dt);

	/** Enters the strafing phase (e.g. after an End crystal was destroyed), if there is a target. */
	void StartStrafing(void);

	/** Decides to fly to the exit portal and land there. */
	void StartPerching(void);

	/** Flies the dragon to the perch above the exit portal while in the FlyingToPortal phase. */
	void FlyToPortal(double a_Dt);

	/** Flies the dragon to the exit portal after a fatal blow, before it really dies. */
	void Dying(double a_Dt);

	/** Actually kills the dragon once it has reached the exit portal. */
	void FinishDying(void);

	/** Damages the living entities the dragon is currently touching (wings / head contact). */
	void AttackEntities(void);

	/** Recounts the End crystals around the arena (throttled). */
	void UpdateCrystalCount(void);
} ;
