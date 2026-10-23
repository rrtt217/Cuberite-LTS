// Advancement.h

// Declares the data structures describing a single advancement, its per-player progress and the
// update carried by one 1.12.2 Advancements packet.

#pragma once

#include "../Defines.h"
#include "../Item.h"

#include <map>
#include <optional>
#include <vector>





/** The frame drawn around an advancement's icon.
The values are not the on-wire values; the protocol translates them when serializing. */
enum class eAdvancementFrame
{
	Task,
	Challenge,
	Goal,
};





/** The optional display data of an advancement.
The title and description are JSON text component strings sent to the client verbatim. */
struct cAdvancementDisplay
{
	AString Title;
	AString Description;
	cItem Icon;
	eAdvancementFrame Frame = eAdvancementFrame::Task;
	bool ShowToast = true;
	bool AnnounceToChat = true;
	bool Hidden = false;
	AString Background;  // Empty when the advancement has no background texture
	float X = 0;
	float Y = 0;
};





/** A single criterion of an advancement.
The trigger and its conditions are consumed by the trigger layer (a follow-up branch); the manager
only needs the criterion name and trigger for now. */
struct cAdvancementCriterion
{
	AString Trigger;
};





/** The rewards granted when an advancement is completed. */
struct cAdvancementRewards
{
	int Experience = 0;
	AStringVector Recipes;
	AString Function;
};





/** A single advancement definition, as declared to the client. */
struct cAdvancement
{
	AString ID;
	AString Parent;  // Empty for root advancements
	bool HasDisplay = false;
	cAdvancementDisplay Display;
	std::map<AString, cAdvancementCriterion> Criteria;
	std::vector<std::vector<AString>> Requirements;  // Empty means "all criteria are required"
	cAdvancementRewards Rewards;
	AString LegacyAchievement;  // Non-empty maps the completion to a 1.8-1.11 achievement (M6)
};





/** The progress a player has made on a single advancement.
Every criterion of the advancement is listed; an empty optional means "not achieved yet". */
struct cAdvancementProgress
{
	AString ID;
	std::map<AString, std::optional<Int64>> Criteria;
};





/** A complete update carried by one Advancements packet. */
struct cAdvancementUpdate
{
	bool Reset = false;
	std::vector<cAdvancement> Advancements;
	std::vector<AString> Removals;
	std::vector<cAdvancementProgress> Progress;
};
