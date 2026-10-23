// Advancement.h

// Declares the data structures describing a single advancement and its per-player progress.
// Only the fields needed by the 1.12.2 Advancements packet are modelled here; the JSON loader and
// the criteria engine are added by follow-up branches.

#pragma once

#include "../Defines.h"
#include "../Item.h"

#include <map>
#include <vector>





/** The frame drawn around an advancement's icon.
The values are not the on-wire values; the protocol translates them when serializing
(see cProtocol_1_12::SendAdvancements). */
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
	bool Hidden = false;
	AString Background;  // Empty when the advancement has no background texture
	float X = 0;
	float Y = 0;
};





/** A single advancement definition, as declared to the client. */
struct cAdvancement
{
	AString ID;
	AString Parent;  // Empty for root advancements
	bool HasDisplay = false;
	cAdvancementDisplay Display;
	std::vector<AString> Criteria;
	std::vector<std::vector<AString>> Requirements;  // Empty means "all criteria are required"
};





/** The progress a player has made on a single advancement. */
struct cAdvancementProgress
{
	AString ID;

	/** Maps an achieved criterion name to the time it was achieved, in milliseconds since the Unix epoch. */
	std::map<AString, Int64> AchievedCriteria;
};





/** A complete update carried by one Advancements packet. */
struct cAdvancementUpdate
{
	bool Reset = false;
	std::vector<cAdvancement> Advancements;
	std::vector<AString> Removals;
	std::vector<cAdvancementProgress> Progress;
};
