// EndCityBlueprintData.h

// Declares the raw End City blueprints, transcribed from the Minecraft Wiki's layered blueprints.

/*
Sources (cleanroom allowlist):
- https://minecraft.wiki/w/End_City/Structure/Base
- https://minecraft.wiki/w/End_City/Structure/Small_Tower
- https://minecraft.wiki/w/End_City/Structure/Large_Tower
- https://minecraft.wiki/w/End_City/Structure/Small_Room
- https://minecraft.wiki/w/End_City/Structure/Large_Room
- https://minecraft.wiki/w/End_City/Structure/Loot_Room
- https://minecraft.wiki/w/End_City/Structure/Empty_Room
- https://minecraft.wiki/w/End_City/Structure/Bridge
- https://minecraft.wiki/w/End_City/Structure/Ship

Each blueprint is stored as one padded character grid per layer. Rows within a layer are separated by '|'
and are ordered along Z; the characters within a row are ordered along X. The char map maps each character
to a block name and an optional orientation suffix.
*/

#pragma once

#include "Globals.h"

/** A single layer of a blueprint. */
struct sEndCityBlueprintLayer
{
	/** Y coordinate of the layer, relative to the blueprint's lowest layer. */
	int m_Y;

	/** The layer's rows, separated by '|' and ordered along Z. */
	const char * m_Rows;
} ;

/** A single blueprint. */
struct sEndCityBlueprint
{
	/** The logical name. */
	const char * m_Name;

	/** The character-to-block map, entries separated by '|'. */
	const char * m_CharMap;

	/** The padded size of the grid. */
	int m_SizeX;
	int m_SizeZ;
	int m_Height;

	/** The layers. */
	const sEndCityBlueprintLayer * m_Layers;

	/** Keep the declared X and Z frame when building the piece, instead of cropping to the content.
	Needed when the content does not fill the frame and its frame coordinates matter. */
	bool m_KeepFrame = false;
} ;

/** All blueprints. */
extern const sEndCityBlueprint g_EndCityBlueprints[];

/** The number of blueprints. */
extern const int g_NumEndCityBlueprints;
