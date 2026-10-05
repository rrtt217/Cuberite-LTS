
#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "EnderDragon.h"
#include "../ClientHandle.h"
#include "../CompositeChat.h"





/** Horizontal speed of the dragon while circling, in blocks per second. */
static constexpr double CIRCLING_SPEED = 8.0;

/** Radius the dragon flies at while circling while End crystals are still present (outside the pillar ring). */
static constexpr double CIRCLING_RADIUS_OUTSIDE = 48.0;

/** Radius the dragon flies at while circling after the End crystals are gone (inside the pillar ring). */
static constexpr double CIRCLING_RADIUS_INSIDE = 20.0;

/** Height the dragon flies at while circling, in blocks. */
static constexpr double CIRCLING_HEIGHT = 80.0;

/** Fraction of the distance to the circling path that the dragon corrects each tick. */
static constexpr double CIRCLING_CORRECTION = 0.05;

/** Half-size of the box around the world centre in which End crystals are counted, in blocks. */
static constexpr double CRYSTAL_SEARCH_RADIUS = 64.0;

/** Ticks between two End crystal recounts. */
static constexpr int CRYSTAL_COUNT_INTERVAL = 20;





cEnderDragon::cEnderDragon(void) :
	Super("EnderDragon", mtEnderDragon, "entity.enderdragon.hurt", "entity.enderdragon.death", "entity.enderdragon.ambient", 16, 8),
	m_DragonPhase(eDragonPhase::Hovering),
	m_CrystalCount(0),
	m_CrystalCountCooldown(0)
{
	// Fly freely: the flight code in Tick() controls our speed, so disable gravity and air drag:
	SetGravity(0);
	SetAirDrag(0);

	// The vanilla client derives the entity IDs of the dragon's parts from the dragon's own ID
	// (dragonID + 1 .. dragonID + PART_COUNT), so reserve that whole block and take the first ID
	// as our own. This keeps the client-derived part IDs free of any other server-side entity.
	const UInt32 BlockStart = cEntity::ReserveUniqueIDs(1 + PART_COUNT);
	m_UniqueID = BlockStart;
}





bool cEnderDragon::IsPartID(UInt32 a_ID) const
{
	const UInt32 DragonID = GetUniqueID();
	return ((a_ID > DragonID) && (a_ID <= DragonID + PART_COUNT));
}





void cEnderDragon::SetDragonPhase(eDragonPhase a_Phase)
{
	if (m_DragonPhase == a_Phase)
	{
		return;
	}

	m_DragonPhase = a_Phase;

	// Let the clients know, so that they can play the matching animation:
	if (m_World != nullptr)
	{
		m_World->BroadcastEntityMetadata(*this);
	}
}





void cEnderDragon::Tick(std::chrono::milliseconds a_Dt, cChunk & a_Chunk)
{
	Super::Tick(a_Dt, a_Chunk);
	if (!IsTicking())
	{
		// The base class tick destroyed us:
		return;
	}

	// Recount the End crystals around the arena (throttled internally):
	UpdateCrystalCount();

	// Placeholder phase selection until the actual fight logic exists: circle while a player is
	// targeted, otherwise hover in place.
	SetDragonPhase((GetTarget() != nullptr) ? eDragonPhase::Circling : eDragonPhase::Hovering);

	switch (m_DragonPhase)
	{
		case eDragonPhase::Circling:
		{
			Circling(std::chrono::duration_cast<std::chrono::duration<double>>(a_Dt).count());
			break;
		}
		case eDragonPhase::Hovering:
		default:
		{
			// Hover in place: stop any leftover speed so that we don't drift:
			SetSpeed(0, 0, 0);
			break;
		}
	}
}





void cEnderDragon::HandlePhysics(std::chrono::milliseconds a_Dt, cChunk & a_Chunk)
{
	UNUSED(a_Chunk);

	// The dragon flies freely and phases through blocks, so skip the base class collision physics and
	// just integrate the speed that the flight code in Tick() has set:
	AddPosition(GetSpeed() * std::chrono::duration_cast<std::chrono::duration<double>>(a_Dt).count());
}





void cEnderDragon::Circling(double a_Dt)
{
	UNUSED(a_Dt);

	const double PosX = GetPosX();
	const double PosZ = GetPosZ();
	const double Radius = std::max(sqrt((PosX * PosX) + (PosZ * PosZ)), 0.001);

	// Vanilla circles the ring of obsidian pillars on the outside while End crystals are still there,
	// and on the inside once all of them have been destroyed:
	const double TargetRadius = (m_CrystalCount > 0) ? CIRCLING_RADIUS_OUTSIDE : CIRCLING_RADIUS_INSIDE;

	// Fly tangentially around the world centre, correcting towards the circling radius and height:
	const double TangentX = -PosZ / Radius;
	const double TangentZ =  PosX / Radius;
	const double RadialCorrection = (TargetRadius - Radius) * CIRCLING_CORRECTION;

	SetSpeed(
		(TangentX * CIRCLING_SPEED) + ((PosX / Radius) * RadialCorrection),
		(CIRCLING_HEIGHT - GetPosY()) * CIRCLING_CORRECTION,
		(TangentZ * CIRCLING_SPEED) + ((PosZ / Radius) * RadialCorrection)
	);
}





void cEnderDragon::UpdateCrystalCount(void)
{
	if (--m_CrystalCountCooldown > 0)
	{
		return;
	}
	m_CrystalCountCooldown = CRYSTAL_COUNT_INTERVAL;

	UInt32 Count = 0;
	const cBoundingBox SearchBox(
		Vector3d(-CRYSTAL_SEARCH_RADIUS, 0, -CRYSTAL_SEARCH_RADIUS),
		Vector3d(CRYSTAL_SEARCH_RADIUS, cChunkDef::Height, CRYSTAL_SEARCH_RADIUS)
	);
	m_World->ForEachEntityInBox(SearchBox, [&Count](cEntity & a_Entity)
		{
			if (a_Entity.IsEnderCrystal())
			{
				++Count;
			}

			// Keep searching:
			return false;
		}
	);

	m_CrystalCount = Count;
}





void cEnderDragon::TakeDamageFromPart(cEntity & a_Attacker, bool a_IsHead)
{
	const int RawDamage = a_Attacker.GetRawDamageAgainst(*this);

	// Vanilla applies original / 4 + min(1, original) to every part except the head, which takes full damage.
	// Ref: https://minecraft.wiki/w/Ender_Dragon (Behavior)
	// Note: this reduces the base damage only; enchantment / critical-hit bonuses are added later by
	// cEntity::DoTakeDamage and therefore are not reduced, unlike vanilla, which reduces the total.
	const float FinalDamage = a_IsHead ?
		static_cast<float>(RawDamage) :
		((RawDamage / 4.0f) + std::min(1.0f, static_cast<float>(RawDamage)));

	TakeDamage(dtAttack, &a_Attacker, RawDamage, FinalDamage, a_Attacker.GetKnockbackAmountAgainst(*this));
}





bool cEnderDragon::DoTakeDamage(TakeDamageInfo & a_TDI)
{
	if (!Super::DoTakeDamage(a_TDI))
	{
		return false;
	}

	m_World->BroadcastBossBarUpdateHealth(*this, GetUniqueID(), GetHealth() / GetMaxHealth());
	return true;
}





void cEnderDragon::GetDrops(cItems & a_Drops, cEntity * a_Killer)
{
	// No drops
}





void cEnderDragon::SpawnOn(cClientHandle & a_Client)
{
	Super::SpawnOn(a_Client);

	// Red boss bar with no divisions that plays boss music and creates fog:
	a_Client.SendBossBarAdd(GetUniqueID(), cCompositeChat("Ender Dragon"), GetHealth() / GetMaxHealth(), BossBarColor::Red, BossBarDivisionType::None, false, true, true);
}
