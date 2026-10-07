#include "Globals.h"
#include "BlockType.h"
#include "Generating/EndCityLootTable.h"
#include "../TestHelpers.h"

#include <vector>





/** Returns true if the item type is one of the End City treasure table's entries. */
static bool IsTableItem(short a_ItemType)
{
	switch (a_ItemType)
	{
		case E_ITEM_DIAMOND:
		case E_ITEM_IRON:
		case E_ITEM_GOLD:
		case E_ITEM_EMERALD:
		case E_ITEM_BEETROOT_SEEDS:
		case E_ITEM_SADDLE:
		case E_ITEM_IRON_HORSE_ARMOR:
		case E_ITEM_GOLD_HORSE_ARMOR:
		case E_ITEM_DIAMOND_HORSE_ARMOR:
		case E_ITEM_DIAMOND_SWORD:
		case E_ITEM_DIAMOND_BOOTS:
		case E_ITEM_DIAMOND_CHESTPLATE:
		case E_ITEM_DIAMOND_LEGGINGS:
		case E_ITEM_DIAMOND_HELMET:
		case E_ITEM_DIAMOND_PICKAXE:
		case E_ITEM_DIAMOND_SHOVEL:
		case E_ITEM_IRON_SWORD:
		case E_ITEM_IRON_BOOTS:
		case E_ITEM_IRON_CHESTPLATE:
		case E_ITEM_IRON_LEGGINGS:
		case E_ITEM_IRON_HELMET:
		case E_ITEM_IRON_PICKAXE:
		case E_ITEM_IRON_SHOVEL:
		{
			return true;
		}
	}
	return false;
}





/** The table always makes 2 to 6 rolls, each of a table item with a plausible amount. */
static void testRollCountAndItems(void)
{
	for (int seed = 0; seed < 1000; seed++)
	{
		std::vector<sEndCityLootRoll> Rolls;
		RollEndCityLoot(seed, Rolls);
		TEST_GREATER_THAN_OR_EQUAL(Rolls.size(), 2U);
		TEST_LESS_THAN_OR_EQUAL(Rolls.size(), 6U);
		for (const auto & Roll: Rolls)
		{
			TEST_TRUE(IsTableItem(Roll.m_ItemType));
			TEST_GREATER_THAN_OR_EQUAL(static_cast<int>(Roll.m_Amount), 1);
			TEST_LESS_THAN_OR_EQUAL(static_cast<int>(Roll.m_Amount), 10);
		}
	}
}





/** The same seed must always produce the same rolls. */
static void testDeterminism(void)
{
	std::vector<sEndCityLootRoll> First;
	std::vector<sEndCityLootRoll> Second;
	RollEndCityLoot(12345, First);
	RollEndCityLoot(12345, Second);
	TEST_EQUAL(First.size(), Second.size());
	for (size_t i = 0; i < First.size(); i++)
	{
		TEST_EQUAL(First[i].m_ItemType, Second[i].m_ItemType);
		TEST_EQUAL(First[i].m_Amount, Second[i].m_Amount);
		TEST_EQUAL(First[i].m_Enchanted, Second[i].m_Enchanted);
	}
}





/** The documented weights make gold more common than diamond, and diamond more common than emerald. */
static void testWeights(void)
{
	int Gold = 0;
	int Diamond = 0;
	int Emerald = 0;
	for (int seed = 0; seed < 5000; seed++)
	{
		std::vector<sEndCityLootRoll> Rolls;
		RollEndCityLoot(seed, Rolls);
		for (const auto & Roll: Rolls)
		{
			if (Roll.m_ItemType == E_ITEM_GOLD)
			{
				Gold++;
			}
			else if (Roll.m_ItemType == E_ITEM_DIAMOND)
			{
				Diamond++;
			}
			else if (Roll.m_ItemType == E_ITEM_EMERALD)
			{
				Emerald++;
			}
		}
	}

	// The documented weights are gold 15, diamond 5 and emerald 2:
	TEST_TRUE(Gold > Diamond);
	TEST_TRUE(Diamond > Emerald);
}





IMPLEMENT_TEST_MAIN("EndCityLootTest",
	testRollCountAndItems();
	testDeterminism();
	testWeights();
)
