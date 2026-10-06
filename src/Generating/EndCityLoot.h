// EndCityLoot.h

// Declares the End City special-content hook and the loot table that installs it

#pragma once

#include <vector>

class cChunkDesc;

/** The kind of a generated End City special content. */
enum eEndCityContentKind
{
	ecctChest,
	ecctBrewingStand,
	ecctItemFrame,
	ecctMobHead,
} ;

/** A generated End City special content, in world coordinates. */
struct sEndCityContent
{
	eEndCityContentKind m_Kind;
	Vector3i m_Pos;
	eBlockFace m_Face;

	/** Extra data, currently the mob head rotation for ecctMobHead. */
	int m_Data;
} ;

/** Fills the generated End City special contents that fall in a chunk. */
typedef void (*EndCityContentsFiller)(cChunkDesc & a_Chunk, const std::vector<sEndCityContent> & a_Contents);

/** Installs the filler; called by this module's static initialiser, so the generator stays free of
the item and entity systems. */
void SetEndCityContentsFiller(EndCityContentsFiller a_Filler);
