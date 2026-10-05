
#pragma once

/*
https://minecraft.wiki/w/End_spike
https://minecraft.wiki/w/End_Crystal
https://minecraft.wiki/w/Ender_Dragon
*/

#include "FinishGen.h"

class cEnderDragonFightStructuresGen :
	public cFinishGen
{
public:
	struct sTowerProperties
	{
		Vector3i m_Pos;
		int m_Height;
		int m_Radius;
		bool m_HasCage;
	};

	cEnderDragonFightStructuresGen(int a_Seed);
	void Init(const AString & a_TowerProperties, int a_Radius);

	/** Returns every generated tower exactly once, in the order they were created. */
	std::vector<sTowerProperties> GetTowers(void) const;

	/** Offsets of the iron bar cage blocks relative to a tower's top, used when a spike is regenerated. */
	static const std::array<Vector3i, 48> m_CagePos;

	/** Offsets that must be cleared inside a tower's cage when a spike is regenerated. */
	static const std::array<Vector3i, 26> m_CageAir;

protected:
	cNoise m_Noise;
	std::map<cChunkCoords, std::vector<sTowerProperties>> m_TowerPos;
	cBlockArea m_Fountain;

	int m_MinX = -1, m_MaxX = 1, m_MinZ = -1, m_MaxZ = 1;

	void GenFinish(cChunkDesc &a_ChunkDesc) override;
	void PlaceTower(cChunkDesc & a_ChunkDesc, const sTowerProperties & a_TowerProperties);
};
