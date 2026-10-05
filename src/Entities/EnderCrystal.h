
#pragma once

#include "Entity.h"





// Forward declaration; the healing target is only needed as a pointer here.
class cEnderDragon;





// tolua_begin
class cEnderCrystal :
	public cEntity
{
	// tolua_end
	using Super = cEntity;

public:

	CLASS_PROTODEF(cEnderCrystal)

	cEnderCrystal(Vector3d a_Pos, bool a_ShowBottom);
	cEnderCrystal(Vector3d a_Pos, Vector3i a_BeamTarget, bool a_DisplayBeam, bool a_ShowBottom);

	// tolua_begin

	Vector3i GetBeamTarget() const { return m_BeamTarget; }
	void SetBeamTarget(Vector3i a_BeamTarget);

	/** If the EnderCrystal should send it's beam to the client and save it. */
	bool DisplaysBeam() const { return m_DisplayBeam; }
	void SetDisplayBeam(bool a_DisplayBeam);

	/** While set, the crystal is part of the dragon re-summon sequence and must not run its healing beam logic. */
	bool HasRespawnBeam() const { return m_RespawnBeam; }
	void SetRespawnBeam(bool a_RespawnBeam) { m_RespawnBeam = a_RespawnBeam; }

	bool ShowsBottom() const { return m_ShowBottom; }
	void SetShowBottom(bool a_ShowBottom);

	// tolua_end

private:

	Vector3i m_BeamTarget;
	bool m_DisplayBeam;

	/** True while this crystal takes part in the dragon re-summon sequence. */
	bool m_RespawnBeam;

	// If the bedrock base should be displayed.
	bool m_ShowBottom;

	// Ticks since this crystal last healed its dragon (vanilla: 1 HP every 10 ticks).
	int m_HealingTimer;

	/** Returns the nearest ender dragon within the healing range, or nullptr if there is none. */
	cEnderDragon * GetHealingDragon(void);

	/** Heals the nearby ender dragon and updates the healing beam. */
	void UpdateHealing(void);

	// cEntity overrides:
	virtual void SpawnOn(cClientHandle & a_ClientHandle) override;
	virtual void Tick(std::chrono::milliseconds a_Dt, cChunk & a_Chunk) override;
	virtual void KilledBy(TakeDamageInfo & a_TDI) override;

};  // tolua_export




