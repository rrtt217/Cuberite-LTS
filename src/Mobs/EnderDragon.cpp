
#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "EnderDragon.h"
#include "../ClientHandle.h"
#include "../Entities/DragonFireballEntity.h"
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

/** Contact damage dealt by the dragon's head (Normal difficulty; Cuberite has no difficulty setting). */
static constexpr int CONTACT_DAMAGE_HEAD = 10;

/** Contact damage dealt by the dragon's wings / body (Normal difficulty). */
static constexpr int CONTACT_DAMAGE_WING = 5;

/** Knockback amount of a contact hit; the base damage code also launches the victim upwards. */
static constexpr double CONTACT_KNOCKBACK = 9.0;

/** Distance from the approximated head position within which a hit counts as a head hit, in blocks. */
static constexpr double HEAD_CONTACT_RADIUS = 3.0;

/** Ticks after the dragon takes damage during which it deals no contact damage (0.5 seconds). */
static constexpr int CONTACT_GRACE_TICKS = 10;

/** Ticks into the death animation after which the dragon starts dropping experience (vanilla: 150). */
static constexpr int ENDER_DRAGON_XP_DROP_TIME = 150;

/** Ticks the death animation lasts before the dragon is removed (vanilla: 200 ticks = 10 seconds). */
static constexpr int ENDER_DRAGON_DEATH_TIME = 200;

/** Horizontal speed of the dragon while strafing towards its target, in blocks per second. */
static constexpr double STRAFING_SPEED = 12.0;

/** How long a strafing run lasts; the dragon resumes circling afterwards. */
static constexpr int STRAFING_DURATION_TICKS = 60;

/** Distance from the target within which a strafing dragon shoots its fireball, in blocks. */
static constexpr double DRAGON_FIREBALL_RANGE = 64.0;

/** Speed at which a dragon fireball travels, in blocks per tick. */
static constexpr double DRAGON_FIREBALL_SPEED = 1.0;

/** Y that the dragon's feet descend to when perching, on top of the exit portal's central pillar.
The entity position is the bottom of its bounding box, and the generated fountain's pillar top is at
Y=66 (see cEnderDragonFightStructuresGen, fountain placed at Y=62), so the feet rest at Y=67. */
static constexpr double PERCH_HEIGHT = 67.0;

/** Distance within which the dragon considers itself landed on the exit portal, in blocks. */
static constexpr double PERCH_REACHED_DISTANCE = 4.0;

/** Ticks the dragon searches for a player after landing, before roaring (1.25 s). */
static constexpr int LANDED_SEARCH_TICKS = 25;

/** Ticks the roar lasts before the breath attack begins. */
static constexpr int LANDED_ROAR_TICKS = 10;

/** Duration of the breath attack itself (3 s); the damage cloud is not implemented yet. */
static constexpr int LANDED_BREATH_TICKS = 60;

/** Number of consecutive breath attacks before the dragon takes off again. */
static constexpr int MAX_BREATH_ATTACKS = 4;

/** Cumulative damage while perched that makes the dragon take off (25% of its max health). */
static constexpr float PERCH_ESCAPE_DAMAGE = 50.0f;

/** Distance from the exit portal within which a player triggers the roar / breath attack, in blocks. */
static constexpr double PERCH_ATTACK_RANGE = 20.0;

/** Distance within which a player keeps the dragon from taking off while perched, in blocks. */
static constexpr double PERCH_LOCATE_RANGE = 150.0;

/** Ticks the dragon spends taking off before it resumes circling. */
static constexpr int TAKEOFF_DURATION_TICKS = 20;

/** Speed at which the dragon flies to / from its perch, in blocks per second. */
static constexpr double PERCH_FLIGHT_SPEED = 10.0;





cEnderDragon::cEnderDragon(void) :
	Super("EnderDragon", mtEnderDragon, "entity.enderdragon.hurt", "entity.enderdragon.death", "entity.enderdragon.ambient", 16, 8),
	m_DragonPhase(eDragonPhase::Hovering),
	m_CrystalCount(0),
	m_CrystalCountCooldown(0),
	m_StrafingTicksLeft(0),
	m_LandedTicksLeft(0),
	m_BreathCount(0),
	m_PerchDamageTaken(0.0f),
	m_TakeoffTicksLeft(0),
	m_LastOrbitAngle(0),
	m_FireballFired(false),
	m_DragonDeathTime(0),
	m_DyingDamageType(dtAttack),
	m_DyingAttackerID(cEntity::INVALID_ID)
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
	if (GetHealth() <= 0)
	{
		// Dead: run our own 200-tick death timer. cMonster::Tick would remove us after just 1 second,
		// which would cut off the vanilla 10-second death animation.
		TickDeath();
		return;
	}

	Super::Tick(a_Dt, a_Chunk);
	if (!IsTicking())
	{
		// The base class tick destroyed us:
		return;
	}

	// A dragon in the hovering state is the harmless one created by /summon (and the default state):
	// it stays put and does not run any of the fight logic below.
	if (m_DragonPhase == eDragonPhase::Hovering)
	{
		SetSpeed(0, 0, 0);
		return;
	}

	// Recount the End crystals around the arena (throttled internally); this may start a strafe:
	UpdateCrystalCount();

	const double DtSec = std::chrono::duration_cast<std::chrono::duration<double>>(a_Dt).count();

	switch (m_DragonPhase)
	{
		case eDragonPhase::Circling:
		{
			Circling(DtSec);
			break;
		}
		case eDragonPhase::Strafing:
		{
			if ((--m_StrafingTicksLeft <= 0) || (GetTarget() == nullptr))
			{
				// A strafing dragon always resumes circling; it never falls back to hovering:
				SetDragonPhase(eDragonPhase::Circling);
			}
			else
			{
				Strafe(DtSec);

				// Vanilla fires a fireball as soon as the target is within 64 blocks:
				cEntity * Target = GetTarget();
				if (!m_FireballFired && (Target != nullptr) && ((Target->GetPosition() - GetPosition()).Length() <= DRAGON_FIREBALL_RANGE))
				{
					FireFireball(*Target);
					m_FireballFired = true;
				}
			}
			break;
		}
		case eDragonPhase::FlyingToPortal:
		{
			FlyToPortal(DtSec);
			break;
		}
		case eDragonPhase::LandedSearching:
		{
			// Perched, waiting before the roar: look for a player near the exit portal.
			SetSpeed(0, 0, 0);
			if (--m_LandedTicksLeft > 0)
			{
				break;
			}

			if (IsPlayerNearPortal(PERCH_ATTACK_RANGE))
			{
				m_LandedTicksLeft = LANDED_ROAR_TICKS;
				SetDragonPhase(eDragonPhase::LandedRoar);
			}
			else if (!IsPlayerNearPortal(PERCH_LOCATE_RANGE))
			{
				// No player anywhere near, give up the perch:
				StartTakeoff();
			}
			else
			{
				// A player is around but not close enough yet; keep searching:
				m_LandedTicksLeft = LANDED_SEARCH_TICKS;
			}
			break;
		}
		case eDragonPhase::LandedRoar:
		{
			// Perched and roaring before the breath attack:
			SetSpeed(0, 0, 0);
			if (--m_LandedTicksLeft <= 0)
			{
				m_LandedTicksLeft = LANDED_BREATH_TICKS;
				SetDragonPhase(eDragonPhase::LandedBreath);
			}
			break;
		}
		case eDragonPhase::LandedBreath:
		{
			// Perched, performing the breath attack. The damage cloud itself is not implemented yet,
			// so the attack only counts towards the take-off limit:
			SetSpeed(0, 0, 0);
			if (--m_LandedTicksLeft > 0)
			{
				break;
			}

			++m_BreathCount;
			if ((m_BreathCount >= MAX_BREATH_ATTACKS) || !IsPlayerNearPortal(PERCH_LOCATE_RANGE))
			{
				StartTakeoff();
			}
			else
			{
				m_LandedTicksLeft = LANDED_SEARCH_TICKS;
				SetDragonPhase(eDragonPhase::LandedSearching);
			}
			break;
		}
		case eDragonPhase::TakingOff:
		{
			// Fly straight up for a moment, then resume circling:
			SetSpeed(0, PERCH_FLIGHT_SPEED, 0);
			if (--m_TakeoffTicksLeft <= 0)
			{
				SetDragonPhase(eDragonPhase::Circling);
			}
			break;
		}
		case eDragonPhase::Dying:
		{
			Dying(DtSec);
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

	// Damage the entities we are touching (wings / head):
	AttackEntities();
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

	// Each completed orbit, roll vanilla's 1 / (3 + crystals) chance to go and perch on the exit portal:
	const double Angle = atan2(PosZ, PosX);
	if ((m_LastOrbitAngle > (M_PI / 2)) && (Angle < -(M_PI / 2)) && GetRandomProvider().RandBool(1.0 / (3 + m_CrystalCount)))
	{
		StartPerching();
		return;
	}
	m_LastOrbitAngle = Angle;

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

	// Face the direction of flight, so that the head (and its contact damage) points forward:
	FaceSpeedDirection();
}





void cEnderDragon::Strafe(double a_Dt)
{
	UNUSED(a_Dt);

	const cEntity * Target = GetTarget();
	if (Target == nullptr)
	{
		return;
	}

	// Fly straight at the target's head. Vanilla shoots a dragon fireball once within 64 blocks;
	// that projectile is not implemented yet, so the run simply ends after STRAFING_DURATION_TICKS:
	Vector3d ToTarget = Target->GetPosition().addedY(Target->GetHeight()) - GetPosition();
	const double Distance = ToTarget.Length();
	if (Distance > 0.01)
	{
		ToTarget *= (STRAFING_SPEED / Distance);
	}

	SetSpeed(ToTarget);
	FaceSpeedDirection();
}





void cEnderDragon::StartStrafing(void)
{
	// Vanilla strafes towards the player; without a target there is nothing to strafe at:
	if (GetTarget() == nullptr)
	{
		return;
	}

	m_StrafingTicksLeft = STRAFING_DURATION_TICKS;
	m_FireballFired = false;
	SetDragonPhase(eDragonPhase::Strafing);
}





void cEnderDragon::FireFireball(cEntity & a_Target)
{
	// Fire from the front of the dragon's head (the look vector points at the back, see FaceSpeedDirection):
	const Vector3d Start = GetPosition().addedY(GetHeight() / 2) - (GetLookVector() * (GetWidth() / 2));
	const Vector3d Direction = a_Target.GetPosition() - Start;
	const double Length = Direction.Length();
	if (Length < 0.001)
	{
		return;
	}

	auto Fireball = std::make_unique<cDragonFireballEntity>(this, Start, Direction * (DRAGON_FIREBALL_SPEED / Length));
	Fireball->Initialize(std::move(Fireball), *m_World);
}





void cEnderDragon::StartPerching(void)
{
	SetDragonPhase(eDragonPhase::FlyingToPortal);
}





void cEnderDragon::FlyToPortal(double a_Dt)
{
	UNUSED(a_Dt);

	Vector3d ToPortal = Vector3d(0, PERCH_HEIGHT, 0) - GetPosition();
	const double Distance = ToPortal.Length();
	if (Distance < PERCH_REACHED_DISTANCE)
	{
		// Landed: look for a nearby player first (vanilla waits 1.25 s before roaring):
		SetSpeed(0, 0, 0);
		m_BreathCount = 0;
		m_PerchDamageTaken = 0.0f;
		m_LandedTicksLeft = LANDED_SEARCH_TICKS;
		SetDragonPhase(eDragonPhase::LandedSearching);
		return;
	}

	ToPortal *= (PERCH_FLIGHT_SPEED / Distance);
	SetSpeed(ToPortal);
	FaceSpeedDirection();
}





void cEnderDragon::StartTakeoff(void)
{
	m_TakeoffTicksLeft = TAKEOFF_DURATION_TICKS;
	SetDragonPhase(eDragonPhase::TakingOff);
}





bool cEnderDragon::IsPerched(void) const
{
	switch (m_DragonPhase)
	{
		case eDragonPhase::LandedSearching:
		case eDragonPhase::LandedRoar:
		case eDragonPhase::LandedBreath:
		{
			return true;
		}
		default:
		{
			return false;
		}
	}
}





bool cEnderDragon::IsPlayerNearPortal(double a_Range) const
{
	bool Found = false;
	const Vector3d PortalPos(0, PERCH_HEIGHT, 0);
	const double SqrRange = a_Range * a_Range;
	m_World->ForEachPlayer([&Found, SqrRange, &PortalPos](cPlayer & a_Player)
		{
			if ((a_Player.GetPosition() - PortalPos).SqrLength() < SqrRange)
			{
				Found = true;
				return true;  // Stop searching
			}

			// Keep searching:
			return false;
		}
	);
	return Found;
}





void cEnderDragon::Dying(double a_Dt)
{
	UNUSED(a_Dt);

	Vector3d ToPortal = Vector3d(0, PERCH_HEIGHT, 0) - GetPosition();
	const double Distance = ToPortal.Length();
	if (Distance < PERCH_REACHED_DISTANCE)
	{
		FinishDying();
		return;
	}

	ToPortal *= (PERCH_FLIGHT_SPEED / Distance);
	SetSpeed(ToPortal);
	FaceSpeedDirection();
}





void cEnderDragon::FinishDying(void)
{
	if (GetHealth() <= 0)
	{
		// Already dead, don't run the death sequence twice:
		return;
	}

	TakeDamageInfo TDI;
	TDI.DamageType = m_DyingDamageType;
	TDI.Attacker = nullptr;
	TDI.RawDamage = 0;
	TDI.FinalDamage = 0;

	Super::KilledBy(TDI);

	// Record the kill so that the dragon does not respawn after a server restart:
	m_World->SetEnderDragonKilled();

	// Start the vanilla death timer (experience at 150 ticks, removal at 200 ticks):
	m_DragonDeathTime = 0;
}





void cEnderDragon::TickDeath(void)
{
	m_DragonDeathTime++;

	// Vanilla starts dropping the experience once the dragon has been dead for 150 ticks, but only
	// when a player was involved in the kill:
	if ((m_DragonDeathTime == ENDER_DRAGON_XP_DROP_TIME) && (m_DyingAttackerID != cEntity::INVALID_ID))
	{
		m_World->SpawnSplitExperienceOrbs(GetPosX(), GetPosY(), GetPosZ(), 12000);
	}

	// Once the 10-second (200-tick) death animation is over, activate the exit portal, place the
	// dragon egg and remove the dragon:
	if (m_DragonDeathTime >= ENDER_DRAGON_DEATH_TIME)
	{
		m_World->ActivateEnderDragonExitPortal();
		Destroy();
	}
}





void cEnderDragon::FaceSpeedDirection(void)
{
	// cEntity::SetYawFromSpeed() uses atan2(speed.x, speed.z), which is mirrored in X relative to the
	// engine's canonical direction (VectorToEuler, used by cMonster for mobs). The ender dragon's model
	// is additionally rotated 180 degrees relative to ordinary mobs, so the canonical direction gets a
	// half turn. Measured in-game: with the plain canonical yaw the dragon flies head-first backwards.
	double Yaw, Pitch;
	const Vector3d & Speed = GetSpeed();
	VectorToEuler(Speed.x, Speed.y, Speed.z, Yaw, Pitch);
	Yaw += 180.0;
	SetYaw(static_cast<float>(Yaw));
	SetHeadYaw(Yaw);
	SetPitch(static_cast<float>(Pitch));
}





bool cEnderDragon::Attack(std::chrono::milliseconds a_Dt)
{
	UNUSED(a_Dt);

	// The dragon damages entities through contact (see AttackEntities), not through the generic melee:
	return false;
}





void cEnderDragon::AttackEntities(void)
{
	// Vanilla applies no contact damage for half a second after the dragon itself took damage:
	if (m_TicksSinceLastDamaged < CONTACT_GRACE_TICKS)
	{
		return;
	}

	// Approximate the head as a point at the front of the bounding box, at half the dragon's height.
	// FaceSpeedDirection() stores the yaw with a half turn offset (relative to the model), so the
	// look vector points at the back of the model; the head is at its opposite end:
	const Vector3d HeadPos = GetPosition().addedY(GetHeight() / 2) - (GetLookVector() * (GetWidth() / 2));

	m_World->ForEachEntityInBox(GetBoundingBox(), [&](cEntity & a_Entity)
		{
			if ((&a_Entity == this) || !a_Entity.IsPawn())
			{
				// Keep searching:
				return false;
			}

			// The head deals more damage than the wings / body:
			const bool IsHead = ((a_Entity.GetPosition() - HeadPos).SqrLength() < (HEAD_CONTACT_RADIUS * HEAD_CONTACT_RADIUS));
			a_Entity.TakeDamage(dtMobAttack, this, (IsHead ? CONTACT_DAMAGE_HEAD : CONTACT_DAMAGE_WING), CONTACT_KNOCKBACK);

			// Keep searching:
			return false;
		}
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

	if (Count < m_CrystalCount)
	{
		// An End crystal was destroyed (vanilla switches to strafing when that happens):
		StartStrafing();
	}

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





bool cEnderDragon::IsDamageSourceAllowed(const TakeDamageInfo & a_TDI) const
{
	// Explosions always hurt the dragon:
	if (a_TDI.DamageType == dtExplosion)
	{
		return true;
	}

	// Any damage dealt by a player (melee, arrows, ...) hurts the dragon:
	return ((a_TDI.Attacker != nullptr) && a_TDI.Attacker->IsPlayer());
}





bool cEnderDragon::DoTakeDamage(TakeDamageInfo & a_TDI)
{
	// The ender dragon is immune to everything except explosions and damage dealt by players:
	if (!IsDamageSourceAllowed(a_TDI))
	{
		return false;
	}

	// While perched, vanilla makes the dragon immune to arrows and thrown tridents:
	if (IsPerched() && (a_TDI.DamageType == dtRangedAttack))
	{
		return false;
	}

	if (!Super::DoTakeDamage(a_TDI))
	{
		return false;
	}

	// Vanilla takes off when the cumulative damage taken while perched exceeds 50 (25% of max health):
	if (IsPerched())
	{
		m_PerchDamageTaken += a_TDI.FinalDamage;
		if (m_PerchDamageTaken > PERCH_ESCAPE_DAMAGE)
		{
			m_PerchDamageTaken = 0.0f;
			StartTakeoff();
		}
	}

	m_World->BroadcastBossBarUpdateHealth(*this, GetUniqueID(), GetHealth() / GetMaxHealth());
	return true;
}





void cEnderDragon::KilledBy(TakeDamageInfo & a_TDI)
{
	if (m_DragonPhase == eDragonPhase::Dying)
	{
		// Already flying to the exit portal, this is the real death:
		Super::KilledBy(a_TDI);
		return;
	}

	// Vanilla does not die immediately: keep 1 HP and fly to the exit portal first.
	m_Health = 1;
	m_DyingDamageType = a_TDI.DamageType;
	m_DyingAttackerID = ((a_TDI.Attacker != nullptr) ? a_TDI.Attacker->GetUniqueID() : cEntity::INVALID_ID);
	SetDragonPhase(eDragonPhase::Dying);
}





void cEnderDragon::GetDrops(cItems & a_Drops, cEntity * a_Killer)
{
	// No drops
}





void cEnderDragon::SpawnOn(cClientHandle & a_Client)
{
	Super::SpawnOn(a_Client);

	// The ender dragon's boss bar is pink (not red), has no divisions and plays boss music / creates fog:
	// Ref: https://minecraft.wiki/w/Bossbar ("the ender dragon has a pink bossbar")
	a_Client.SendBossBarAdd(GetUniqueID(), cCompositeChat("Ender Dragon"), GetHealth() / GetMaxHealth(), BossBarColor::Pink, BossBarDivisionType::None, false, true, true);
}
