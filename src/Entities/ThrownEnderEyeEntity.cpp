#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "ThrownEnderEyeEntity.h"
#include "FastRandom.h"





cThrownEnderEyeEntity::cThrownEnderEyeEntity(cEntity * a_Creator, Vector3d a_Pos, Vector3d a_Target):
	Super(pkEnderEye, a_Creator, a_Pos, Vector3d(), 0.25f, 0.25f),
	m_Target(a_Target)
{
	// The eye flies through blocks, so it must never be considered grounded:
	m_IsInGround = false;
}





void cThrownEnderEyeEntity::OnHitEntity(cEntity & a_EntityHit, Vector3d a_HitPos)
{
	// The eye flies through entities without affecting them:
	UNUSED(a_EntityHit);
	UNUSED(a_HitPos);
}





void cThrownEnderEyeEntity::OnHitSolidBlock(Vector3d a_HitPos, eBlockFace a_HitFace)
{
	// The eye flies through blocks (vanilla: "traveling through any blocks necessary"):
	UNUSED(a_HitPos);
	UNUSED(a_HitFace);
}





void cThrownEnderEyeEntity::Tick(std::chrono::milliseconds a_Dt, cChunk & a_Chunk)
{
	Super::Tick(a_Dt, a_Chunk);
	if (!IsTicking())
	{
		// The base class tick destroyed us:
		return;
	}
	if (m_TicksAlive >= LIFETIME_TICKS)
	{
		EndOfLife();
	}
}





void cThrownEnderEyeEntity::HandlePhysics(std::chrono::milliseconds a_Dt, cChunk & a_Chunk)
{
	UNUSED(a_Chunk);

	// The eye computes its own flight: it ignores gravity, blocks and entities, and instead moves towards
	// the target while climbing or diving, depending on how far away the target still is:
	const double DtSec = std::chrono::duration_cast<std::chrono::duration<double>>(a_Dt).count();
	const Vector3d Pos = GetPosition();
	const double ToTargetX = m_Target.x - Pos.x;
	const double ToTargetZ = m_Target.z - Pos.z;
	const double TargetDistance = sqrt((ToTargetX * ToTargetX) + (ToTargetZ * ToTargetZ));

	Vector3d Move;
	if (TargetDistance > 0.001)
	{
		// Move towards the target in the XZ plane, without overshooting it:
		const double Step = std::min(HORIZONTAL_SPEED * DtSec, TargetDistance);
		Move.x = ToTargetX / TargetDistance * Step;
		Move.z = ToTargetZ / TargetDistance * Step;
	}
	Move.y = ((TargetDistance > HIKE_DISTANCE) ? CLIMB_SPEED : -DIVE_SPEED) * DtSec;

	SetPosition(Pos + Move);
	SetSpeed((DtSec > 0) ? (Move / DtSec) : Vector3d());
}





void cThrownEnderEyeEntity::EndOfLife(void)
{
	const Vector3d Pos = GetPosition();

	// Vanilla plays the same sound whether the eye drops or breaks, and adds break particles only when it
	// shatters (Minecraft Wiki: Eye of Ender - Sounds and Locating strongholds):
	m_World->BroadcastSoundEffect("entity.ender_eye.death", Pos, 1.0f, 0.8f);
	if (GetRandomProvider().RandInt(99) < SHATTER_CHANCE)
	{
		m_World->BroadcastSoundParticleEffect(EffectID::PARTICLE_EYE_OF_ENDER, Pos, 0);
	}
	else
	{
		cItems Pickups;
		Pickups.Add(E_ITEM_EYE_OF_ENDER, 1);
		m_World->SpawnItemPickups(Pickups, Pos);
	}
	Destroy();
}
