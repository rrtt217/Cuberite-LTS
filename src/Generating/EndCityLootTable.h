// EndCityLootTable.h

// Declares the pure End City treasure table roll logic

#pragma once

#include <vector>





/** One result of an End City treasure table roll. */
struct sEndCityLootRoll
{
	/** The item type that was picked. */
	short m_ItemType;

	/** How many of that item the roll produced. */
	char m_Amount;

	/** Whether the item must be enchanted with enchant_with_levels. */
	bool m_Enchanted;
} ;





/** Rolls the End City treasure table: 2 to 6 independent, weighted picks with replacement.
This is the pure part of the table, kept free of the item system so it can be unit tested. */
void RollEndCityLoot(int a_Seed, std::vector<sEndCityLootRoll> & a_Out);
