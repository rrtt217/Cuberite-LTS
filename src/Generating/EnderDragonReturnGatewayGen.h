// EnderDragonReturnGatewayGen.h

#pragma once

#include "FinishGen.h"





/** Generates the natural return gateways (end_gateway_return) scattered through the End's outer islands.
Unlike the gateways spawned by the dragon fight, these teleport entities back to the End platform. */
class cEnderDragonReturnGatewayGen :
	public cFinishGen
{
public:
	/** The vanilla rarity of an end_gateway_return: one attempt per this many generated chunks. */
	static constexpr int DEFAULT_CHANCE = 700;

	cEnderDragonReturnGatewayGen(int a_Seed, int a_Chance = DEFAULT_CHANCE);

protected:
	int m_Seed;
	int m_Chance;

	// cFinishGen override:
	void GenFinish(cChunkDesc & a_ChunkDesc) override;
};
