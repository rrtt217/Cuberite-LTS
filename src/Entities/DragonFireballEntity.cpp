#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "DragonFireballEntity.h"
#include "AreaEffectCloud.h"
#include "../World.h"





/** Ticks between a dragon fireball hitting a block and bursting into its cloud (0.5 s). */
static constexpr int DRAGON_FIREBALL_BURST_DELAY = 10;

/** Initial radius of the cloud left behind, in blocks. */
static constexpr float DRAGON_FIREBALL_CLOUD_RADIUS_START = 3.0f;

/** Radius the cloud grows to over its lifetime, in blocks. */
static constexpr float DRAGON_FIREBALL_CLOUD_RADIUS_END = 5.0f;

/** Lifetime of the cloud, in ticks (30 seconds). */
static constexpr int DRAGON_FIREBALL_CLOUD_DURATION = 600;

/** Delay between two applications of the cloud's effect to the same entity, in ticks. */
static constexpr int DRAGON_FIREBALL_CLOUD_REAPPLICATION_DELAY = 10;

/** Colour of the purple cloud. */
static constexpr int DRAGON_FIREBALL_CLOUD_COLOR = 0x800080;

/** Particle displayed by the cloud. */
static constexpr const char * DRAGON_FIREBALL_CLOUD_PARTICLE = "mobspell";





cDragonFireballEntity::cDragonFireballEntity(cEntity * a_Creator, Vector3d a_Pos, Vector3d a_Speed) :
	Super(pkDragonFireball, a_Creator, a_Pos, a_Speed, 1.0f, 1.0f),
	m_BurstTicksLeft(-1)
{
	// Fly straight, like the other fireballs:
	SetGravity(0);
	SetAirDrag(0);
}





void cDragonFireballEntity::Tick(std::chrono::milliseconds a_Dt, cChunk & a_Chunk)
{
	// Once the fuse is lit, stop flying and just wait for the burst:
	if (m_BurstTicksLeft >= 0)
	{
		if (--m_BurstTicksLeft <= 0)
		{
			Burst();
			Destroy();
		}
		return;
	}

	Super::Tick(a_Dt, a_Chunk);
}





void cDragonFireballEntity::OnHitSolidBlock(Vector3d a_HitPos, eBlockFace a_HitFace)
{
	UNUSED(a_HitFace);

	// Light the fuse; the cloud appears half a second later:
	SetPosition(a_HitPos);
	SetSpeed(0, 0, 0);
	m_BurstTicksLeft = DRAGON_FIREBALL_BURST_DELAY;
}





void cDragonFireballEntity::OnHitEntity(cEntity & a_EntityHit, Vector3d a_HitPos)
{
	// Dragon fireballs deal no damage or knockback and pass through entities:
	UNUSED(a_EntityHit);
	UNUSED(a_HitPos);
}





void cDragonFireballEntity::Burst(void)
{
	auto Cloud = std::make_unique<cAreaEffectCloud>(GetPosition());
	Cloud->SetRadius(DRAGON_FIREBALL_CLOUD_RADIUS_START);
	Cloud->SetDuration(DRAGON_FIREBALL_CLOUD_DURATION);
	Cloud->SetRadiusPerTick((DRAGON_FIREBALL_CLOUD_RADIUS_END - DRAGON_FIREBALL_CLOUD_RADIUS_START) / DRAGON_FIREBALL_CLOUD_DURATION);
	Cloud->SetRadiusOnUse(0.0f);  // Unlike lingering potions, this cloud does not shrink on use
	Cloud->SetDurationOnUse(0);
	Cloud->SetReapplicationDelay(DRAGON_FIREBALL_CLOUD_REAPPLICATION_DELAY);
	Cloud->SetColor(DRAGON_FIREBALL_CLOUD_COLOR);
	Cloud->SetParticle(DRAGON_FIREBALL_CLOUD_PARTICLE);
	Cloud->SetCanBeCollected(true);
	Cloud->AddEffect(cEntityEffect::effInstantDamage, 1, 1);  // Instant Damage II
	Cloud->Initialize(std::move(Cloud), *m_World);
}
