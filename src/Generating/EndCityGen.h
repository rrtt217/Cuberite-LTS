// EndCityGen.h

// Declares the cEndCityGen class representing the End City finisher generator

/*
Generates End cities on the End's outer islands. The cities are placed on a 20-chunk grid, with the
origin chunk of each city lying in the [0 .. 8] range within its grid cell, so that the origin block
coordinates are congruent to [0 .. 128] modulo 320 (see the End City specification for the source).

Room geometry is currently an approximation of the vanilla one; the exact per-block blueprints are
tracked as a follow-up in the specification.
*/





#pragma once

#include "GridStructGen.h"





// fwd:
class cTerrainHeightGen;





class cEndCityGen:
	public cGridStructGen
{
	using Super = cGridStructGen;

public:

	cEndCityGen(int a_Seed, cTerrainHeightGen & a_HeightGen);

	/** Returns the origin block coords of the city generated for the grid cell whose minimum corner is
	(a_GridX, a_GridZ). Exposed so that the 20-chunk grid rule can be unit-tested. */
	static Vector3i GetCellOrigin(int a_Seed, int a_GridX, int a_GridZ);

protected:

	class cEndCity;

	/** The height generator used to place the city onto the terrain and to test the ground flatness. */
	cTerrainHeightGen & m_HeightGen;


	// cGridStructGen override:
	virtual cStructurePtr CreateStructure(int a_GridX, int a_GridZ, int a_OriginX, int a_OriginZ) override;
} ;




