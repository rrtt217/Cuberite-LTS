// CommandOrigin.h

// Declares the cCommandOrigin class that describes who issued a command and from where




#pragma once

#include "Vector3.h"





class cPlayer;
class cWorld;





// tolua_begin

/** Describes the execution context of a command: the world it runs in, the position it originates from
and the player that issued it (if any).
Vanilla 1.12.2 resolves relative coordinates and position-based target selectors against this origin.
The console and RCON use the default world, command blocks use their own block position and neither has
an executor. */
class cCommandOrigin
{
public:
	cCommandOrigin(void):
		m_World(nullptr),
		m_Position(),
		m_Player(nullptr)
	{
	}

	cCommandOrigin(cWorld * a_World, const Vector3d & a_Position, cPlayer * a_Player):
		m_World(a_World),
		m_Position(a_Position),
		m_Player(a_Player)
	{
	}

	/** Returns the world the command originates from, or nullptr if it is not known. */
	cWorld * GetWorld(void) const { return m_World; }

	/** Returns the position the command originates from. */
	Vector3d GetPosition(void) const { return m_Position; }

	/** Returns the player that issued the command, or nullptr for the console, RCON and command blocks. */
	cPlayer * GetPlayer(void) const { return m_Player; }

	/** Returns true if the command was issued by a player. */
	bool HasPlayer(void) const { return (m_Player != nullptr); }

private:
	/** The world the command originates from; nullptr if unknown. */
	cWorld * m_World;

	/** The position the command originates from. */
	Vector3d m_Position;

	/** The player that issued the command; nullptr for the console, RCON and command blocks. */
	cPlayer * m_Player;
} ;

// tolua_end
