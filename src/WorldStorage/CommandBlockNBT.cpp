// CommandBlockNBT.cpp

// Implements the NBT (de)serialisation of the 1.12.2 command block block-entity fields.
// Behavior spec: "vanilla-1.12.2-command-blocks.md" in the specs folder, section 2.7.

#include "Globals.h"
#include "CommandBlockNBT.h"





void WriteCommandBlockNBT(cFastNBTWriter & a_Writer, const sCommandBlockData & a_Data)
{
	a_Writer.AddString("Command",   a_Data.m_Command);
	a_Writer.AddInt   ("SuccessCount", a_Data.m_SuccessCount);
	a_Writer.AddString("LastOutput", a_Data.m_LastOutput);
	a_Writer.AddByte  ("TrackOutput",           a_Data.m_TrackOutput ? 1 : 0);
	a_Writer.AddByte  ("powered",               a_Data.m_Powered ? 1 : 0);
	a_Writer.AddByte  ("auto",                  a_Data.m_Auto ? 1 : 0);
	a_Writer.AddByte  ("conditionMet",          a_Data.m_ConditionMet ? 1 : 0);
	a_Writer.AddByte  ("UpdateLastExecution",   a_Data.m_UpdateLastExecution ? 1 : 0);
	a_Writer.AddLong  ("LastExecution",         a_Data.m_LastExecution);
}





void ReadCommandBlockNBT(const cParsedNBT & a_NBT, int a_TagIdx, sCommandBlockData & a_Data)
{
	int Tag = a_NBT.FindChildByName(a_TagIdx, "Command");
	if (Tag >= 0)
	{
		a_Data.m_Command = a_NBT.GetString(Tag);
	}

	Tag = a_NBT.FindChildByName(a_TagIdx, "SuccessCount");
	if (Tag >= 0)
	{
		a_Data.m_SuccessCount = a_NBT.GetInt(Tag);
	}

	Tag = a_NBT.FindChildByName(a_TagIdx, "LastOutput");
	if (Tag >= 0)
	{
		a_Data.m_LastOutput = a_NBT.GetString(Tag);
	}

	Tag = a_NBT.FindChildByName(a_TagIdx, "TrackOutput");
	if (Tag >= 0)
	{
		a_Data.m_TrackOutput = (a_NBT.GetByte(Tag) != 0);
	}

	Tag = a_NBT.FindChildByName(a_TagIdx, "powered");
	if (Tag >= 0)
	{
		a_Data.m_Powered = (a_NBT.GetByte(Tag) != 0);
	}

	Tag = a_NBT.FindChildByName(a_TagIdx, "auto");
	if (Tag >= 0)
	{
		a_Data.m_Auto = (a_NBT.GetByte(Tag) != 0);
	}

	Tag = a_NBT.FindChildByName(a_TagIdx, "conditionMet");
	if (Tag >= 0)
	{
		a_Data.m_ConditionMet = (a_NBT.GetByte(Tag) != 0);
	}

	Tag = a_NBT.FindChildByName(a_TagIdx, "UpdateLastExecution");
	if (Tag >= 0)
	{
		a_Data.m_UpdateLastExecution = (a_NBT.GetByte(Tag) != 0);
	}

	Tag = a_NBT.FindChildByName(a_TagIdx, "LastExecution");
	if (Tag >= 0)
	{
		a_Data.m_LastExecution = a_NBT.GetLong(Tag);
	}
}
