// EndChorusGen.h

#pragma once

#include "ComposableGenerator.h"





/** Generates the natural chorus trees of the End's outer islands (vanilla's End Highlands).
The trees come from the overridden vanilla feature "chorus_plant", which makes 0-4 attempts per chunk.
Only the composited heightmap generator is used, so that neighbouring chunks are not fully regenerated. */
class cEndChorusGen :
	public cFinishGen
{
public:
	/** Highest number of tree attempts in one chunk (vanilla "count": uniform 0-4). */
	static constexpr int MAX_TREE_ATTEMPTS = 4;

	/** Horizontal distance from the centre beyond which the outer islands (vanilla's End Highlands) lie. */
	static constexpr double MIN_DISTANCE = 1024.0;

	/** Highest surface Y that still counts as the void instead of land. */
	static constexpr int MIN_LAND_Y = 15;

	cEndChorusGen(int a_Seed, cTerrainHeightGen & a_HeightGen);

protected:
	int m_Seed;
	cTerrainHeightGen & m_HeightGen;

	// cFinishGen override:
	void GenFinish(cChunkDesc & a_ChunkDesc) override;

	/** Grows the trees rooted in a_Coords (whose heightmap is a_HeightMap) into a_Target. */
	void GenerateChunkTrees(const cChunkCoords & a_Coords, const cChunkDef::HeightMap & a_HeightMap, cChunkDesc & a_Target);

	/** Copies the blocks of a_Image that fall inside a_Target into air blocks. */
	static void ApplyImage(cChunkDesc & a_Target, const sSetBlockVector & a_Image);
};
