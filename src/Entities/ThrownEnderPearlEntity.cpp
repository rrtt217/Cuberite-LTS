
#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "FastRandom.h"
#include "Mobs/MonsterTypes.h"
#include "ThrownEnderPearlEntity.h"
#include "Player.h"





cThrownEnderPearlEntity::cThrownEnderPearlEntity(cEntity * a_Creator, Vector3d a_Pos, Vector3d a_Speed):
	Super(pkEnderPearl, a_Creator, a_Pos, a_Speed, 0.25f, 0.25f)
{
}





void cThrownEnderPearlEntity::OnHitEntity(cEntity & a_EntityHit, Vector3d a_HitPos)
{
	Super::OnHitEntity(a_EntityHit, a_HitPos);

	int Damage = 0;
	if (a_EntityHit.IsEnderCrystal())
	{
		// Endercrystals are destroyed:
		Damage = CeilC(a_EntityHit.GetHealth());
	}

	a_EntityHit.TakeDamage(dtRangedAttack, this, Damage, 1);
	TeleportCreator(a_HitPos);
	Destroy();
}





void cThrownEnderPearlEntity::OnHitSolidBlock(Vector3d a_HitPos, eBlockFace a_HitFace)
{
	Super::OnHitSolidBlock(a_HitPos, a_HitFace);

	TeleportCreator(a_HitPos);
	Destroy();
}





bool cThrownEnderPearlEntity::DetectPortal(void)
{
	const auto Position = GetPosition().Floor();
	const bool InGateway = cChunkDef::IsValidHeight(Position) && (GetWorld()->GetBlock(Position) == E_BLOCK_END_GATEWAY);
	if (!InGateway)
	{
		return Super::DetectPortal();
	}

	// A pearl thrown into an End gateway activates the gateway for its thrower and is consumed,
	// without any of the usual ender pearl effects (fall damage, endermites):
	Vector3d ExitPos;
	if (GetWorld()->GetEnderDragonGatewayExit(Position, true, ExitPos))
	{
		TeleportCreatorToGateway(ExitPos);
		Destroy();
		return true;
	}

	// The linked gateway is still being generated; let the pearl keep flying for now:
	return false;
}





void cThrownEnderPearlEntity::TeleportCreatorToGateway(Vector3d a_ExitPos)
{
	if (m_CreatorData.m_Name.empty())
	{
		return;
	}

	GetWorld()->FindAndDoWithPlayer(m_CreatorData.m_Name, [a_ExitPos](cPlayer & a_Entity)
	{
		a_Entity.TeleportToCoords(a_ExitPos.x, a_ExitPos.y, a_ExitPos.z);
		return false;
	});
}





void cThrownEnderPearlEntity::TeleportCreator(Vector3d a_HitPos)
{
	if (m_CreatorData.m_Name.empty())
	{
		return;
	}



	GetWorld()->FindAndDoWithPlayer(m_CreatorData.m_Name, [=](cPlayer & a_Entity)
	{

		auto & Random = GetRandomProvider();

		// 5% chance to spawn an endermite
		if (Random.RandBool(0.05))
		{
			Vector3d PlayerPosition = a_Entity.GetPosition();
			m_World->SpawnMob(PlayerPosition.x, PlayerPosition.y, PlayerPosition.z, mtEndermite);
		}


		// Teleport the creator here, make them take 5 damage:
		a_Entity.TeleportToCoords(a_HitPos.x, a_HitPos.y + 0.2, a_HitPos.z);
		a_Entity.TakeDamage(dtEnderPearl, this, 5, 0);

		return false;
	});
}
