// EndCityLoot.cpp

// Implements the End City treasure chest loot table and the ship's special contents

/*
The item list, the 2-6 roll count and the enchant_with_levels 20-39 range come from the Minecraft
Wiki: the 1.12-era loot chest module (end_city_treasure) and the 1.12.2-era Loot table article. The
ship's brewing stand (two Potions of Healing II) and its item frame with an elytra come from the
End City structure details. The full behaviour specification and its sources live in the structure
chest loot specification.
*/

#include "Globals.h"
#include "EndCityLoot.h"
#include "EndCityLootTable.h"
#include "ChunkDesc.h"
#include "../BlockEntities/ChestEntity.h"
#include "../BlockEntities/BrewingstandEntity.h"
#include "../Entities/ItemFrame.h"
#include "../Enchantments.h"
#include "../FastRandom.h"
#include "../Item.h"
#include "../ItemGrid.h"

#include <random>





/** The enchant_with_levels range used by the End City treasure table's enchanted entries. */
static constexpr int END_CITY_LOOT_MIN_ENCHANT_LEVEL = 20;
static constexpr int END_CITY_LOOT_MAX_ENCHANT_LEVEL = 39;

/** Seeds the enchantment rolls apart from the table rolls. */
static constexpr int END_CITY_LOOT_ENCHANT_SEED = 0x5f3759df;

/** The potion damage bits for Instant Health and for level II, per cEntityEffect. */
static constexpr short END_CITY_POTION_INSTANT_HEALTH = 0x05;
static constexpr short END_CITY_POTION_LEVEL_II = 0x20;





/** Returns a deterministic loot seed for the chest at the specified world coordinates. */
static int MakeEndCityLootSeed(const Vector3i & a_Pos)
{
	return (a_Pos.x * 73856093) ^ (a_Pos.y * 19349663) ^ (a_Pos.z * 83492791);
}





/** Applies the vanilla enchant_with_levels behaviour to the item.
The wiki states the enchanted End City entries are enchanted like one 20-39 level enchanting-table
application, so pick a level in that range and run the server's enchanting-table algorithm, which
already models the enchantability roll and the extra-enchantment chances. */
static void EnchantEndCityItem(cItem & a_Item, MTRand & a_Random)
{
	const int Level = END_CITY_LOOT_MIN_ENCHANT_LEVEL + a_Random.RandInt(END_CITY_LOOT_MAX_ENCHANT_LEVEL - END_CITY_LOOT_MIN_ENCHANT_LEVEL);
	a_Item.EnchantByXPLevels(static_cast<unsigned>(Level), a_Random);
}





/** Fills the chest contents with the rolled table entries, each into a random still-empty slot. */
static void FillEndCityChest(cItemGrid & a_Contents, int a_Seed)
{
	std::vector<sEndCityLootRoll> Rolls;
	RollEndCityLoot(a_Seed, Rolls);

	const int NumSlots = a_Contents.GetNumSlots();
	if (NumSlots <= 0)
	{
		return;
	}

	// The enchantment choices roll separately from the table picks:
	MTRand Rng;
	Rng.Engine().seed(static_cast<unsigned>(a_Seed) ^ END_CITY_LOOT_ENCHANT_SEED);
	for (const auto & Roll: Rolls)
	{
		cItem Item(Roll.m_ItemType, Roll.m_Amount);
		if (Roll.m_Enchanted)
		{
			EnchantEndCityItem(Item, Rng);
		}

		// The wiki only says the slot arrangement is random and seed-driven, so use a random
		// still-empty slot:
		int Slot = Rng.RandInt(NumSlots - 1);
		for (int Tries = 0; (Tries < NumSlots) && !a_Contents.IsSlotEmpty(Slot); Tries++)
		{
			Slot = (Slot + 1) % NumSlots;
		}
		if (a_Contents.IsSlotEmpty(Slot))
		{
			a_Contents.SetSlot(Slot, Item);
		}
	}
}





/** Fills the ship's brewing stand with the two Potions of Healing II documented for it. */
static void FillEndCityBrewingStand(cBrewingstandEntity & a_Stand)
{
	const cItem Potion(E_ITEM_POTION, 1, END_CITY_POTION_INSTANT_HEALTH | END_CITY_POTION_LEVEL_II);
	a_Stand.SetLeftBottleSlot(Potion);
	a_Stand.SetMiddleBottleSlot(Potion);
}





/** Fills the End City special contents that landed in this chunk. */
static void FillEndCityContents(cChunkDesc & a_Chunk, const std::vector<sEndCityContent> & a_Contents)
{
	const int ChunkMinX = a_Chunk.GetChunkX() * cChunkDef::Width;
	const int ChunkMinZ = a_Chunk.GetChunkZ() * cChunkDef::Width;
	for (const auto & Content: a_Contents)
	{
		const int RelX = Content.m_Pos.x - ChunkMinX;
		const int RelZ = Content.m_Pos.z - ChunkMinZ;
		switch (Content.m_Kind)
		{
			case ecctChest:
			{
				auto * Chest = static_cast<cChestEntity *>(a_Chunk.GetBlockEntity(RelX, Content.m_Pos.y, RelZ));
				if (Chest != nullptr)
				{
					FillEndCityChest(Chest->GetContents(), MakeEndCityLootSeed(Content.m_Pos));
				}
				break;
			}

			case ecctBrewingStand:
			{
				auto * Stand = static_cast<cBrewingstandEntity *>(a_Chunk.GetBlockEntity(RelX, Content.m_Pos.y, RelZ));
				if (Stand != nullptr)
				{
					FillEndCityBrewingStand(*Stand);
				}
				break;
			}

			case ecctItemFrame:
			{
				auto Frame = std::make_unique<cItemFrame>(Content.m_Face, Vector3d(0.5, 0.5, 0.5) + Content.m_Pos);
				cItem Elytra(E_ITEM_ELYTRA);
				Frame->SetItem(Elytra);
				a_Chunk.GetEntities().emplace_back(std::move(Frame));
				break;
			}
		}
	}
}





/** Installs the filler when this module is linked into the server. */
static struct sEndCityLootRegistrar
{
	sEndCityLootRegistrar(void)
	{
		SetEndCityContentsFiller(&FillEndCityContents);
	}
} g_EndCityLootRegistrar;
