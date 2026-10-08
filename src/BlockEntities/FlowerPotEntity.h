// FlowerPotEntity.h

// Declares the cFlowerPotEntity class representing a single sign in the world





#pragma once

#include "BlockEntity.h"
#include "../BlockType.h"
#include "../Item.h"





// tolua_begin
class cFlowerPotEntity :
	public cBlockEntity
{
	// tolua_end

	using Super = cBlockEntity;

public:  // tolua_export

	/** Creates a new flowerpot entity at the specified block coords. a_World may be nullptr */
	cFlowerPotEntity(BLOCKTYPE a_BlockType, NIBBLETYPE a_BlockMeta, Vector3i a_Pos, cWorld * a_World);


	// tolua_begin

	/** Is a flower in the pot? */
	bool IsItemInPot(void) { return !m_Item.IsEmpty(); }

	/** Get the item in the flower pot */
	cItem GetItem(void) const { return m_Item; }

	/** Set the item in the flower pot */
	void SetItem(const cItem & a_Item) { m_Item = a_Item; }

	// tolua_end

	// cBlockEntity overrides:
	virtual cItems ConvertToPickups() const override;
	virtual void CopyFrom(const cBlockEntity & a_Src) override;
	virtual bool UsedBy(cPlayer * a_Player) override;
	virtual void SendTo(cClientHandle & a_Client) override;

	/** Returns whether the specified item can be placed into a flower pot.
	Kept inline so that the data-file loaders can validate the contents without linking the
	world-bound implementation of this block entity. */
	static bool IsFlower(short m_ItemType, short m_ItemData)
	{
		switch (m_ItemType)
		{
			case E_BLOCK_DANDELION:
			case E_BLOCK_FLOWER:
			case E_BLOCK_CACTUS:
			case E_BLOCK_BROWN_MUSHROOM:
			case E_BLOCK_RED_MUSHROOM:
			case E_BLOCK_SAPLING:
			case E_BLOCK_DEAD_BUSH:
			{
				return true;
			}
			case E_BLOCK_TALL_GRASS:
			{
				return (m_ItemData == static_cast<short>(2));
			}
			default:
			{
				return false;
			}
		}
	}

private:

	cItem m_Item;
} ;  // tolua_export
