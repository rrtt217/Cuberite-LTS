#pragma once

#include "../World.h"





/** Generates the obsidian platform that entities arrive on when they enter the End.
It is (re)generated on every entry, just like in vanilla. */
class cEndPlatform
{
public:

	/** Where a player arrives on top of the platform. */
	static constexpr double PLAYER_SPAWN_X = 100.0;
	static constexpr double PLAYER_SPAWN_Y = 49.0;
	static constexpr double PLAYER_SPAWN_Z = 0.0;

	/** Where a non-player entity arrives on top of the platform. */
	static constexpr double ENTITY_SPAWN_X = 100.5;
	static constexpr double ENTITY_SPAWN_Y = 50.0;
	static constexpr double ENTITY_SPAWN_Z = 0.5;

	/** Places (or refreshes) the End's arrival platform in a_EndWorld. */
	static void Generate(cWorld * a_EndWorld);
};
