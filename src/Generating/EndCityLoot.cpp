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
#include "../Item.h"
#include "../ItemGrid.h"

#include <random>





/** The enchant_with_levels range used by the End City treasure table's enchanted entries. */
static constexpr int END_CITY_LOOT_MIN_ENCHANT_LEVEL = 20;
static constexpr int END_CITY_LOOT_MAX_ENCHANT_LEVEL = 39;

/** The most enchantments one enchanting-table application at that level can add. */
static constexpr int END_CITY_LOOT_MAX_ENCHANTMENTS = 5;

/** Seeds the enchantment rolls apart from the table rolls. */
static constexpr int END_CITY_LOOT_ENCHANT_SEED = 0x5f3759df;





/** Applies the vanilla enchant_with_levels behaviour to the item.
The wiki states the enchanted End City entries are enchanted like one 20-39 level enchanting-table
application. This reuses the server's existing weight helper, so it approximates the exact
enchanting-table algorithm. */
static void EnchantEndCityItem(cItem & a_Item, std::minstd_rand & a_Rng)
{
	const int Level = END_CITY_LOOT_MIN_ENCHANT_LEVEL + static_cast<int>(a_Rng() % (END_CITY_LOOT_MAX_ENCHANT_LEVEL - END_CITY_LOOT_MIN_ENCHANT_LEVEL + 1));

	cWeightedEnchantments Enchantments;
	cEnchantments::AddItemEnchantmentWeights(Enchantments, a_Item.m_ItemType, static_cast<unsigned>(Level));

	const int NumEnchantments = 1 + static_cast<int>(a_Rng() % END_CITY_LOOT_MAX_ENCHANTMENTS);
	for (int i = 0; i < NumEnchantments; i++)
	{
		const cEnchantments Enchantment = cEnchantments::SelectEnchantmentFromVector(Enchantments, static_cast<int>(a_Rng()));
		a_Item.m_Enchantments.Add(Enchantment);
		cEnchantments::RemoveEnchantmentWeightFromVector(Enchantments, Enchantment);
		cEnchantments::CheckEnchantmentConflictsFromVector(Enchantments, Enchantment);
	}
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
	std::minstd_rand Rng(a_Seed ^ END_CITY_LOOT_ENCHANT_SEED);
	for (const auto & Roll: Rolls)
	{
		cItem Item(Roll.m_ItemType, Roll.m_Amount);
		if (Roll.m_Enchanted)
		{
			EnchantEndCityItem(Item, Rng);
		}

		// The wiki only says the slot arrangement is random and seed-driven, so use a random
		// still-empty slot:
		int Slot = static_cast<int>(Rng() % NumSlots);
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
