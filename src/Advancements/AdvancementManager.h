// AdvancementManager.h

// Declares the cAdvancementManager class that loads advancement definitions and builds the updates
// sent to 1.12+ clients.

#pragma once

#include "Advancement.h"

#include <map>
#include <set>





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

	/** Loaded advancements, keyed by identifier. A std::map keeps the join update deterministic. */
	std::map<AString, cAdvancement> m_Advancements;

	/** Direct children of each advancement. */
	std::map<AString, AStringVector> m_Children;

	/** Adds one parsed JSON file to the manager. */
	void AddFile(const AString & a_FileName, const AString & a_RelativePath);

	/** Drops advancements with missing parents, parent cycles or invalid requirements. */
	void Validate(void);

	/** Rebuilds m_Children from the loaded advancements. */
	void BuildChildren(void);
};
