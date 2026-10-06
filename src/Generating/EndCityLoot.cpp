// EndCityLoot.cpp

// Implements the filling of End City chests from the treasure table

/*
The item list, the 2-6 roll count and the enchant_with_levels 20-39 range come from the Minecraft
Wiki: the 1.12-era loot chest module (end_city_treasure) and the 1.12.2-era Loot table article. The
full behaviour specification and its sources live in the structure chest loot specification.
*/

#include "Globals.h"
#include "EndCityLoot.h"
#include "EndCityLootTable.h"
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





/** Installs the filler when this module is linked into the server. */
static struct sEndCityLootRegistrar
{
	sEndCityLootRegistrar(void)
	{
		SetEndCityChestFiller(&FillEndCityChest);
	}
} g_EndCityLootRegistrar;
