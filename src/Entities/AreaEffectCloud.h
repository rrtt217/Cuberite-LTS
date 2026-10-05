#pragma once

#include "Entity.h"
#include "EntityEffect.h"





/** A cloud that applies status effects to entities standing inside it.
Vanilla creates these for lingering potions, creeper explosions and dragon fireballs.
Ref: https://minecraft.wiki/w/Area_Effect_Cloud */
class cAreaEffectCloud :
	public cEntity
{
	using Super = cEntity;

public:

	CLASS_PROTODEF(cAreaEffectCloud)

	cAreaEffectCloud(Vector3d a_Pos);

	/** One status effect that the cloud applies. */
	struct sEffect
	{
		cEntityEffect::eType m_Type;
		int m_Duration;
		short m_Amplifier;
	};

	// Configuration:

	float GetRadius(void) const { return m_Radius; }
	void SetRadius(float a_Radius);
	void SetRadiusPerTick(float a_RadiusPerTick) { m_RadiusPerTick = a_RadiusPerTick; }
	void SetRadiusOnUse(float a_RadiusOnUse) { m_RadiusOnUse = a_RadiusOnUse; }

	/** Returns whether the cloud can be bottled for dragon's breath. */
	bool CanBeCollected(void) const { return m_CanBeCollected; }
	void SetCanBeCollected(bool a_CanBeCollected) { m_CanBeCollected = a_CanBeCollected; }

	int GetAge(void) const { return m_Age; }
	void SetAge(int a_Age) { m_Age = a_Age; }

	float GetRadiusPerTick(void) const { return m_RadiusPerTick; }
	float GetRadiusOnUse(void) const { return m_RadiusOnUse; }
	int GetDurationOnUse(void) const { return m_DurationOnUse; }

	/** Returns the status effects that the cloud applies. */
	const std::vector<sEffect> & GetEffects(void) const { return m_Effects; }

	int GetWaitTime(void) const { return m_WaitTime; }
	void SetWaitTime(int a_WaitTime);

	int GetDuration(void) const { return m_Duration; }
	void SetDuration(int a_Duration) { m_Duration = a_Duration; }
	void SetDurationOnUse(int a_DurationOnUse) { m_DurationOnUse = a_DurationOnUse; }

	int GetReapplicationDelay(void) const { return m_ReapplicationDelay; }
	void SetReapplicationDelay(int a_ReapplicationDelay) { m_ReapplicationDelay = a_ReapplicationDelay; }

	int GetColor(void) const { return m_Color; }
	void SetColor(int a_Color) { m_Color = a_Color; }

	const AString & GetParticle(void) const { return m_Particle; }
	void SetParticle(const AString & a_Particle) { m_Particle = a_Particle; }

	/** Adds a status effect that the cloud applies to pawns inside it. */
	void AddEffect(cEntityEffect::eType a_Type, int a_Duration, short a_Amplifier);

	/** Removes all stored status effects. */
	void ClearEffects(void);

	// cEntity overrides:
	virtual void Tick(std::chrono::milliseconds a_Dt, cChunk & a_Chunk) override;
	virtual void SpawnOn(cClientHandle & a_Client) override;
	virtual void OnRightClicked(cPlayer & a_Player) override;

private:

	/** Ticks the cloud has existed for. */
	int m_Age;

	/** Ticks the cloud waits before applying effects. The radius is ignored until then. */
	int m_WaitTime;

	/** Ticks the cloud lives after m_WaitTime; -1 means it never expires by itself. */
	int m_Duration;

	/** Change of m_Duration whenever an effect is applied. */
	int m_DurationOnUse;

	/** Ticks before an entity can be affected again. */
	int m_ReapplicationDelay;

	/** Current radius. */
	float m_Radius;

	/** Change of m_Radius per tick. */
	float m_RadiusPerTick;

	/** Change of m_Radius whenever an effect is applied. */
	float m_RadiusOnUse;

	/** Particle colour (RGB). */
	int m_Color;

	/** Particle displayed by the cloud. */
	AString m_Particle;

	/** Whether the cloud can be bottled for dragon's breath (true for dragon / dragon fireball clouds). */
	bool m_CanBeCollected;

	/** Effects applied to pawns inside the cloud. */
	std::vector<sEffect> m_Effects;

	/** Maps an affected entity's unique ID to the tick at which it may be affected again. */
	std::unordered_map<UInt32, int> m_NextApplicationTicks;

	/** Updates the entity width from the current radius. */
	void UpdateWidth(void);
};
