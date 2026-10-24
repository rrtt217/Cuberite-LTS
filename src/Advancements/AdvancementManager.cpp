// AdvancementManager.cpp

#include "Globals.h"
#include "AdvancementManager.h"

#include "../BiomeDef.h"
#include "../BlockType.h"
#include "../CraftingRecipes.h"
#include "../Root.h"
#include "../Entities/EntityEffect.h"
#include "../Entities/Player.h"
#include "../Inventory.h"
#include "../JsonUtils.h"
#include "../Mobs/Monster.h"
#include "../WorldStorage/NamespaceSerializer.h"
#include "OSSupport/File.h"

#include <json/json.h>

#include <algorithm>
#include <set>





namespace
{

	bool EndsWith(const AString & a_String, const AString & a_Suffix)
	{
		return (a_String.size() >= a_Suffix.size()) &&
			(a_String.compare(a_String.size() - a_Suffix.size(), a_Suffix.size(), a_Suffix) == 0);
	}





	void CollectJsonFiles(const AString & a_Folder, const AString & a_Relative, std::vector<std::pair<AString, AString>> & a_Files)
	{
		for (const auto & Name : cFile::GetFolderContents(a_Folder))
		{
			const AString Full = a_Folder + cFile::GetPathSeparator() + Name;
			const AString Relative = a_Relative.empty() ? Name : (a_Relative + "/" + Name);
			if (cFile::IsFolder(Full))
			{
				CollectJsonFiles(Full, Relative, a_Files);
			}
			else if (EndsWith(Name, ".json"))
			{
				a_Files.emplace_back(Full, Relative);
			}
		}
	}





	/** Converts a JSON value into a raw chat component string. */
	AString ReadChatComponent(const Json::Value & a_Value)
	{
		if (a_Value.isString())
		{
			Json::Value Object(Json::objectValue);
			Object["text"] = a_Value.asString();
			return JsonUtils::WriteFastString(Object);
		}
		if (a_Value.isObject() || a_Value.isArray())
		{
			return JsonUtils::WriteFastString(a_Value);
		}
		return "{\"text\":\"\"}";
	}





	/** Parses a raw chat component string back into a JSON value. */
	Json::Value ParseChatComponent(const AString & a_Component)
	{
		Json::Value Value;
		AString Error;
		if (JsonUtils::ParseString(a_Component, Value, &Error) && Value.isObject())
		{
			return Value;
		}
		Value = Json::Value(Json::objectValue);
		Value["text"] = a_Component;
		return Value;
	}





	cItem ReadIcon(const Json::Value & a_Value)
	{
		if (!a_Value.isObject())
		{
			return cItem();
		}
		const short Damage = static_cast<short>(a_Value.isMember("data") ? a_Value["data"].asInt() : 0);
		if (!a_Value.isMember("item"))
		{
			return cItem();
		}
		const auto & Item = a_Value["item"];
		if (Item.isString())
		{
			cItem Result;
			if (StringToItem(Item.asString(), Result))
			{
				Result.m_ItemDamage = Damage;
				return Result;
			}
			return cItem();
		}
		if (Item.isInt())
		{
			return cItem(static_cast<short>(Item.asInt()), 1, Damage);
		}
		return cItem();
	}





	eAdvancementFrame ReadFrame(const AString & a_Frame)
	{
		if (a_Frame == "challenge")
		{
			return eAdvancementFrame::Challenge;
		}
		if (a_Frame == "goal")
		{
			return eAdvancementFrame::Goal;
		}
		return eAdvancementFrame::Task;
	}





	const char * FrameToKey(const eAdvancementFrame a_Frame)
	{
		switch (a_Frame)
		{
			case eAdvancementFrame::Task:      return "task";
			case eAdvancementFrame::Challenge: return "challenge";
			case eAdvancementFrame::Goal:      return "goal";
		}
		return "task";
	}

	bool MatchRange(const Json::Value & a_Value, const int a_Actual)
	{
		if (a_Value.isInt())
		{
			return (a_Actual == a_Value.asInt());
		}
		if (a_Value.isObject())
		{
			if (a_Value.isMember("min") && (a_Actual < a_Value["min"].asInt()))
			{
				return false;
			}
			if (a_Value.isMember("max") && (a_Actual > a_Value["max"].asInt()))
			{
				return false;
			}
		}
		return true;
	}





	bool MatchItemPredicate(const Json::Value & a_Predicate, const cItem & a_Item)
	{
		if (a_Predicate.isMember("item"))
		{
			const auto & Item = a_Predicate["item"];
			if (Item.isString())
			{
				cItem Required;
				if (!StringToItem(Item.asString(), Required) || (Required.m_ItemType != a_Item.m_ItemType))
				{
					return false;
				}
			}
			else if (Item.isInt() && (Item.asInt() != a_Item.m_ItemType))
			{
				return false;
			}
		}
		if (a_Predicate.isMember("data") && (a_Predicate["data"].asInt() != a_Item.m_ItemDamage))
		{
			return false;
		}
		if (a_Predicate.isMember("count") && !MatchRange(a_Predicate["count"], a_Item.m_ItemCount))
		{
			return false;
		}
		if (a_Predicate.isMember("durability"))
		{
			if (!a_Item.IsDamageable())
			{
				return false;
			}
			const int Durability = static_cast<int>(a_Item.GetMaxDamage()) - static_cast<int>(a_Item.m_ItemDamage);
			if (!MatchRange(a_Predicate["durability"], Durability))
			{
				return false;
			}
		}
		return true;
	}





	bool InventoryContains(const cInventory & a_Inventory, const Json::Value & a_Items)
	{
		for (const auto & Predicate : a_Items)
		{
			bool Found = false;
			for (int Slot = 0; Slot < cInventory::invNumSlots; ++Slot)
			{
				const auto & Item = a_Inventory.GetSlot(Slot);
				if (!Item.IsEmpty() && MatchItemPredicate(Predicate, Item))
				{
					Found = true;
					break;
				}
			}
			if (!Found)
			{
				return false;
			}
		}
		return true;
	}





	bool MatchSlotCounts(const Json::Value & a_Slots, const cInventory & a_Inventory)
	{
		int Empty = 0;
		int Full = 0;
		int Occupied = 0;
		for (int Slot = 0; Slot < cInventory::invNumSlots; ++Slot)
		{
			const auto & Item = a_Inventory.GetSlot(Slot);
			if (Item.IsEmpty())
			{
				++Empty;
				continue;
			}
			++Occupied;
			if (Item.m_ItemCount >= Item.GetMaxStackSize())
			{
				++Full;
			}
		}
		if (a_Slots.isMember("empty") && !MatchRange(a_Slots["empty"], Empty))
		{
			return false;
		}
		if (a_Slots.isMember("full") && !MatchRange(a_Slots["full"], Full))
		{
			return false;
		}
		if (a_Slots.isMember("occupied") && !MatchRange(a_Slots["occupied"], Occupied))
		{
			return false;
		}
		return true;
	}





	bool IsImplementedTrigger(const AString & a_Trigger)
	{
		return (a_Trigger == "minecraft:inventory_changed") ||
			(a_Trigger == "minecraft:player_killed_entity") ||
			(a_Trigger == "minecraft:entity_killed_player") ||
			(a_Trigger == "minecraft:player_hurt_entity") ||
			(a_Trigger == "minecraft:entity_hurt_player") ||
			(a_Trigger == "minecraft:item_durability_changed") ||
			(a_Trigger == "minecraft:consume_item") ||
			(a_Trigger == "minecraft:placed_block") ||
			(a_Trigger == "minecraft:used_ender_eye") ||
			(a_Trigger == "minecraft:used_totem") ||
			(a_Trigger == "minecraft:recipe_unlocked") ||
			(a_Trigger == "minecraft:effects_changed") ||
			(a_Trigger == "minecraft:changed_dimension") ||
			(a_Trigger == "minecraft:tick") ||
			(a_Trigger == "minecraft:enter_block") ||
			(a_Trigger == "minecraft:enchanted_item") ||
			(a_Trigger == "minecraft:slept_in_bed") ||
			(a_Trigger == "minecraft:summoned_entity") ||
			(a_Trigger == "minecraft:tame_animal") ||
			(a_Trigger == "minecraft:bred_animals") ||
			(a_Trigger == "minecraft:cured_zombie_villager") ||
			(a_Trigger == "minecraft:brewed_potion") ||
			(a_Trigger == "minecraft:construct_beacon") ||
			(a_Trigger == "minecraft:location") ||
			(a_Trigger == "minecraft:nether_travel");
	}





	cEntityEffect::eType EffectNameToType(const AString & a_Name)
	{
		AString Name = a_Name;
		if (Name.compare(0, 10, "minecraft:") == 0)
		{
			Name.erase(0, 10);
		}
		static const struct
		{
			const char * m_Name;
			cEntityEffect::eType m_Type;
		} Effects[] =
		{
			{"speed", cEntityEffect::effSpeed},
			{"slowness", cEntityEffect::effSlowness},
			{"haste", cEntityEffect::effHaste},
			{"mining_fatigue", cEntityEffect::effMiningFatigue},
			{"strength", cEntityEffect::effStrength},
			{"instant_health", cEntityEffect::effInstantHealth},
			{"instant_damage", cEntityEffect::effInstantDamage},
			{"jump_boost", cEntityEffect::effJumpBoost},
			{"nausea", cEntityEffect::effNausea},
			{"regeneration", cEntityEffect::effRegeneration},
			{"resistance", cEntityEffect::effResistance},
			{"fire_resistance", cEntityEffect::effFireResistance},
			{"water_breathing", cEntityEffect::effWaterBreathing},
			{"invisibility", cEntityEffect::effInvisibility},
			{"blindness", cEntityEffect::effBlindness},
			{"night_vision", cEntityEffect::effNightVision},
			{"hunger", cEntityEffect::effHunger},
			{"weakness", cEntityEffect::effWeakness},
			{"poison", cEntityEffect::effPoison},
			{"wither", cEntityEffect::effWither},
			{"health_boost", cEntityEffect::effHealthBoost},
			{"absorption", cEntityEffect::effAbsorption},
			{"saturation", cEntityEffect::effSaturation},
		};
		for (const auto & Effect : Effects)
		{
			if (Name == Effect.m_Name)
			{
				return Effect.m_Type;
			}
		}
		return cEntityEffect::effNoEffect;
	}





	bool MatchEffectPredicate(const Json::Value & a_Predicate, const cEntityEffect & a_Effect)
	{
		if (!a_Predicate.isObject())
		{
			return true;
		}
		if (a_Predicate.isMember("amplifier") && !MatchRange(a_Predicate["amplifier"], a_Effect.GetIntensity()))
		{
			return false;
		}
		if (a_Predicate.isMember("duration") && !MatchRange(a_Predicate["duration"], a_Effect.GetDuration()))
		{
			return false;
		}
		return true;
	}





	bool MatchEffectsConditions(const Json::Value & a_Conditions, const cPlayer & a_Player)
	{
		if (!a_Conditions.isObject() || !a_Conditions.isMember("effects") || !a_Conditions["effects"].isObject())
		{
			return true;
		}
		for (auto it = a_Conditions["effects"].begin(); it != a_Conditions["effects"].end(); ++it)
		{
			const auto Type = EffectNameToType(it.key().asString());
			if (Type == cEntityEffect::effNoEffect)
			{
				return false;
			}
			const auto * Effect = a_Player.GetEntityEffect(Type);
			if ((Effect == nullptr) || !MatchEffectPredicate(*it, *Effect))
			{
				return false;
			}
		}
		return true;
	}





	bool MatchEffectsConditionsSupport(const Json::Value & a_Conditions, AString & a_Reason)
	{
		if (!a_Conditions.isObject() || (a_Conditions.size() == 0))
		{
			return true;
		}
		for (auto it = a_Conditions.begin(); it != a_Conditions.end(); ++it)
		{
			if (it.key().asString() != "effects")
			{
				a_Reason = "unsupported condition \"" + it.key().asString() + "\"";
				return false;
			}
		}
		if (!a_Conditions["effects"].isObject())
		{
			a_Reason = "effects must be an object";
			return false;
		}
		for (auto it = a_Conditions["effects"].begin(); it != a_Conditions["effects"].end(); ++it)
		{
			if (EffectNameToType(it.key().asString()) == cEntityEffect::effNoEffect)
			{
				a_Reason = "unsupported effect \"" + it.key().asString() + "\"";
				return false;
			}
			if (!it->isObject())
			{
				a_Reason = "effect predicate must be an object";
				return false;
			}
			for (auto predIt = it->begin(); predIt != it->end(); ++predIt)
			{
				const AString Key = predIt.key().asString();
				if ((Key != "amplifier") && (Key != "duration"))
				{
					a_Reason = "unsupported effect predicate \"" + Key + "\"";
					return false;
				}
			}
		}
		return true;
	}





	bool AreTriggerConditionsSupported(const AString & a_Trigger, const Json::Value & a_Conditions, AString & a_Reason)
	{
		if (a_Trigger == "minecraft:inventory_changed")
		{
			// The M3 matcher supports a subset and ignores the rest; keep loading:
			return true;
		}
		if ((a_Trigger == "minecraft:player_killed_entity") || (a_Trigger == "minecraft:entity_killed_player") || (a_Trigger == "minecraft:player_hurt_entity"))
		{
			if (!a_Conditions.isObject() || (a_Conditions.size() == 0))
			{
				return true;
			}
			for (auto it = a_Conditions.begin(); it != a_Conditions.end(); ++it)
			{
				const AString Key = it.key().asString();
				if ((Key != "entity") && (Key != "player"))
				{
					a_Reason = "unsupported condition \"" + Key + "\"";
					return false;
				}
				if (!it->isObject())
				{
					a_Reason = "unsupported predicate \"" + Key + "\"";
					return false;
				}
				for (auto PredIt = it->begin(); PredIt != it->end(); ++PredIt)
				{
					const AString PredKey = PredIt.key().asString();
					if ((PredKey != "type") && (PredKey != "player") && (PredKey != "location"))
					{
						a_Reason = "unsupported entity predicate \"" + PredKey + "\"";
						return false;
					}
					if ((PredKey == "location") && PredIt->isObject())
					{
						for (auto LocIt = PredIt->begin(); LocIt != PredIt->end(); ++LocIt)
						{
							if (LocIt.key().asString() != "dimension")
							{
								a_Reason = "unsupported location predicate \"" + LocIt.key().asString() + "\"";
								return false;
							}
						}
					}
				}
			}
			return true;
		}
		if (a_Trigger == "minecraft:nether_travel")
		{
			if (!a_Conditions.isObject() || (a_Conditions.size() == 0))
			{
				return true;
			}
			for (auto it = a_Conditions.begin(); it != a_Conditions.end(); ++it)
			{
				if (it.key().asString() != "distance")
				{
					a_Reason = "unsupported condition \"" + it.key().asString() + "\"";
					return false;
				}
			}
			if (a_Conditions.isMember("distance") && a_Conditions["distance"].isObject())
			{
				for (auto it = a_Conditions["distance"].begin(); it != a_Conditions["distance"].end(); ++it)
				{
					const AString Key = it.key().asString();
					if ((Key != "absolute") && (Key != "horizontal") && (Key != "x") && (Key != "y") && (Key != "z"))
					{
						a_Reason = "unsupported distance condition \"" + Key + "\"";
						return false;
					}
				}
			}
			return true;
		}
		if (a_Trigger == "minecraft:location")
		{
			if (!a_Conditions.isObject() || (a_Conditions.size() == 0))
			{
				return true;
			}
			for (auto it = a_Conditions.begin(); it != a_Conditions.end(); ++it)
			{
				const AString Key = it.key().asString();
				if ((Key != "dimension") && (Key != "position") && (Key != "biome") && (Key != "feature"))
				{
					a_Reason = "unsupported location condition \"" + Key + "\"";
					return false;
				}
			}
			if (a_Conditions.isMember("position") && a_Conditions["position"].isObject())
			{
				for (auto it = a_Conditions["position"].begin(); it != a_Conditions["position"].end(); ++it)
				{
					const AString Key = it.key().asString();
					if ((Key != "x") && (Key != "y") && (Key != "z"))
					{
						a_Reason = "unsupported position condition \"" + Key + "\"";
						return false;
					}
				}
			}
			return true;
		}
		if (a_Trigger == "minecraft:construct_beacon")
		{
			if (!a_Conditions.isObject() || (a_Conditions.size() == 0))
			{
				return true;
			}
			for (auto it = a_Conditions.begin(); it != a_Conditions.end(); ++it)
			{
				if (it.key().asString() != "level")
				{
					a_Reason = "unsupported condition \"" + it.key().asString() + "\"";
					return false;
				}
			}
			return true;
		}
		if (a_Trigger == "minecraft:brewed_potion")
		{
			if (!a_Conditions.isObject() || (a_Conditions.size() == 0))
			{
				return true;
			}
			a_Reason = "potion condition not implemented";
			return false;
		}
		if (a_Trigger == "minecraft:cured_zombie_villager")
		{
			if (!a_Conditions.isObject() || (a_Conditions.size() == 0))
			{
				return true;
			}
			for (auto it = a_Conditions.begin(); it != a_Conditions.end(); ++it)
			{
				const AString Key = it.key().asString();
				if ((Key != "villager") && (Key != "zombie"))
				{
					a_Reason = "unsupported condition \"" + Key + "\"";
					return false;
				}
			}
			return true;
		}
		if (a_Trigger == "minecraft:bred_animals")
		{
			if (!a_Conditions.isObject() || (a_Conditions.size() == 0))
			{
				return true;
			}
			for (auto it = a_Conditions.begin(); it != a_Conditions.end(); ++it)
			{
				const AString Key = it.key().asString();
				if ((Key != "parent") && (Key != "partner") && (Key != "child"))
				{
					a_Reason = "unsupported condition \"" + Key + "\"";
					return false;
				}
			}
			return true;
		}
		if (a_Trigger == "minecraft:tame_animal")
		{
			if (!a_Conditions.isObject() || (a_Conditions.size() == 0))
			{
				return true;
			}
			for (auto it = a_Conditions.begin(); it != a_Conditions.end(); ++it)
			{
				if (it.key().asString() != "entity")
				{
					a_Reason = "unsupported condition \"" + it.key().asString() + "\"";
					return false;
				}
			}
			return true;
		}
		if (a_Trigger == "minecraft:summoned_entity")
		{
			if (!a_Conditions.isObject() || (a_Conditions.size() == 0))
			{
				return true;
			}
			for (auto it = a_Conditions.begin(); it != a_Conditions.end(); ++it)
			{
				if (it.key().asString() != "entity")
				{
					a_Reason = "unsupported condition \"" + it.key().asString() + "\"";
					return false;
				}
			}
			return true;
		}
		if (a_Trigger == "minecraft:slept_in_bed")
		{
			if (!a_Conditions.isObject() || (a_Conditions.size() == 0))
			{
				return true;
			}
			a_Reason = "slept_in_bed conditions not implemented";
			return false;
		}
		if (a_Trigger == "minecraft:enchanted_item")
		{
			if (!a_Conditions.isObject() || (a_Conditions.size() == 0))
			{
				return true;
			}
			for (auto it = a_Conditions.begin(); it != a_Conditions.end(); ++it)
			{
				const AString Key = it.key().asString();
				if ((Key != "item") && (Key != "levels"))
				{
					a_Reason = "unsupported condition \"" + Key + "\"";
					return false;
				}
			}
			return true;
		}
		if (a_Trigger == "minecraft:enter_block")
		{
			if (!a_Conditions.isObject() || (a_Conditions.size() == 0))
			{
				return true;
			}
			for (auto it = a_Conditions.begin(); it != a_Conditions.end(); ++it)
			{
				if (it.key().asString() != "block")
				{
					a_Reason = "unsupported condition \"" + it.key().asString() + "\"";
					return false;
				}
			}
			return true;
		}
		if (a_Trigger == "minecraft:tick")
		{
			if (!a_Conditions.isObject() || (a_Conditions.size() == 0))
			{
				return true;
			}
			a_Reason = "tick conditions not implemented";
			return false;
		}
		if (a_Trigger == "minecraft:changed_dimension")
		{
			if (!a_Conditions.isObject() || (a_Conditions.size() == 0))
			{
				return true;
			}
			for (auto it = a_Conditions.begin(); it != a_Conditions.end(); ++it)
			{
				const AString Key = it.key().asString();
				if ((Key != "from") && (Key != "to"))
				{
					a_Reason = "unsupported condition \"" + Key + "\"";
					return false;
				}
			}
			return true;
		}
		if (a_Trigger == "minecraft:effects_changed")
		{
			return MatchEffectsConditionsSupport(a_Conditions, a_Reason);
		}
		if (a_Trigger == "minecraft:recipe_unlocked")
		{
			if (!a_Conditions.isObject() || (a_Conditions.size() == 0))
			{
				return true;
			}
			for (auto it = a_Conditions.begin(); it != a_Conditions.end(); ++it)
			{
				if (it.key().asString() != "recipe")
				{
					a_Reason = "unsupported condition \"" + it.key().asString() + "\"";
					return false;
				}
			}
			return true;
		}
		if (a_Trigger == "minecraft:used_totem")
		{
			if (!a_Conditions.isObject() || (a_Conditions.size() == 0))
			{
				return true;
			}
			for (auto it = a_Conditions.begin(); it != a_Conditions.end(); ++it)
			{
				if (it.key().asString() != "item")
				{
					a_Reason = "unsupported condition \"" + it.key().asString() + "\"";
					return false;
				}
			}
			return true;
		}
		if (a_Trigger == "minecraft:used_ender_eye")
		{
			if (!a_Conditions.isObject() || (a_Conditions.size() == 0))
			{
				return true;
			}
			for (auto it = a_Conditions.begin(); it != a_Conditions.end(); ++it)
			{
				if (it.key().asString() != "distance")
				{
					a_Reason = "unsupported condition \"" + it.key().asString() + "\"";
					return false;
				}
			}
			return true;
		}
		if (a_Trigger == "minecraft:placed_block")
		{
			if (!a_Conditions.isObject() || (a_Conditions.size() == 0))
			{
				return true;
			}
			for (auto it = a_Conditions.begin(); it != a_Conditions.end(); ++it)
			{
				const AString Key = it.key().asString();
				if ((Key != "block") && (Key != "item"))
				{
					a_Reason = "unsupported condition \"" + Key + "\"";
					return false;
				}
			}
			return true;
		}
		if (a_Trigger == "minecraft:consume_item")
		{
			if (!a_Conditions.isObject() || (a_Conditions.size() == 0))
			{
				return true;
			}
			for (auto it = a_Conditions.begin(); it != a_Conditions.end(); ++it)
			{
				if (it.key().asString() != "item")
				{
					a_Reason = "unsupported condition \"" + it.key().asString() + "\"";
					return false;
				}
			}
			return true;
		}
		if (a_Trigger == "minecraft:item_durability_changed")
		{
			if (!a_Conditions.isObject() || (a_Conditions.size() == 0))
			{
				return true;
			}
			for (auto it = a_Conditions.begin(); it != a_Conditions.end(); ++it)
			{
				const AString Key = it.key().asString();
				if ((Key != "item") && (Key != "delta") && (Key != "durability"))
				{
					a_Reason = "unsupported condition \"" + Key + "\"";
					return false;
				}
			}
			return true;
		}
		if (a_Trigger == "minecraft:entity_hurt_player")
		{
			if (!a_Conditions.isObject() || (a_Conditions.size() == 0))
			{
				return true;
			}
			for (auto it = a_Conditions.begin(); it != a_Conditions.end(); ++it)
			{
				const AString Key = it.key().asString();
				if (Key != "damage")
				{
					a_Reason = "unsupported condition \"" + Key + "\"";
					return false;
				}
			}
			if (a_Conditions.isMember("damage") && a_Conditions["damage"].isObject())
			{
				for (auto it = a_Conditions["damage"].begin(); it != a_Conditions["damage"].end(); ++it)
				{
					const AString Key = it.key().asString();
					if (Key != "source_entity")
					{
						a_Reason = "unsupported damage predicate \"" + Key + "\"";
						return false;
					}
				}
				if (a_Conditions["damage"].isMember("source_entity") && a_Conditions["damage"]["source_entity"].isObject())
				{
					for (auto it = a_Conditions["damage"]["source_entity"].begin(); it != a_Conditions["damage"]["source_entity"].end(); ++it)
					{
						const AString Key = it.key().asString();
						if ((Key != "type") && (Key != "player"))
						{
							a_Reason = "unsupported source_entity predicate \"" + Key + "\"";
							return false;
						}
					}
				}
			}
			return true;
		}
		a_Reason = "trigger not implemented";
		return false;
	}





	AString EntityTypeName(const cEntity & a_Entity)
	{
		if (a_Entity.IsPlayer())
		{
			return "minecraft:player";
		}
		if (a_Entity.IsMob())
		{
			const auto & Monster = static_cast<const cMonster &>(a_Entity);
			const auto Name = NamespaceSerializer::From(Monster.GetMobType());
			return AString("minecraft:") + AString(Name.data(), Name.size());
		}
		return AString();
	}





	bool MatchEntityPredicate(const Json::Value & a_Conditions, const cEntity & a_Entity);





	AString NormalizeBlockName(const AString & a_Name)
	{
		AString Name = a_Name;
		if (Name.compare(0, 10, "minecraft:") == 0)
		{
			Name.erase(0, 10);
		}
		return Name;
	}





	bool MatchBlockName(const Json::Value & a_Value, BLOCKTYPE a_BlockType)
	{
		if (!a_Value.isString())
		{
			return false;
		}
		const AString Expected = NormalizeBlockName(a_Value.asString());
		const AString Actual = NormalizeBlockName(ItemToString(cItem(a_BlockType)));
		return (Expected == Actual);
	}





	bool MatchDoubleRange(const Json::Value & a_Value, double a_Actual)
	{
		if (a_Value.isObject())
		{
			if (a_Value.isMember("min") && (a_Actual < a_Value["min"].asDouble()))
			{
				return false;
			}
			if (a_Value.isMember("max") && (a_Actual > a_Value["max"].asDouble()))
			{
				return false;
			}
			return true;
		}
		if (a_Value.isDouble() || a_Value.isInt())
		{
			return (std::abs(a_Actual - a_Value.asDouble()) < 1e-9);
		}
		return true;
	}





	bool MatchDurabilityChanged(const Json::Value & a_Conditions, const cItem & a_ItemBefore, int a_Delta, int a_Durability)
	{
		if (!a_Conditions.isObject())
		{
			return true;
		}
		if (a_Conditions.isMember("item") && !MatchItemPredicate(a_Conditions["item"], a_ItemBefore))
		{
			return false;
		}
		if (a_Conditions.isMember("delta") && !MatchRange(a_Conditions["delta"], a_Delta))
		{
			return false;
		}
		if (a_Conditions.isMember("durability") && !MatchRange(a_Conditions["durability"], a_Durability))
		{
			return false;
		}
		return true;
	}





	bool MatchDamageSource(const Json::Value & a_Conditions, const cEntity * a_Attacker)
	{
		if (!a_Conditions.isObject() || !a_Conditions.isMember("damage"))
		{
			return true;
		}
		const auto & Damage = a_Conditions["damage"];
		if (!Damage.isObject() || !Damage.isMember("source_entity"))
		{
			return true;
		}
		if (a_Attacker == nullptr)
		{
			return false;
		}
		return MatchEntityPredicate(Damage["source_entity"], *a_Attacker);
	}





	bool MatchEntityPredicateValue(const Json::Value & a_Predicate, const cEntity & a_Entity)
	{
		if (!a_Predicate.isObject())
		{
			return true;
		}
		if (a_Predicate.isMember("type") && (a_Predicate["type"].asString() != EntityTypeName(a_Entity)))
		{
			return false;
		}
		if (a_Predicate.isMember("player") && (a_Predicate["player"].asBool() != a_Entity.IsPlayer()))
		{
			return false;
		}
		if (a_Predicate.isMember("location") && a_Predicate["location"].isObject())
		{
			const auto & Location = a_Predicate["location"];
			if (Location.isMember("dimension"))
			{
				const auto * World = a_Entity.GetWorld();
				if (World == nullptr)
				{
					return false;
				}
				const auto Dimension = World->GetDimension();
				const AString Actual = (Dimension == dimNether) ? "the_nether" : ((Dimension == dimEnd) ? "the_end" : "overworld");
				AString Expected = Location["dimension"].asString();
				if (Expected.compare(0, 10, "minecraft:") == 0)
				{
					Expected.erase(0, 10);
				}
				if (Expected != Actual)
				{
					return false;
				}
			}
		}
		return true;
	}





	bool MatchEntityPredicate(const Json::Value & a_Conditions, const cEntity & a_Entity)
	{
		if (!a_Conditions.isObject() || !a_Conditions.isMember("entity"))
		{
			return true;
		}
		return MatchEntityPredicateValue(a_Conditions["entity"], a_Entity);
	}





	AString BiomeToAdvancementID(EMCSBiome a_Biome)
	{
		switch (a_Biome)
		{
			case biOcean:              return "ocean";
			case biPlains:             return "plains";
			case biDesert:             return "desert";
			case biExtremeHills:       return "extreme_hills";
			case biForest:             return "forest";
			case biTaiga:              return "taiga";
			case biSwampland:          return "swampland";
			case biRiver:              return "river";
			case biFrozenOcean:        return "frozen_ocean";
			case biFrozenRiver:        return "frozen_river";
			case biIcePlains:          return "ice_flats";
			case biIceMountains:       return "ice_mountains";
			case biMushroomIsland:     return "mushroom_island";
			case biMushroomShore:      return "mushroom_island_shore";
			case biBeach:              return "beaches";
			case biDesertHills:        return "desert_hills";
			case biForestHills:        return "forest_hills";
			case biTaigaHills:         return "taiga_hills";
			case biExtremeHillsEdge:   return "extreme_hills_edge";
			case biJungle:             return "jungle";
			case biJungleHills:        return "jungle_hills";
			case biJungleEdge:         return "jungle_edge";
			case biDeepOcean:          return "deep_ocean";
			case biStoneBeach:         return "stone_beach";
			case biColdBeach:          return "cold_beach";
			case biBirchForest:        return "birch_forest";
			case biBirchForestHills:   return "birch_forest_hills";
			case biRoofedForest:       return "roofed_forest";
			case biColdTaiga:          return "taiga_cold";
			case biColdTaigaHills:     return "taiga_cold_hills";
			case biMegaTaiga:          return "redwood_taiga";
			case biMegaTaigaHills:     return "redwood_taiga_hills";
			case biExtremeHillsPlus:   return "extreme_hills_with_trees";
			case biSavanna:            return "savanna";
			case biSavannaPlateau:     return "savanna_rock";
			case biMesa:               return "mesa";
			case biMesaPlateauF:       return "mesa_rock";
			case biMesaPlateau:        return "mesa_clear_rock";
			case biSunflowerPlains:    return "plains";
			case biDesertM:            return "desert";
			case biExtremeHillsM:      return "extreme_hills";
			case biFlowerForest:       return "forest";
			case biTaigaM:             return "taiga";
			case biSwamplandM:         return "swampland";
			case biIcePlainsSpikes:    return "ice_flats";
			case biJungleM:            return "jungle";
			case biJungleEdgeM:        return "jungle_edge";
			case biBirchForestM:       return "birch_forest";
			case biBirchForestHillsM:  return "birch_forest_hills";
			case biRoofedForestM:      return "roofed_forest";
			case biColdTaigaM:         return "taiga_cold";
			case biMegaSpruceTaiga:    return "redwood_taiga";
			case biMegaSpruceTaigaHills: return "redwood_taiga_hills";
			case biExtremeHillsPlusM:  return "extreme_hills_with_trees";
			case biSavannaM:           return "savanna";
			case biSavannaPlateauM:    return "savanna_rock";
			case biMesaBryce:          return "mesa_rock";
			case biMesaPlateauFM:      return "mesa_rock";
			case biMesaPlateauM:       return "mesa_clear_rock";
			default:                   return "";
		}
	}





	AString NormalizeBiomeName(const AString & a_Name)
	{
		AString Name = a_Name;
		if (Name.compare(0, 10, "minecraft:") == 0)
		{
			Name.erase(0, 10);
		}
		return Name;
	}





	bool MatchInventoryChanged(const Json::Value & a_Conditions, const cInventory & a_Inventory)
	{
		if (!a_Conditions.isObject())
		{
			return true;
		}
		if (a_Conditions.isMember("items") && !InventoryContains(a_Inventory, a_Conditions["items"]))
		{
			return false;
		}
		if (a_Conditions.isMember("slots") && !MatchSlotCounts(a_Conditions["slots"], a_Inventory))
		{
			return false;
		}
		return true;
	}

}  // namespace (anonymous)





void cAdvancementManager::LoadFromFolder(const AString & a_Folder)
{
	m_Advancements.clear();
	m_Children.clear();
	m_Triggers.clear();
	m_PendingTriggers.clear();

	std::vector<std::pair<AString, AString>> Files;
	CollectJsonFiles(a_Folder, "", Files);
	for (const auto & [Full, Relative] : Files)
	{
		AddFile(Full, Relative);
	}

	Validate();
	BuildChildren();

	// Keep only the trigger entries whose advancement and criterion survived validation:
	for (const auto & [Trigger, Entry] : m_PendingTriggers)
	{
		const auto It = m_Advancements.find(Entry.AdvancementID);
		if ((It == m_Advancements.end()) || (It->second.Criteria.find(Entry.Criterion) == It->second.Criteria.end()))
		{
			continue;
		}
		if (!IsImplementedTrigger(Trigger))
		{
			continue;
		}
		AString Reason;
		if (!AreTriggerConditionsSupported(Trigger, Entry.Conditions, Reason))
		{
			LOGWARNING("Advancement \"%s\" criterion \"%s\": %s; criterion will never trigger",
				Entry.AdvancementID.c_str(), Entry.Criterion.c_str(), Reason.c_str());
			continue;
		}
		m_Triggers.emplace(Trigger, Entry);
	}
	BuildInventoryIndex();

	LOGINFO("Loaded %zu advancement(s) from \"%s\"", m_Advancements.size(), a_Folder.c_str());
}





void cAdvancementManager::AddFile(const AString & a_FileName, const AString & a_RelativePath)
{
	AString Relative = a_RelativePath;
	if (EndsWith(Relative, ".json"))
	{
		Relative.erase(Relative.size() - 5);
	}
	const auto Slash = Relative.find('/');
	if ((Slash == AString::npos) || (Slash == 0) || (Slash + 1 >= Relative.size()))
	{
		LOGWARNING("Advancement file \"%s\" must be placed under a namespace folder, skipping", a_FileName.c_str());
		return;
	}
	const AString ID = Relative.substr(0, Slash) + ":" + Relative.substr(Slash + 1);

	const auto Contents = cFile::ReadWholeFile(a_FileName);
	if (Contents.empty())
	{
		LOGWARNING("Cannot read advancement file \"%s\", skipping", a_FileName.c_str());
		return;
	}
	Json::Value Root;
	AString Error;
	if (!JsonUtils::ParseString(Contents, Root, &Error) || !Root.isObject())
	{
		LOGWARNING("Cannot parse advancement file \"%s\": %s", a_FileName.c_str(), Error.c_str());
		return;
	}

	cAdvancement Adv;
	Adv.ID = ID;
	Adv.Parent = Root.isMember("parent") ? Root["parent"].asString() : "";

	if (Root.isMember("display") && Root["display"].isObject())
	{
		const auto & Display = Root["display"];
		Adv.HasDisplay = true;
		Adv.Display.Title = ReadChatComponent(Display["title"]);
		Adv.Display.Description = ReadChatComponent(Display["description"]);
		Adv.Display.Icon = ReadIcon(Display["icon"]);
		Adv.Display.Frame = ReadFrame(Display.isMember("frame") ? Display["frame"].asString() : "task");
		Adv.Display.Background = Display.isMember("background") ? Display["background"].asString() : "";
		Adv.Display.ShowToast = Display.isMember("show_toast") ? Display["show_toast"].asBool() : true;
		Adv.Display.AnnounceToChat = Display.isMember("announce_to_chat") ? Display["announce_to_chat"].asBool() : true;
		Adv.Display.Hidden = Display.isMember("hidden") ? Display["hidden"].asBool() : false;
		Adv.Display.X = Display.isMember("x") ? Display["x"].asFloat() : 0.0f;
		Adv.Display.Y = Display.isMember("y") ? Display["y"].asFloat() : 0.0f;
	}

	if (Root.isMember("criteria") && Root["criteria"].isObject())
	{
		for (auto it = Root["criteria"].begin(); it != Root["criteria"].end(); ++it)
		{
			cAdvancementCriterion Criterion;
			Criterion.Trigger = ((*it).isObject() && (*it).isMember("trigger")) ? (*it)["trigger"].asString() : "";
			const AString CriterionName = it.key().asString();
			Adv.Criteria[CriterionName] = Criterion;
			if (!Criterion.Trigger.empty())
			{
				Json::Value Conditions;
				if ((*it).isObject() && (*it).isMember("conditions"))
				{
					Conditions = (*it)["conditions"];
				}
				m_PendingTriggers.emplace_back(Criterion.Trigger, cTriggerEntry{ID, CriterionName, Conditions});
			}
		}
	}

	if (Root.isMember("requirements") && Root["requirements"].isArray())
	{
		for (const auto & Group : Root["requirements"])
		{
			std::vector<AString> Names;
			if (Group.isArray())
			{
				for (const auto & Name : Group)
				{
					Names.push_back(Name.asString());
				}
			}
			Adv.Requirements.push_back(std::move(Names));
		}
	}

	if (Root.isMember("rewards") && Root["rewards"].isObject())
	{
		const auto & Rewards = Root["rewards"];
		Adv.Rewards.Experience = Rewards.isMember("experience") ? Rewards["experience"].asInt() : 0;
		if (Rewards.isMember("recipes") && Rewards["recipes"].isArray())
		{
			for (const auto & Recipe : Rewards["recipes"])
			{
				Adv.Rewards.Recipes.push_back(Recipe.asString());
			}
		}
		Adv.Rewards.Function = Rewards.isMember("function") ? Rewards["function"].asString() : "";
		if (Rewards.isMember("loot"))
		{
			LOGWARNING("Advancement \"%s\" has rewards.loot, which is not supported yet; ignoring", ID.c_str());
		}
	}

	Adv.LegacyAchievement = Root.isMember("legacy_achievement") ? Root["legacy_achievement"].asString() : "";

	if (m_Advancements.find(ID) != m_Advancements.end())
	{
		LOGWARNING("Duplicate advancement \"%s\" in \"%s\", keeping the first definition", ID.c_str(), a_FileName.c_str());
		return;
	}
	m_Advancements[ID] = std::move(Adv);
}





void cAdvancementManager::Validate(void)
{
	// Drop advancements with missing parents or with requirements that reference unknown criteria:
	bool Changed = true;
	while (Changed)
	{
		Changed = false;
		for (auto it = m_Advancements.begin(); it != m_Advancements.end();)
		{
			const auto & Adv = it->second;
			bool Remove = false;
			if (!Adv.Parent.empty() && (m_Advancements.find(Adv.Parent) == m_Advancements.end()))
			{
				LOGWARNING("Advancement \"%s\" has unknown parent \"%s\", dropping it", Adv.ID.c_str(), Adv.Parent.c_str());
				Remove = true;
			}
			for (const auto & Group : Adv.Requirements)
			{
				for (const auto & Name : Group)
				{
					if (Adv.Criteria.find(Name) == Adv.Criteria.end())
					{
						LOGWARNING("Advancement \"%s\" requires unknown criterion \"%s\", dropping it", Adv.ID.c_str(), Name.c_str());
						Remove = true;
					}
				}
			}
			if (Remove)
			{
				it = m_Advancements.erase(it);
				Changed = true;
			}
			else
			{
				++it;
			}
		}
	}

	// Drop advancements that are part of a parent cycle:
	for (auto it = m_Advancements.begin(); it != m_Advancements.end();)
	{
		std::set<AString> Seen;
		const cAdvancement * Adv = &it->second;
		bool Cycle = false;
		while ((Adv != nullptr) && !Adv->Parent.empty())
		{
			if (!Seen.insert(Adv->ID).second)
			{
				Cycle = true;
				break;
			}
			const auto Parent = m_Advancements.find(Adv->Parent);
			Adv = (Parent == m_Advancements.end()) ? nullptr : &Parent->second;
		}
		if (Cycle)
		{
			LOGWARNING("Advancement \"%s\" is part of a parent cycle, dropping it", it->second.ID.c_str());
			it = m_Advancements.erase(it);
		}
		else
		{
			++it;
		}
	}
}





void cAdvancementManager::BuildChildren(void)
{
	m_Children.clear();
	for (const auto & [ID, Adv] : m_Advancements)
	{
		if (!Adv.Parent.empty())
		{
			m_Children[Adv.Parent].push_back(ID);
		}
	}
	for (auto & [Parent, Children] : m_Children)
	{
		std::sort(Children.begin(), Children.end());
	}
}





const cAdvancement * cAdvancementManager::GetAdvancement(const AString & a_ID) const
{
	const auto it = m_Advancements.find(a_ID);
	return (it == m_Advancements.end()) ? nullptr : &it->second;
}





bool cAdvancementManager::IsValid(const AString & a_ID) const
{
	return m_Advancements.find(a_ID) != m_Advancements.end();
}





AString cAdvancementManager::GetParent(const AString & a_ID) const
{
	const auto it = m_Advancements.find(a_ID);
	return (it == m_Advancements.end()) ? AString() : it->second.Parent;
}





AStringVector cAdvancementManager::GetChildren(const AString & a_ID) const
{
	const auto it = m_Children.find(a_ID);
	return (it == m_Children.end()) ? AStringVector() : it->second;
}





AStringVector cAdvancementManager::GetAllIDs(void) const
{
	AStringVector IDs;
	IDs.reserve(m_Advancements.size());
	for (const auto & [ID, Adv] : m_Advancements)
	{
		IDs.push_back(ID);
	}
	return IDs;
}





bool cAdvancementManager::IsCompleted(const cAdvancement & a_Advancement, const std::map<AString, Int64> & a_Progress) const
{
	if (a_Advancement.Criteria.empty())
	{
		return false;
	}
	if (a_Advancement.Requirements.empty())
	{
		for (const auto & [Name, Criterion] : a_Advancement.Criteria)
		{
			if (a_Progress.find(Name) == a_Progress.end())
			{
				return false;
			}
		}
		return true;
	}
	for (const auto & Group : a_Advancement.Requirements)
	{
		bool AnyAchieved = false;
		for (const auto & Name : Group)
		{
			if (a_Progress.find(Name) != a_Progress.end())
			{
				AnyAchieved = true;
				break;
			}
		}
		if (!AnyAchieved)
		{
			return false;
		}
	}
	return true;
}





cAdvancementProgress cAdvancementManager::BuildProgress(const cAdvancement & a_Advancement, const std::map<AString, Int64> & a_Progress) const
{
	cAdvancementProgress Result;
	Result.ID = a_Advancement.ID;
	for (const auto & [Name, Criterion] : a_Advancement.Criteria)
	{
		const auto it = a_Progress.find(Name);
		if (it != a_Progress.end())
		{
			Result.Criteria[Name] = it->second;
		}
		else
		{
			Result.Criteria[Name] = std::nullopt;
		}
	}
	return Result;
}





std::set<AString> cAdvancementManager::ComputeVisible(const std::map<AString, std::map<AString, Int64>> & a_Progress) const
{
	std::set<AString> Visible;

	// Adds the advancement and all its ancestors up to the tab root:
	const auto AddWithAncestors = [this, &Visible](const AString & a_ID)
	{
		AString Current = a_ID;
		while (!Current.empty())
		{
			Visible.insert(Current);
			const auto It = m_Advancements.find(Current);
			if (It == m_Advancements.end())
			{
				break;
			}
			Current = It->second.Parent;
		}
	};

	for (const auto & [ID, Advancement] : m_Advancements)
	{
		const auto ProgressIt = a_Progress.find(ID);
		if (ProgressIt == a_Progress.end())
		{
			continue;
		}
		if (!IsCompleted(Advancement, ProgressIt->second))
		{
			continue;
		}

		// Two parent / child steps around the completed advancement:
		std::set<AString> Reachable{ID};
		std::vector<AString> Frontier{ID};
		for (int Depth = 0; Depth < 2; ++Depth)
		{
			std::vector<AString> Next;
			for (const auto & Current : Frontier)
			{
				const auto CurrentIt = m_Advancements.find(Current);
				if ((CurrentIt != m_Advancements.end()) && !CurrentIt->second.Parent.empty() && Reachable.insert(CurrentIt->second.Parent).second)
				{
					Next.push_back(CurrentIt->second.Parent);
				}
				const auto Children = m_Children.find(Current);
				if (Children != m_Children.end())
				{
					for (const auto & Child : Children->second)
					{
						if (Reachable.insert(Child).second)
						{
							Next.push_back(Child);
						}
					}
				}
			}
			Frontier = std::move(Next);
		}
		for (const auto & VisibleID : Reachable)
		{
			AddWithAncestors(VisibleID);
		}
	}
	return Visible;
}





cAdvancementUpdate cAdvancementManager::BuildJoinUpdate(const std::map<AString, std::map<AString, Int64>> & a_Progress) const
{
	cAdvancementUpdate Update;
	Update.Reset = true;
	static const std::map<AString, Int64> EmptyProgress;
	for (const auto & ID : ComputeVisible(a_Progress))
	{
		const auto * Advancement = GetAdvancement(ID);
		if (Advancement == nullptr)
		{
			continue;
		}
		Update.Advancements.push_back(*Advancement);
		if (!Advancement->Criteria.empty())
		{
			const auto It = a_Progress.find(ID);
			Update.Progress.push_back(BuildProgress(*Advancement, (It == a_Progress.end()) ? EmptyProgress : It->second));
		}
	}
	return Update;
}





cAdvancementUpdate cAdvancementManager::BuildUpdate(
	const std::map<AString, std::map<AString, Int64>> & a_Progress,
	const std::set<AString> & a_VisibleBefore,
	const std::set<AString> & a_VisibleAfter,
	const AString & a_ChangedID
) const
{
	cAdvancementUpdate Update;
	Update.Reset = false;
	static const std::map<AString, Int64> EmptyProgress;

	// The definitions that became visible:
	std::set<AString> ProgressIDs;
	for (const auto & ID : a_VisibleAfter)
	{
		if (a_VisibleBefore.find(ID) == a_VisibleBefore.end())
		{
			const auto * Advancement = GetAdvancement(ID);
			if (Advancement != nullptr)
			{
				Update.Advancements.push_back(*Advancement);
			}
			ProgressIDs.insert(ID);
		}
	}

	// The changed advancement, if it is visible. Its definition is sent alongside the progress:
	// the client uses the display data when it decides to show the toast.
	if (a_VisibleAfter.find(a_ChangedID) != a_VisibleAfter.end())
	{
		if (a_VisibleBefore.find(a_ChangedID) == a_VisibleBefore.end())
		{
			// Already added above as newly visible.
		}
		else
		{
			const auto * Changed = GetAdvancement(a_ChangedID);
			if (Changed != nullptr)
			{
				Update.Advancements.push_back(*Changed);
			}
		}
		ProgressIDs.insert(a_ChangedID);
	}
	for (const auto & ID : ProgressIDs)
	{
		const auto * Advancement = GetAdvancement(ID);
		if ((Advancement == nullptr) || Advancement->Criteria.empty())
		{
			continue;
		}
		const auto It = a_Progress.find(ID);
		Update.Progress.push_back(BuildProgress(*Advancement, (It == a_Progress.end()) ? EmptyProgress : It->second));
	}

	// The advancements that are no longer visible:
	for (const auto & ID : a_VisibleBefore)
	{
		if (a_VisibleAfter.find(ID) == a_VisibleAfter.end())
		{
			Update.Removals.push_back(ID);
		}
	}
	return Update;
}





void cAdvancementManager::TriggerInventoryChanged(cPlayer & a_Player, short a_ChangedItemType)
{
	std::vector<const cTriggerEntry *> Candidates;
	const auto It = m_InventoryItemIndex.find(a_ChangedItemType);
	if (It != m_InventoryItemIndex.end())
	{
		Candidates.insert(Candidates.end(), It->second.begin(), It->second.end());
	}
	Candidates.insert(Candidates.end(), m_InventorySlotEntries.begin(), m_InventorySlotEntries.end());
	Candidates.insert(Candidates.end(), m_InventoryUnconditional.begin(), m_InventoryUnconditional.end());
	std::sort(Candidates.begin(), Candidates.end());
	Candidates.erase(std::unique(Candidates.begin(), Candidates.end()), Candidates.end());

	const auto & Inventory = a_Player.GetInventory();
	for (const auto * Entry : Candidates)
	{
		if (a_Player.HasAdvancementCriterion(Entry->AdvancementID, Entry->Criterion))
		{
			continue;
		}
		if (MatchInventoryChanged(Entry->Conditions, Inventory))
		{
			a_Player.GrantAdvancementCriterion(Entry->AdvancementID, Entry->Criterion);
		}
	}
}





void cAdvancementManager::BuildInventoryIndex(void)
{
	m_InventoryItemIndex.clear();
	m_InventorySlotEntries.clear();
	m_InventoryUnconditional.clear();
	const auto Range = m_Triggers.equal_range("minecraft:inventory_changed");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		const auto & Conditions = Entry.Conditions;
		bool HasItems = false;
		bool HasSlots = false;
		if (Conditions.isObject())
		{
			if (Conditions.isMember("items") && Conditions["items"].isArray())
			{
				for (const auto & Predicate : Conditions["items"])
				{
					if (!Predicate.isObject() || !Predicate.isMember("item"))
					{
						continue;
					}
					const auto & Item = Predicate["item"];
					short ItemType = 0;
					if (Item.isInt())
					{
						ItemType = static_cast<short>(Item.asInt());
					}
					else if (Item.isString())
					{
						cItem Resolved;
						if (!StringToItem(Item.asString(), Resolved))
						{
							continue;
						}
						ItemType = Resolved.m_ItemType;
					}
					if (ItemType == 0)
					{
						continue;
					}
					HasItems = true;
					m_InventoryItemIndex[ItemType].push_back(&Entry);
				}
			}
			HasSlots = Conditions.isMember("slots");
		}
		if (HasSlots)
		{
			m_InventorySlotEntries.push_back(&Entry);
		}
		if (!HasItems && !HasSlots)
		{
			m_InventoryUnconditional.push_back(&Entry);
		}
	}
}





void cAdvancementManager::TriggerPlayerKilledEntity(cPlayer & a_Player, const cEntity & a_Victim)
{
	const auto Range = m_Triggers.equal_range("minecraft:player_killed_entity");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			continue;
		}
		if (Entry.Conditions.isMember("player") && (!MatchEntityPredicateValue(Entry.Conditions["player"], a_Player)))
		{
			continue;
		}
		if (MatchEntityPredicate(Entry.Conditions, a_Victim))
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





void cAdvancementManager::TriggerEntityKilledPlayer(cPlayer & a_Player, const cEntity * a_Killer)
{
	if (a_Killer == nullptr)
	{
		return;
	}
	const auto Range = m_Triggers.equal_range("minecraft:entity_killed_player");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			continue;
		}
		if (MatchEntityPredicate(Entry.Conditions, *a_Killer))
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





void cAdvancementManager::TriggerPlayerHurtEntity(cPlayer & a_Player, const cEntity & a_Victim)
{
	const auto Range = m_Triggers.equal_range("minecraft:player_hurt_entity");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			continue;
		}
		if (MatchEntityPredicate(Entry.Conditions, a_Victim))
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





void cAdvancementManager::TriggerEntityHurtPlayer(cPlayer & a_Player, const cEntity * a_Attacker)
{
	const auto Range = m_Triggers.equal_range("minecraft:entity_hurt_player");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			continue;
		}
		if (MatchDamageSource(Entry.Conditions, a_Attacker))
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





void cAdvancementManager::TriggerItemDurabilityChanged(cPlayer & a_Player, const cItem & a_ItemBefore, int a_Delta, int a_Durability)
{
	const auto Range = m_Triggers.equal_range("minecraft:item_durability_changed");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			continue;
		}
		if (MatchDurabilityChanged(Entry.Conditions, a_ItemBefore, a_Delta, a_Durability))
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





void cAdvancementManager::TriggerConsumedItem(cPlayer & a_Player, const cItem & a_Item)
{
	const auto Range = m_Triggers.equal_range("minecraft:consume_item");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			continue;
		}
		if (!Entry.Conditions.isObject() || !Entry.Conditions.isMember("item") ||
			MatchItemPredicate(Entry.Conditions["item"], a_Item))
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





void cAdvancementManager::TriggerPlacedBlock(cPlayer & a_Player, BLOCKTYPE a_BlockType, const cItem & a_HeldItem)
{
	const auto Range = m_Triggers.equal_range("minecraft:placed_block");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			continue;
		}
		bool Matches = true;
		if (Entry.Conditions.isObject())
		{
			if (Entry.Conditions.isMember("block") && !MatchBlockName(Entry.Conditions["block"], a_BlockType))
			{
				Matches = false;
			}
			if (Entry.Conditions.isMember("item") && !MatchItemPredicate(Entry.Conditions["item"], a_HeldItem))
			{
				Matches = false;
			}
		}
		if (Matches)
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





void cAdvancementManager::TriggerUsedEnderEye(cPlayer & a_Player, int a_Distance)
{
	const auto Range = m_Triggers.equal_range("minecraft:used_ender_eye");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			continue;
		}
		if (!Entry.Conditions.isObject() || !Entry.Conditions.isMember("distance") ||
			MatchRange(Entry.Conditions["distance"], a_Distance))
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





void cAdvancementManager::TriggerUsedTotem(cPlayer & a_Player, const cItem & a_Item)
{
	const auto Range = m_Triggers.equal_range("minecraft:used_totem");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			continue;
		}
		if (!Entry.Conditions.isObject() || !Entry.Conditions.isMember("item") ||
			MatchItemPredicate(Entry.Conditions["item"], a_Item))
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





void cAdvancementManager::TriggerRecipeUnlocked(cPlayer & a_Player, UInt32 a_RecipeId)
{
	const auto Range = m_Triggers.equal_range("minecraft:recipe_unlocked");
	if (Range.first == Range.second)
	{
		return;
	}
	AString RecipeName;
	const auto & NameMap = cRoot::Get()->GetCraftingRecipes()->GetRecipeNameMap();
	for (const auto & Entry : NameMap)
	{
		if (Entry.second == a_RecipeId)
		{
			RecipeName = Entry.first;
			break;
		}
	}
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			continue;
		}
		bool Matches = true;
		if (Entry.Conditions.isObject() && Entry.Conditions.isMember("recipe"))
		{
			AString Expected = Entry.Conditions["recipe"].asString();
			if (Expected.compare(0, 10, "minecraft:") == 0)
			{
				Expected.erase(0, 10);
			}
			Matches = (Expected == RecipeName);
		}
		if (Matches)
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





void cAdvancementManager::TriggerEffectsChanged(cPlayer & a_Player)
{
	const auto Range = m_Triggers.equal_range("minecraft:effects_changed");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			continue;
		}
		if (MatchEffectsConditions(Entry.Conditions, a_Player))
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





	AString DimensionName(int a_Dimension)
	{
		switch (a_Dimension)
		{
			case dimNether: return "the_nether";
			case dimEnd:    return "the_end";
			default:        return "overworld";
		}
	}





	AString NormalizeDimensionName(const AString & a_Name)
	{
		AString Name = a_Name;
		if (Name.compare(0, 10, "minecraft:") == 0)
		{
			Name.erase(0, 10);
		}
		return Name;
	}





void cAdvancementManager::TriggerChangedDimension(cPlayer & a_Player, int a_From, int a_To)
{
	const auto Range = m_Triggers.equal_range("minecraft:changed_dimension");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			continue;
		}
		bool Matches = true;
		if (Entry.Conditions.isObject())
		{
			if (Entry.Conditions.isMember("from") &&
				(NormalizeDimensionName(Entry.Conditions["from"].asString()) != DimensionName(a_From)))
			{
				Matches = false;
			}
			if (Entry.Conditions.isMember("to") &&
				(NormalizeDimensionName(Entry.Conditions["to"].asString()) != DimensionName(a_To)))
			{
				Matches = false;
			}
		}
		if (Matches)
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





void cAdvancementManager::TriggerTick(cPlayer & a_Player)
{
	const auto Range = m_Triggers.equal_range("minecraft:tick");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (!a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





void cAdvancementManager::TriggerEnterBlock(cPlayer & a_Player, BLOCKTYPE a_BlockType)
{
	const auto Range = m_Triggers.equal_range("minecraft:enter_block");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			continue;
		}
		if (!Entry.Conditions.isObject() || !Entry.Conditions.isMember("block") ||
			MatchBlockName(Entry.Conditions["block"], a_BlockType))
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





void cAdvancementManager::TriggerEnchantedItem(cPlayer & a_Player, const cItem & a_Item, int a_Levels)
{
	const auto Range = m_Triggers.equal_range("minecraft:enchanted_item");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			continue;
		}
		bool Matches = true;
		if (Entry.Conditions.isObject())
		{
			if (Entry.Conditions.isMember("item") && !MatchItemPredicate(Entry.Conditions["item"], a_Item))
			{
				Matches = false;
			}
			if (Entry.Conditions.isMember("levels") && !MatchRange(Entry.Conditions["levels"], a_Levels))
			{
				Matches = false;
			}
		}
		if (Matches)
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





void cAdvancementManager::TriggerSleptInBed(cPlayer & a_Player)
{
	const auto Range = m_Triggers.equal_range("minecraft:slept_in_bed");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (!a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





void cAdvancementManager::TriggerSummonedEntity(cPlayer & a_Player, const cEntity & a_Entity)
{
	const auto Range = m_Triggers.equal_range("minecraft:summoned_entity");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			continue;
		}
		if (MatchEntityPredicate(Entry.Conditions, a_Entity))
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





void cAdvancementManager::TriggerTameAnimal(cPlayer & a_Player, const cEntity & a_Animal)
{
	const auto Range = m_Triggers.equal_range("minecraft:tame_animal");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			continue;
		}
		if (MatchEntityPredicate(Entry.Conditions, a_Animal))
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





void cAdvancementManager::TriggerBredAnimals(cPlayer & a_Player, const cEntity & a_Parent, const cEntity & a_Partner, const cEntity * a_Child)
{
	const auto Range = m_Triggers.equal_range("minecraft:bred_animals");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			continue;
		}
		bool Matches = true;
		if (Entry.Conditions.isObject())
		{
			if (Entry.Conditions.isMember("parent") && !MatchEntityPredicateValue(Entry.Conditions["parent"], a_Parent))
			{
				Matches = false;
			}
			if (Entry.Conditions.isMember("partner") && !MatchEntityPredicateValue(Entry.Conditions["partner"], a_Partner))
			{
				Matches = false;
			}
			if (Entry.Conditions.isMember("child") && ((a_Child == nullptr) || !MatchEntityPredicateValue(Entry.Conditions["child"], *a_Child)))
			{
				Matches = false;
			}
		}
		if (Matches)
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





void cAdvancementManager::TriggerCuredZombieVillager(cPlayer & a_Player, const cEntity & a_Zombie, const cEntity * a_Villager)
{
	const auto Range = m_Triggers.equal_range("minecraft:cured_zombie_villager");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			continue;
		}
		bool Matches = true;
		if (Entry.Conditions.isObject())
		{
			if (Entry.Conditions.isMember("zombie") && !MatchEntityPredicateValue(Entry.Conditions["zombie"], a_Zombie))
			{
				Matches = false;
			}
			if (Entry.Conditions.isMember("villager") &&
				((a_Villager == nullptr) || !MatchEntityPredicateValue(Entry.Conditions["villager"], *a_Villager)))
			{
				Matches = false;
			}
		}
		if (Matches)
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





void cAdvancementManager::TriggerBrewedPotion(cPlayer & a_Player, const cItem & a_Item)
{
	const auto Range = m_Triggers.equal_range("minecraft:brewed_potion");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			continue;
		}
		if (!Entry.Conditions.isObject() || (Entry.Conditions.size() == 0))
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





void cAdvancementManager::TriggerConstructBeacon(cPlayer & a_Player, int a_Level)
{
	const auto Range = m_Triggers.equal_range("minecraft:construct_beacon");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			continue;
		}
		if (!Entry.Conditions.isObject() || !Entry.Conditions.isMember("level") ||
			MatchRange(Entry.Conditions["level"], a_Level))
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





void cAdvancementManager::TriggerLocation(cPlayer & a_Player)
{
	const auto Range = m_Triggers.equal_range("minecraft:location");
	if (Range.first == Range.second)
	{
		return;
	}
	const auto Pos = a_Player.GetPosition();
	const int BlockX = static_cast<int>(std::floor(Pos.x));
	const int BlockY = static_cast<int>(std::floor(Pos.y));
	const int BlockZ = static_cast<int>(std::floor(Pos.z));
	auto * World = a_Player.GetWorld();
	const AString Dimension = (World != nullptr) ? DimensionName(World->GetDimension()) : "overworld";
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			continue;
		}
		bool Matches = true;
		if (Entry.Conditions.isObject())
		{
			if (Entry.Conditions.isMember("dimension") &&
				(NormalizeDimensionName(Entry.Conditions["dimension"].asString()) != Dimension))
			{
				Matches = false;
			}
			if (Entry.Conditions.isMember("biome"))
			{
				const AString ActualBiome = (World != nullptr) ? BiomeToAdvancementID(World->GetBiomeAt(BlockX, BlockZ)) : AString();
				const AString ExpectedBiome = NormalizeBiomeName(Entry.Conditions["biome"].asString());
				if (ActualBiome != ExpectedBiome)
				{
					Matches = false;
				}
			}
			if (Entry.Conditions.isMember("position") && Entry.Conditions["position"].isObject())
			{
				const auto & Position = Entry.Conditions["position"];
				if (Position.isMember("x") && !MatchRange(Position["x"], BlockX))
				{
					Matches = false;
				}
				if (Position.isMember("y") && !MatchRange(Position["y"], BlockY))
				{
					Matches = false;
				}
				if (Position.isMember("z") && !MatchRange(Position["z"], BlockZ))
				{
					Matches = false;
				}
			}
			if (Entry.Conditions.isMember("feature"))
			{
				const AString Feature = Entry.Conditions["feature"].asString();
				const bool Inside = (World != nullptr) && World->GetGenerator().IsInsideStructure(Feature, Vector3i(BlockX, BlockY, BlockZ));
				if (!Inside)
				{
					Matches = false;
				}
			}
		}
		if (Matches)
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





void cAdvancementManager::TriggerNetherTravel(cPlayer & a_Player, const Vector3d & a_Entry, const Vector3d & a_Exit)
{
	const double DX = std::abs(a_Exit.x - a_Entry.x);
	const double DY = std::abs(a_Exit.y - a_Entry.y);
	const double DZ = std::abs(a_Exit.z - a_Entry.z);
	const double Horizontal = std::hypot(DX, DZ);
	const double Absolute = std::sqrt(DX * DX + DY * DY + DZ * DZ);
	const auto Range = m_Triggers.equal_range("minecraft:nether_travel");
	for (auto It = Range.first; It != Range.second; ++It)
	{
		const auto & Entry = It->second;
		if (a_Player.HasAdvancementCriterion(Entry.AdvancementID, Entry.Criterion))
		{
			continue;
		}
		bool Matches = true;
		if (Entry.Conditions.isObject() && Entry.Conditions.isMember("distance"))
		{
			const auto & Distance = Entry.Conditions["distance"];
			if (Distance.isMember("absolute") && !MatchDoubleRange(Distance["absolute"], Absolute))
			{
				Matches = false;
			}
			if (Distance.isMember("horizontal") && !MatchDoubleRange(Distance["horizontal"], Horizontal))
			{
				Matches = false;
			}
			if (Distance.isMember("x") && !MatchDoubleRange(Distance["x"], DX))
			{
				Matches = false;
			}
			if (Distance.isMember("y") && !MatchDoubleRange(Distance["y"], DY))
			{
				Matches = false;
			}
			if (Distance.isMember("z") && !MatchDoubleRange(Distance["z"], DZ))
			{
				Matches = false;
			}
		}
		if (Matches)
		{
			a_Player.GrantAdvancementCriterion(Entry.AdvancementID, Entry.Criterion);
		}
	}
}





AString cAdvancementManager::BuildAnnouncementJson(const cAdvancement & a_Advancement, const AString & a_PlayerName, const AString & a_PlayerUUID)
{
	Json::Value Title = ParseChatComponent(a_Advancement.Display.Title);
	Json::Value Description = ParseChatComponent(a_Advancement.Display.Description);
	Title["color"] = "green";

	// The hover text of the title shows the title, a newline and the description:
	Json::Value HoverValue(Json::objectValue);
	HoverValue["text"] = "";
	Json::Value HoverExtra(Json::arrayValue);
	Json::Value TitleInHover = Title;
	TitleInHover["color"] = "green";
	HoverExtra.append(TitleInHover);
	Json::Value Newline;
	Newline["text"] = "\n";
	HoverExtra.append(Newline);
	HoverExtra.append(Description);
	HoverValue["extra"] = HoverExtra;
	Json::Value Hover;
	Hover["action"] = "show_text";
	Hover["value"] = HoverValue;
	Title["hoverEvent"] = Hover;

	// The second argument is a green "[title]" component:
	Json::Value Green(Json::objectValue);
	Green["color"] = "green";
	Green["text"] = "[";
	Json::Value GreenExtra(Json::arrayValue);
	GreenExtra.append(Title);
	Json::Value Close;
	Close["text"] = "]";
	GreenExtra.append(Close);
	Green["extra"] = GreenExtra;

	// The first argument is the player name with the vanilla click / hover actions:
	Json::Value Player(Json::objectValue);
	Player["text"] = a_PlayerName;
	Player["insertion"] = a_PlayerName;
	Json::Value Click;
	Click["action"] = "suggest_command";
	Click["value"] = "/msg " + a_PlayerName + " ";
	Player["clickEvent"] = Click;
	Json::Value Entity;
	const AString Separator = ",";
	Entity["text"] = "{name:\"" + a_PlayerName + "\"" + Separator + "id:\"" + a_PlayerUUID + "\"}";
	Json::Value HoverEntity;
	HoverEntity["action"] = "show_entity";
	HoverEntity["value"] = Entity;
	Player["hoverEvent"] = HoverEntity;

	Json::Value With(Json::arrayValue);
	With.append(Player);
	With.append(Green);

	Json::Value Root(Json::objectValue);
	Root["translate"] = AString("chat.type.advancement.") + FrameToKey(a_Advancement.Display.Frame);
	Root["with"] = With;
	return JsonUtils::WriteFastString(Root);
}
