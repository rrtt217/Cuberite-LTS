// CommandBlockNBT.h

// Declares the 1.12.2 command block block-entity data and its NBT (de)serialisation.
// Keeping the field mapping separate from the chunk serializer / Anvil loader makes it unit-testable.
// Behavior spec: "vanilla-1.12.2-command-blocks.md" in the specs folder, section 2.7.

#pragma once

#include "../Globals.h"
#include "FastNBT.h"





/** The 1.12.2 command block block-entity fields, excluding the block entity id and position.
The defaults are the vanilla defaults (spec section 2.7). */
struct sCommandBlockData
{
	AString m_Command;
	Int32   m_SuccessCount = 0;
	AString m_LastOutput;
	bool    m_TrackOutput = true;
	bool    m_Powered = false;
	bool    m_Auto = false;
	bool    m_ConditionMet = true;
	bool    m_UpdateLastExecution = true;
	Int64   m_LastExecution = 0;
} ;





/** Writes the command block fields into the current compound of a_Writer. */
void WriteCommandBlockNBT(cFastNBTWriter & a_Writer, const sCommandBlockData & a_Data);

/** Reads the command block fields from the compound at a_TagIdx into a_Data.
Fields that are not present keep their current value. */
void ReadCommandBlockNBT(const cParsedNBT & a_NBT, int a_TagIdx, sCommandBlockData & a_Data);
