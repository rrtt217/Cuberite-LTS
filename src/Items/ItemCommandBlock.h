
// ItemCommandBlock.h

// Declares the cItemCommandBlockHandler class representing the handler for the three command block
// items. Command blocks inherit the placement facing of dispensers / pistons: the block faces the
// player, using the shared cDisplacementYawRotator (spec section 2.4).

#pragma once

#include "ItemHandler.h"
#include "Blocks/BlockDropSpenser.h"





class cItemCommandBlockHandler final  :
	public cItemHandler
{
	using Super = cItemHandler;

public:

	using Super::Super;

private:

	virtual bool CommitPlacement(cPlayer & a_Player, const cItem & a_HeldItem, const Vector3i a_PlacePosition, const eBlockFace a_ClickedBlockFace, const Vector3i a_CursorPosition) const override
	{
		const auto Meta = cBlockDropSpenserHandler::DisplacementYawToMetaData(a_PlacePosition, a_Player.GetEyePosition(), a_Player.GetYaw());
		return a_Player.PlaceBlock(a_PlacePosition, static_cast<BLOCKTYPE>(a_HeldItem.m_ItemType), Meta);
	}
} ;
