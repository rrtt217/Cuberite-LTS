
#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "Chunk.h"
#include "Enderman.h"
#include "EndermanTeleportRules.h"
#include "../Entities/Player.h"
#include "../FastRandom.h"
#include "../LineBlockTracer.h"




////////////////////////////////////////////////////////////////////////////////
// cPlayerLookCheck
class cPlayerLookCheck
{
public:

	cPlayerLookCheck(Vector3d a_EndermanHeadPosition, int a_SightDistance) :
		m_Player(nullptr),
		m_EndermanHeadPosition(a_EndermanHeadPosition),
		m_SightDistance(a_SightDistance)
	{
	}

	bool operator () (cPlayer & a_Player)
	{
		// Don't check players who cannot be targeted
		if (!a_Player.CanMobsTarget())
		{
			return false;
		}

		const auto PlayerHeadPosition = a_Player.GetPosition().addedY(a_Player.GetHeight());
		const auto Direction = m_EndermanHeadPosition - PlayerHeadPosition;

		// Don't check players who are more than SightDistance (64) blocks away:
		if (Direction.Length() > m_SightDistance)
		{
			return false;
		}

		// Don't check if the player has a pumpkin on his head:
		if (a_Player.GetEquippedHelmet().m_ItemType == E_BLOCK_PUMPKIN)
		{
			return false;
		}

		const auto LookVector = a_Player.GetLookVector();  // Note: ||LookVector|| is always 1.
		const auto Cosine = Direction.Dot(LookVector) / Direction.Length();  // a.b / (||a|| * ||b||)

		// If the player's crosshair is within 5 degrees of the enderman, it counts as looking:
		if ((Cosine < std::cos(0.09)) || (Cosine > std::cos(0)))  // 0.09 rad ~ 5 degrees
		{
			return false;
		}

		// TODO: Check if endermen are angered through water in Vanilla
		if (!cLineBlockTracer::LineOfSightTrace(*a_Player.GetWorld(), m_EndermanHeadPosition, PlayerHeadPosition, cLineBlockTracer::losAirWater))
		{
			// No direct line of sight
			return false;
		}

		m_Player = &a_Player;
		return true;
	}

	cPlayer * GetPlayer(void) const { return m_Player; }

protected:

	cPlayer * m_Player;
	Vector3d m_EndermanHeadPosition;
	int m_SightDistance;
} ;





cEnderman::cEnderman(void) :
	Super("Enderman", mtEnderman, "entity.endermen.hurt", "entity.endermen.death", "entity.endermen.ambient", 0.6f, 2.9f),
	m_bIsScreaming(false),
	m_CarriedBlock(E_BLOCK_AIR),
	m_CarriedMeta(0),
	m_ChaseTeleportCooldown(0),
	m_TimeWithoutTarget(0)
{
}





void cEnderman::GetDrops(cItems & a_Drops, cEntity * a_Killer)
{
	unsigned int LootingLevel = 0;
	if (a_Killer != nullptr)
	{
		LootingLevel = a_Killer->GetEquippedWeapon().m_Enchantments.GetLevel(cEnchantments::enchLooting);
	}
	AddRandomDropItem(a_Drops, 0, 1 + LootingLevel, E_ITEM_ENDER_PEARL);
}





void cEnderman::CheckEventSeePlayer(cChunk & a_Chunk)
{
	if (GetTarget() != nullptr)
	{
		return;
	}

	cPlayerLookCheck Callback(GetPosition().addedY(GetHeight()), m_SightDistance);
	if (m_World->ForEachPlayer(Callback))
	{
		return;
	}

	ASSERT(Callback.GetPlayer() != nullptr);

	// Target the player:
	cAggressiveMonster::EventSeePlayer(Callback.GetPlayer(), a_Chunk);
	m_bIsScreaming = true;
	GetWorld()->BroadcastEntityMetadata(*this);

	// Being provoked BY the stare itself makes the enderman teleport away immediately, exactly once
	// (spec 3.2 - 1.12.2-era behaviour per the Chinese wiki history, 19w07a "no longer teleports
	// immediately, but stares first"; observed on live 1.12.2: it charges normally afterwards, even
	// while the provoking player keeps looking).  A per-tick stare teleport (an earlier reading of the
	// same note) made it blink away while being looked at and never close in - wrong.):
	TeleportRandomly(1);
}





void cEnderman::EventLosePlayer()
{
	Super::EventLosePlayer();
	m_bIsScreaming = false;
	GetWorld()->BroadcastEntityMetadata(*this);
}





void cEnderman::InStateChasing(std::chrono::milliseconds a_Dt, cChunk & a_Chunk)
{
	const auto Target = GetTarget();

	// No staring logic here: in 1.12.2 a provoked enderman charges while it is being looked at -
	// the stare only teleports it once, at the moment of the stare-provocation (spec 3.2, CheckEventSeePlayer):
	Super::InStateChasing(a_Dt, a_Chunk);  // Move towards the target

	if ((Target == nullptr) || !Target->IsPlayer())
	{
		// The random blink happens against player targets only - an enderman fighting other mobs does not do it
		// (spec 3.6.2, Chinese wiki).  It works at ANY distance (the same note): while the target is far, it closes
		// in; while the target is close, it lands on a random spot around it (the "behind the player" blink).
		// It also covers the pathfinder standstills of the tall (3-block-clearance) enderman, like Vanilla does:
		m_ChaseTeleportCooldown = std::chrono::milliseconds(0);
		return;
	}

	if (m_ChaseTeleportCooldown > std::chrono::milliseconds(0))
	{
		m_ChaseTeleportCooldown -= a_Dt;
		return;
	}

	m_ChaseTeleportCooldown = std::chrono::milliseconds(GetRandomProvider().RandInt(TELEPORT_CHASE_COOLDOWN_MIN_MS, TELEPORT_CHASE_COOLDOWN_MAX_MS));
	TeleportTowardsTarget();
}





bool cEnderman::DoTakeDamage(TakeDamageInfo & a_TDI)
{
	if (!Super::DoTakeDamage(a_TDI))
	{
		return false;
	}

	// Endermen always attempt to teleport upon taking damage, from any source (spec 3.6.1):
	// only damage from splash / lingering water bottles and soon-to-hit projectiles gets 64 attempts,
	// all other damage makes a single attempt (Chinese wiki "Teleportation");
	// Cuberite has no enderman water bottle damage nor projectile-impact prediction (spec 6), hence: always 1.
	if (GetHealth() > 0)
	{
		TeleportRandomly(1);
	}
	return true;
}





bool cEnderman::TeleportRandomly(unsigned int a_NumTries)
{
	if (IsRiding())
	{
		// Endermen in minecarts or boats do not try to teleport (spec 3.6, Chinese wiki "Teleportation"):
		return false;
	}

	// The 64x64x64 cube centered on the current position is covered by offsets -32 .. +31 on each axis (spec 3.6):
	const Vector3i Center = GetPosition().Floor();
	const Vector3i MinCorner = Center - Vector3i(TELEPORT_RANDOM_HALF_CUBE, TELEPORT_RANDOM_HALF_CUBE, TELEPORT_RANDOM_HALF_CUBE);
	const Vector3i MaxCorner = Center + Vector3i(TELEPORT_RANDOM_HALF_CUBE - 1, TELEPORT_RANDOM_HALF_CUBE - 1, TELEPORT_RANDOM_HALF_CUBE - 1);

	Vector3d Destination;
	if (!cPawn::FindTeleportDestination(*m_World, TELEPORT_LANDING_HEIGHT, a_NumTries, Destination, MinCorner, MaxCorner))
	{
		return false;
	}
	DoTeleport(Destination);
	return true;
}





bool cEnderman::TeleportTowardsTarget(void)
{
	if (IsRiding())
	{
		// Endermen in minecarts or boats do not try to teleport (spec 3.6, Chinese wiki "Teleportation"):
		return false;
	}

	const auto Target = GetTarget();
	if (Target == nullptr)
	{
		return false;
	}

	// The random destination lies within the 9x11x9 box around the chase center (spec 3.6.2):
	auto & Random = GetRandomProvider();
	const Vector3i Center = EndermanChaseTeleportCenter(GetPosition(), Target->GetPosition()).Floor();
	const Vector3i Point(
		Center.x + Random.RandInt(-TELEPORT_CHASE_BOX_HALF_XZ, TELEPORT_CHASE_BOX_HALF_XZ),
		Center.y + Random.RandInt(-TELEPORT_CHASE_BOX_HALF_Y, TELEPORT_CHASE_BOX_HALF_Y),
		Center.z + Random.RandInt(-TELEPORT_CHASE_BOX_HALF_XZ, TELEPORT_CHASE_BOX_HALF_XZ)
	);

	Vector3d Destination;
	if (!cPawn::FindTeleportDestination(*m_World, TELEPORT_LANDING_HEIGHT, 1, Destination, Point, Point))
	{
		return false;
	}
	DoTeleport(Destination);
	return true;
}





void cEnderman::DoTeleport(Vector3d a_Destination)
{
	// The teleport sound is played only at the destination (MC-94481, fixed in 1.9, spec 3.6):
	m_World->BroadcastSoundEffect("entity.endermen.teleport", a_Destination, 1.0f, 1.0f);

	// Portal particles at the origin and at the destination (spec 3.6):
	m_World->BroadcastParticleEffect("portal", static_cast<Vector3f>(GetPosition()), Vector3f{}, 0, TELEPORT_PARTICLE_AMOUNT);
	TeleportToCoords(a_Destination.x, a_Destination.y, a_Destination.z);
	m_World->BroadcastParticleEffect("portal", static_cast<Vector3f>(GetPosition()), Vector3f{}, 0, TELEPORT_PARTICLE_AMOUNT);
}





void cEnderman::Tick(std::chrono::milliseconds a_Dt, cChunk & a_Chunk)
{
	Super::Tick(a_Dt, a_Chunk);
	if (!IsTicking())
	{
		// The base class tick destroyed us
		return;
	}

	if (m_EMState != CHASING)
	{
		cMonster * EndermiteFound = GetMonsterOfTypeInSight(mtEndermite, 64);
		if (EndermiteFound != nullptr)
		{
			SetTarget(EndermiteFound);
			m_EMState = CHASING;
			m_bIsScreaming = true;
		}
	}
	else
	{
		const auto Target = GetTarget();
		if (Target != nullptr)
		{
			if (!Target->IsTicking())
			{
				m_EMState = IDLE;
				m_bIsScreaming = false;
			}
		}
	}

	PREPARE_REL_AND_CHUNK(GetPosition().Floor(), a_Chunk);
	if (!RelSuccess)
	{
		return;
	}

	if (GetHealth() <= 0)
	{
		// The death puff animation is still playing, don't run the teleport logic (spec 3.6):
		return;
	}

	// Track the time without a target, the sunlight teleport requires 600 game ticks of it (spec 3.6.3):
	if (GetTarget() == nullptr)
	{
		m_TimeWithoutTarget += a_Dt;
	}
	else
	{
		m_TimeWithoutTarget = std::chrono::milliseconds(0);
	}

	if (IsInWater() || Chunk->IsWeatherWetAt(Rel))
	{
		// Take damage when wet, and teleport repeatedly until a dry spot is found (spec 3.4):
		const Vector3d OldPosition = GetPosition();
		TakeDamage(dtEnvironment, nullptr, 1, 0);  // the damage itself may already have teleported us (DoTakeDamage)
		// Anger persists through water since 1.8 - the target is deliberately NOT dropped here (spec 3.3).
		if (GetPosition() == OldPosition)
		{
			TeleportRandomly(1);
		}
	}
	else if ((m_World->GetDimension() == dimOverworld) &&
		IsOnGround() &&
		(m_TimeWithoutTarget >= std::chrono::milliseconds(TELEPORT_SUNLIGHT_NO_TARGET_MS)) &&
		(Chunk->GetSkyLight(Rel) >= 15))
	{
		// Without a target, endermen in the overworld try to teleport away from open sunlight (spec 3.6.3,
		// Chinese wiki "Teleportation"): each tick, with probability P(Li) for the internal light level Li at the
		// eyes.  The time-altered light already encodes "day" - at night it drops to 4, so no separate time gate.
		// [guess] the eye block offset; [needs-check] "internal light" is taken as the time-altered skylight,
		// it might include block light as well, which cannot matter under the open-sky requirement:
		if (static_cast<int>(Rel.y) + TELEPORT_SUNLIGHT_EYE_BLOCK_OFFSET < static_cast<int>(cChunkDef::Height))
		{
			const auto EyeLight = Chunk->GetSkyLightAltered(Rel.x, Rel.y + TELEPORT_SUNLIGHT_EYE_BLOCK_OFFSET, Rel.z);
			if (GetRandomProvider().RandReal<double>() < EndermanSunlightTeleportChance(EyeLight))
			{
				TeleportRandomly(1);
			}
		}
	}
}
