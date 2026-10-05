
#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "EnderDragon.h"
#include "../ClientHandle.h"
#include "../CompositeChat.h"





cEnderDragon::cEnderDragon(void) :
	Super("EnderDragon", mtEnderDragon, "entity.enderdragon.hurt", "entity.enderdragon.death", "entity.enderdragon.ambient", 16, 8)
{
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





void cEnderDragon::TakeDamageFromPart(cEntity & a_Attacker, bool a_IsHead)
{
	const int RawDamage = a_Attacker.GetRawDamageAgainst(*this);

	// Vanilla applies original / 4 + min(1, original) to every part except the head, which takes full damage.
	// Ref: https://minecraft.wiki/w/Ender_Dragon (Behavior)
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
