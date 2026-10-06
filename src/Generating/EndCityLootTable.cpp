// EndCityLootTable.cpp

// Implements the pure End City treasure table roll logic

/*
The item list, the 2-6 roll count and the enchant_with_levels 20-39 range come from the Minecraft
Wiki: the 1.12-era loot chest module (end_city_treasure) and the 1.12.2-era Loot table article. The
full behaviour specification and its sources live in the structure chest loot specification.
*/

#include "Globals.h"
#include "EndCityLootTable.h"
#include "../BlockType.h"

#include <random>





/** The End City treasure table's roll count: a base of 2 plus up to 4 more. */
static constexpr int END_CITY_LOOT_MIN_ROLLS = 2;
static constexpr int END_CITY_LOOT_EXTRA_ROLLS = 5;

/** One entry of the vanilla End City treasure table. */
struct sEndCityLootEntry
{
	/** The item type. */
	short m_ItemType;

	/** The minimum and maximum stack size of one roll. */
	int m_MinAmount;
	int m_MaxAmount;

	/** The relative weight of this entry within the pool. */
	int m_Weight;

	/** Whether the item is enchanted with enchant_with_levels. */
	bool m_Enchanted;
} ;

/** The End City treasure table, as documented in the structure chest loot specification.
The pool has a total weight of 85. */
static const sEndCityLootEntry g_EndCityLoot[] =
{
	// Item,                     Min, Max, Weight, Enchanted
	{E_ITEM_DIAMOND,               2,   7,      5, false},
	{E_ITEM_IRON,                  4,   8,     10, false},
	{E_ITEM_GOLD,                  2,   7,     15, false},
	{E_ITEM_EMERALD,               2,   6,      2, false},
	{E_ITEM_BEETROOT_SEEDS,        1,  10,      5, false},
	{E_ITEM_SADDLE,                1,   1,      3, false},
	{E_ITEM_IRON_HORSE_ARMOR,      1,   1,      1, false},
	{E_ITEM_GOLD_HORSE_ARMOR,      1,   1,      1, false},
	{E_ITEM_DIAMOND_HORSE_ARMOR,   1,   1,      1, false},
	{E_ITEM_DIAMOND_SWORD,         1,   1,      3, true},
	{E_ITEM_DIAMOND_BOOTS,         1,   1,      3, true},
	{E_ITEM_DIAMOND_CHESTPLATE,    1,   1,      3, true},
	{E_ITEM_DIAMOND_LEGGINGS,      1,   1,      3, true},
	{E_ITEM_DIAMOND_HELMET,        1,   1,      3, true},
	{E_ITEM_DIAMOND_PICKAXE,       1,   1,      3, true},
	{E_ITEM_DIAMOND_SHOVEL,        1,   1,      3, true},
	{E_ITEM_IRON_SWORD,            1,   1,      3, true},
	{E_ITEM_IRON_BOOTS,            1,   1,      3, true},
	{E_ITEM_IRON_CHESTPLATE,       1,   1,      3, true},
	{E_ITEM_IRON_LEGGINGS,         1,   1,      3, true},
	{E_ITEM_IRON_HELMET,           1,   1,      3, true},
	{E_ITEM_IRON_PICKAXE,          1,   1,      3, true},
	{E_ITEM_IRON_SHOVEL,           1,   1,      3, true},
} ;





void RollEndCityLoot(int a_Seed, std::vector<sEndCityLootRoll> & a_Out)
{
	a_Out.clear();
	std::minstd_rand Rng(a_Seed);

	int TotalWeight = 0;
	for (const auto & Entry: g_EndCityLoot)
	{
		TotalWeight += Entry.m_Weight;
	}

	const int Rolls = END_CITY_LOOT_MIN_ROLLS + static_cast<int>(Rng() % END_CITY_LOOT_EXTRA_ROLLS);
	for (int i = 0; i < Rolls; i++)
	{
		int Pick = static_cast<int>(Rng() % TotalWeight);
		const sEndCityLootEntry * Entry = nullptr;
		for (const auto & Candidate: g_EndCityLoot)
		{
			Pick -= Candidate.m_Weight;
			if (Pick < 0)
			{
				Entry = &Candidate;
				break;
			}
		}
		if (Entry == nullptr)
		{
			continue;
		}

		const int Range = (Entry->m_MaxAmount > Entry->m_MinAmount) ? (Entry->m_MaxAmount - Entry->m_MinAmount + 1) : 1;
		const int Amount = Entry->m_MinAmount + static_cast<int>(Rng() % Range);
		a_Out.push_back({Entry->m_ItemType, static_cast<char>(Amount), Entry->m_Enchanted});
	}
}
