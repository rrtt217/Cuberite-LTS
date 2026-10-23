// AdvancementManager.h

// Declares the cAdvancementManager class that loads advancement definitions and builds the updates
// sent to 1.12+ clients.

#pragma once

#include "Advancement.h"

#include <json/json.h>

#include <map>
#include <set>
#include <utility>
#include <vector>





class cEntity;
class cPlayer;





class cAdvancementManager
{
public:

	/** Loads all advancement definitions from the specified folder (CWD-relative).
	The identifier of each definition is derived from its path: <folder>/<namespace>/<path>.json
	becomes <namespace>:<path>. */
	void LoadFromFolder(const AString & a_Folder);

	/** Returns the advancement with the specified identifier, or nullptr if it is unknown. */
	const cAdvancement * GetAdvancement(const AString & a_ID) const;

	/** Returns true if the identifier names a loaded advancement. */
	bool IsValid(const AString & a_ID) const;

	/** Runs the minecraft:inventory_changed criteria against the player's current inventory. */
	void TriggerInventoryChanged(cPlayer & a_Player);

	/** Runs the minecraft:player_killed_entity criteria for the killing player. */
	void TriggerPlayerKilledEntity(cPlayer & a_Player, const cEntity & a_Victim);

	/** Runs the minecraft:entity_killed_player criteria for the dying player. */
	void TriggerEntityKilledPlayer(cPlayer & a_Player, const cEntity * a_Killer);

	/** Runs the minecraft:player_hurt_entity criteria for the attacking player. */
	void TriggerPlayerHurtEntity(cPlayer & a_Player, const cEntity & a_Victim);

	/** Runs the minecraft:entity_hurt_player criteria for the hurt player. */
	void TriggerEntityHurtPlayer(cPlayer & a_Player, const cEntity * a_Attacker);

	/** Runs the minecraft:item_durability_changed criteria for the player's item. */
	void TriggerItemDurabilityChanged(cPlayer & a_Player, const cItem & a_ItemBefore, int a_Delta, int a_Durability);

	/** Runs the minecraft:consumed_item criteria for the eaten item. */
	void TriggerConsumedItem(cPlayer & a_Player, const cItem & a_Item);

	/** Runs the minecraft:placed_block criteria for a successfully placed block. */
	void TriggerPlacedBlock(cPlayer & a_Player, BLOCKTYPE a_BlockType, const cItem & a_HeldItem);

	/** Runs the minecraft:used_ender_eye criteria for a thrown eye of ender, with the horizontal stronghold distance. */
	void TriggerUsedEnderEye(cPlayer & a_Player, int a_Distance);

	/** Runs the minecraft:used_totem criteria for an activated totem. */
	void TriggerUsedTotem(cPlayer & a_Player, const cItem & a_Item);

	/** Runs the minecraft:recipe_unlocked criteria for a newly unlocked recipe. */
	void TriggerRecipeUnlocked(cPlayer & a_Player, UInt32 a_RecipeId);

	/** Runs the minecraft:effects_changed criteria after the player's effects changed. */
	void TriggerEffectsChanged(cPlayer & a_Player);

	/** Runs the minecraft:changed_dimension criteria after a player changed dimension. */
	void TriggerChangedDimension(cPlayer & a_Player, int a_From, int a_To);

	/** Runs the minecraft:tick criteria for the player (called once per player tick). */
	void TriggerTick(cPlayer & a_Player);

	/** Runs the minecraft:enter_block criteria for a block the player just entered. */
	void TriggerEnterBlock(cPlayer & a_Player, BLOCKTYPE a_BlockType);

	/** Runs the minecraft:enchanted_item criteria for an enchanted item. */
	void TriggerEnchantedItem(cPlayer & a_Player, const cItem & a_Item, int a_Levels);

	/** Runs the minecraft:slept_in_bed criteria after the player successfully slept. */
	void TriggerSleptInBed(cPlayer & a_Player);

	/** Runs the minecraft:summoned_entity criteria for an entity the player summoned. */
	void TriggerSummonedEntity(cPlayer & a_Player, const cEntity & a_Entity);

	/** Returns the parent of the specified advancement, or an empty string for roots / unknown IDs. */
	AString GetParent(const AString & a_ID) const;

	/** Returns the direct children of the specified advancement, sorted by identifier. */
	AStringVector GetChildren(const AString & a_ID) const;

	/** Returns all loaded identifiers, sorted. */
	AStringVector GetAllIDs() const;

	/** Returns true if all requirements of the advancement are met by the specified achieved criteria. */
	bool IsCompleted(const cAdvancement & a_Advancement, const std::map<AString, Int64> & a_Progress) const;

	/** Builds the progress entry for the advancement, listing every criterion. */
	cAdvancementProgress BuildProgress(const cAdvancement & a_Advancement, const std::map<AString, Int64> & a_Progress) const;

	/** Returns the identifiers of the advancements that are currently visible to the player:
	a completed advancement and everything within two parent / child steps of it, plus the ancestors
	up to the tab root (so that the client can create the tab). */
	std::set<AString> ComputeVisible(const std::map<AString, std::map<AString, Int64>> & a_Progress) const;

	/** Builds the update sent when a 1.12+ player joins: only visible advancements, with progress. */
	cAdvancementUpdate BuildJoinUpdate(const std::map<AString, std::map<AString, Int64>> & a_Progress) const;

	/** Builds an incremental update after a progress change.
	Mapping carries the advancements that became visible, progress carries the changed advancement
	and the newly visible ones, removals carries the advancements that are no longer visible. */
	cAdvancementUpdate BuildUpdate(
		const std::map<AString, std::map<AString, Int64>> & a_Progress,
		const std::set<AString> & a_VisibleBefore,
		const std::set<AString> & a_VisibleAfter,
		const AString & a_ChangedID
	) const;

	/** Builds the chat message announced when an advancement with announce_to_chat is completed. */
	static AString BuildAnnouncementJson(const cAdvancement & a_Advancement, const AString & a_PlayerName, const AString & a_PlayerUUID);

private:

	/** One criterion that waits for a trigger, with the parsed conditions. */
	struct cTriggerEntry
	{
		AString AdvancementID;
		AString Criterion;
		Json::Value Conditions;
	};

	/** Loaded advancements, keyed by identifier. A std::map keeps the join update deterministic. */
	std::map<AString, cAdvancement> m_Advancements;

	/** Direct children of each advancement. */
	std::map<AString, AStringVector> m_Children;

	/** Criteria indexed by trigger name; only criteria of surviving advancements are present. */
	std::multimap<AString, cTriggerEntry> m_Triggers;

	/** Trigger entries collected while loading, filtered against m_Advancements after validation. */
	std::vector<std::pair<AString, cTriggerEntry>> m_PendingTriggers;

	/** Adds one parsed JSON file to the manager. */
	void AddFile(const AString & a_FileName, const AString & a_RelativePath);

	/** Drops advancements with missing parents, parent cycles or invalid requirements. */
	void Validate(void);

	/** Rebuilds m_Children from the loaded advancements. */
	void BuildChildren(void);
};
