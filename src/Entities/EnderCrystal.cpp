
#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "EnderCrystal.h"
#include "../BoundingBox.h"
#include "../ClientHandle.h"
#include "../Chunk.h"
#include "../Mobs/EnderDragon.h"
#include "../World.h"





/** Distance within which an End crystal heals / hurts an ender dragon, in blocks. */
static constexpr double ENDER_CRYSTAL_HEAL_RANGE = 32.0;

/** Ticks between two healing ticks (vanilla: 1 HP every 10 game ticks). */
static constexpr int ENDER_CRYSTAL_HEAL_INTERVAL = 10;

/** Damage dealt to the dragon when a crystal that was healing it is destroyed. */
static constexpr int ENDER_CRYSTAL_HEAL_BREAK_DAMAGE = 10;





cEnderCrystal::cEnderCrystal(Vector3d a_Pos, bool a_ShowBottom) :
	cEnderCrystal(a_Pos, {}, false, a_ShowBottom)
{
}





cEnderCrystal::cEnderCrystal(Vector3d a_Pos, Vector3i a_BeamTarget, bool a_DisplayBeam, bool a_ShowBottom) :
	Super(etEnderCrystal, a_Pos, 2.0f, 2.0f),
	m_BeamTarget(a_BeamTarget),
	m_DisplayBeam(a_DisplayBeam),
	m_ShowBottom(a_ShowBottom),
	m_HealingTimer(0)
{
	SetMaxHealth(5);
}





void cEnderCrystal::SetShowBottom(bool a_ShowBottom)
{
	m_ShowBottom = a_ShowBottom;
	m_World->BroadcastEntityMetadata(*this);
}





void cEnderCrystal::SetBeamTarget(Vector3i a_BeamTarget)
{
	m_BeamTarget = a_BeamTarget;
	m_World->BroadcastEntityMetadata(*this);
}





void cEnderCrystal::SetDisplayBeam(bool a_DisplayBeam)
{
	m_DisplayBeam = a_DisplayBeam;
	m_World->BroadcastEntityMetadata(*this);
}





void cEnderCrystal::SpawnOn(cClientHandle & a_ClientHandle)
{
	a_ClientHandle.SendSpawnEntity(*this);
	a_ClientHandle.SendEntityMetadata(*this);
}





void cEnderCrystal::Tick(std::chrono::milliseconds a_Dt, cChunk & a_Chunk)
{
	UNUSED(a_Dt);
	if (m_World->GetDimension() != dimEnd)
	{
		return;
	}

	if (m_World->GetBlock(POS_TOINT) != E_BLOCK_FIRE)
	{
		m_World->SetBlock(POS_TOINT, E_BLOCK_FIRE, 0);
	}

	// Heal a nearby ender dragon and connect it with the white healing beam:
	UpdateHealing();
}





void cEnderCrystal::KilledBy(TakeDamageInfo & a_TDI)
{
	Super::KilledBy(a_TDI);

	// If this crystal was healing an ender dragon, the dragon takes 10 damage:
	cEnderDragon * Dragon = GetHealingDragon();
	if (Dragon != nullptr)
	{
		Dragon->TakeDamage(dtExplosion, cEntity::INVALID_ID, ENDER_CRYSTAL_HEAL_BREAK_DAMAGE, 0);
	}

	// Destroy first so the Explodinator doesn't find us (when iterating through entities):
	Destroy();

	m_World->DoExplosionAt(6.0, GetPosX(), GetPosY() + GetHeight() / 2, GetPosZ(), true, esEnderCrystal, this);

	const auto Position = GetPosition().Floor();
	if (cChunkDef::IsValidHeight(Position))
	{
		m_World->SetBlock(Position, E_BLOCK_FIRE, 0);
	}
}





cEnderDragon * cEnderCrystal::GetHealingDragon(void)
{
	cEnderDragon * NearestDragon = nullptr;
	double NearestSqr = ENDER_CRYSTAL_HEAL_RANGE * ENDER_CRYSTAL_HEAL_RANGE;
	const Vector3d Pos = GetPosition();
	const cBoundingBox SearchBox(
		Pos - Vector3d(ENDER_CRYSTAL_HEAL_RANGE, ENDER_CRYSTAL_HEAL_RANGE, ENDER_CRYSTAL_HEAL_RANGE),
		Pos + Vector3d(ENDER_CRYSTAL_HEAL_RANGE, ENDER_CRYSTAL_HEAL_RANGE, ENDER_CRYSTAL_HEAL_RANGE)
	);
	m_World->ForEachEntityInBox(SearchBox, [&](cEntity & a_Entity)
		{
			if (a_Entity.IsMob() && (static_cast<cMonster &>(a_Entity).GetMobType() == mtEnderDragon))
			{
				auto & Dragon = static_cast<cEnderDragon &>(a_Entity);

				// Never heal a dead or dying dragon, that would resurrect it or undo its death:
				if ((Dragon.GetHealth() <= 0) || (Dragon.GetDragonPhase() == cEnderDragon::eDragonPhase::Dying))
				{
					return false;
				}

				const double Sqr = (a_Entity.GetPosition() - Pos).SqrLength();
				if (Sqr < NearestSqr)
				{
					NearestSqr = Sqr;
					NearestDragon = &Dragon;
				}
			}

			// Keep searching:
			return false;
		}
	);
	return NearestDragon;
}





void cEnderCrystal::UpdateHealing(void)
{
	cEnderDragon * Dragon = GetHealingDragon();
	if (Dragon == nullptr)
	{
		if (DisplaysBeam())
		{
			SetDisplayBeam(false);
		}
		m_HealingTimer = 0;
		return;
	}

	// Point the white healing beam at the dragon:
	const Vector3i Target = Dragon->GetPosition().Floor();
	if (!DisplaysBeam())
	{
		SetBeamTarget(Target);
		SetDisplayBeam(true);
	}
	else if (m_BeamTarget != Target)
	{
		SetBeamTarget(Target);
	}

	// Heal 1 HP every 10 game ticks:
	if (++m_HealingTimer >= ENDER_CRYSTAL_HEAL_INTERVAL)
	{
		m_HealingTimer = 0;
		Dragon->Heal(1);
	}
}
