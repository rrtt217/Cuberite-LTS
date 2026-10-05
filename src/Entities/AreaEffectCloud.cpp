#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "AreaEffectCloud.h"
#include "../BoundingBox.h"
#include "../ClientHandle.h"
#include "../Chunk.h"
#include "../World.h"
#include "Pawn.h"





/** Vertical half-extent within which the cloud affects entities, in blocks. */
static constexpr double AREA_EFFECT_CLOUD_AFFECT_HEIGHT = 0.5;

/** Default radius of a newly created cloud, in blocks. */
static constexpr float AREA_EFFECT_CLOUD_DEFAULT_RADIUS = 0.5f;

/** Largest radius a cloud may grow to; vanilla caps Radius at 32. */
static constexpr float AREA_EFFECT_CLOUD_MAX_RADIUS = 32.0f;

/** Default particle displayed by the cloud (the potion swirl, id 15 in the 1.8 particle table). */
static constexpr const char * AREA_EFFECT_CLOUD_DEFAULT_PARTICLE = "mobspell";

/** Default lifetime of a cloud after its wait time, in ticks. */
static constexpr int AREA_EFFECT_CLOUD_DEFAULT_DURATION = 600;

/** Default delay between two applications of the effect to the same entity, in ticks. */
static constexpr int AREA_EFFECT_CLOUD_DEFAULT_REAPPLICATION_DELAY = 10;





cAreaEffectCloud::cAreaEffectCloud(Vector3d a_Pos) :
	Super(etAreaEffectCloud, a_Pos, AREA_EFFECT_CLOUD_DEFAULT_RADIUS * 2, 0.5f),
	m_Age(0),
	m_WaitTime(0),
	m_Duration(AREA_EFFECT_CLOUD_DEFAULT_DURATION),
	m_DurationOnUse(0),
	m_ReapplicationDelay(AREA_EFFECT_CLOUD_DEFAULT_REAPPLICATION_DELAY),
	m_Radius(AREA_EFFECT_CLOUD_DEFAULT_RADIUS),
	m_RadiusPerTick(0.0f),
	m_RadiusOnUse(0.0f),
	m_Color(0),
	m_Particle(AREA_EFFECT_CLOUD_DEFAULT_PARTICLE)
{
}





void cAreaEffectCloud::SetRadius(float a_Radius)
{
	m_Radius = Clamp(a_Radius, 0.0f, AREA_EFFECT_CLOUD_MAX_RADIUS);
	UpdateWidth();
}





void cAreaEffectCloud::SetWaitTime(int a_WaitTime)
{
	m_WaitTime = std::max(a_WaitTime, 0);
}





void cAreaEffectCloud::UpdateWidth(void)
{
	SetWidth(m_Radius * 2);
}





void cAreaEffectCloud::AddEffect(cEntityEffect::eType a_Type, int a_Duration, short a_Amplifier)
{
	m_Effects.push_back({ a_Type, a_Duration, a_Amplifier });
}





void cAreaEffectCloud::ClearEffects(void)
{
	m_Effects.clear();
}





void cAreaEffectCloud::SpawnOn(cClientHandle & a_Client)
{
	a_Client.SendSpawnEntity(*this);
	a_Client.SendEntityMetadata(*this);
}





void cAreaEffectCloud::Tick(std::chrono::milliseconds a_Dt, cChunk & a_Chunk)
{
	Super::Tick(a_Dt, a_Chunk);
	if (!IsTicking())
	{
		return;
	}

	m_Age++;

	// Dissipate once the total lifetime is over (Duration == -1 means "never"):
	if (
		(m_Duration != -1) &&
		(
			(m_Age > m_WaitTime + m_Duration) ||
			((m_Duration <= 0) && (m_Age > m_WaitTime))
		)
	)
	{
		Destroy();
		return;
	}

	// Grow or shrink the cloud by its per-tick radius change:
	SetRadius(m_Radius + m_RadiusPerTick);
	if (m_Radius <= 0.0f)
	{
		Destroy();
		return;
	}

	// During the wait time the cloud does not apply anything:
	if (m_Age <= m_WaitTime)
	{
		return;
	}

	// Apply the stored effects to every pawn inside the cloud:
	const Vector3d Pos = GetPosition();
	const cBoundingBox Box(
		Pos - Vector3d(m_Radius, AREA_EFFECT_CLOUD_AFFECT_HEIGHT, m_Radius),
		Pos + Vector3d(m_Radius, AREA_EFFECT_CLOUD_AFFECT_HEIGHT, m_Radius)
	);
	bool Applied = false;
	m_World->ForEachEntityInBox(Box, [&](cEntity & a_Entity)
		{
			if (!a_Entity.IsPawn())
			{
				// Keep searching:
				return false;
			}

			const UInt32 ID = a_Entity.GetUniqueID();
			const auto Prev = m_NextApplicationTicks.find(ID);
			if ((Prev != m_NextApplicationTicks.end()) && (Prev->second > m_Age))
			{
				// This entity has been affected too recently:
				return false;
			}

			m_NextApplicationTicks[ID] = m_Age + m_ReapplicationDelay;

			auto & Pawn = static_cast<cPawn &>(a_Entity);
			for (const auto & Effect : m_Effects)
			{
				Pawn.AddEntityEffect(Effect.m_Type, Effect.m_Duration, Effect.m_Amplifier);
			}

			Applied = true;

			// Keep searching:
			return false;
		}
	);

	if (Applied)
	{
		SetRadius(m_Radius + m_RadiusOnUse);
		m_Duration += m_DurationOnUse;
	}
}
