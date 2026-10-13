
// CommandBlockEntity.cpp

// Implements the cCommandBlockEntity class representing a single command block in the world

#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules
#include "CommandBlockEntity.h"
#include "CommandBlockRules.h"

#include "../CommandOutput.h"
#include "../Root.h"
#include "../Server.h"  // ExecuteConsoleCommand()
#include "../ChatColor.h"
#include "../World.h"
#include "../Chunk.h"
#include "../ClientHandle.h"
#include "../Entities/Player.h"





cCommandBlockEntity::cCommandBlockEntity(BLOCKTYPE a_BlockType, NIBBLETYPE a_BlockMeta, Vector3i a_Pos, cWorld * a_World):
	Super(a_BlockType, a_BlockMeta, a_Pos, a_World),
	m_ShouldExecute(false),
	m_Result(0),
	m_TrackOutput(true),
	m_Auto(false),
	m_Powered(false),
	m_ConditionMet(true),
	m_UpdateLastExecution(true),
	m_LastExecution(0)
{
	ASSERT(
		(a_BlockType == E_BLOCK_COMMAND_BLOCK) ||
		(a_BlockType == E_BLOCK_CHAIN_COMMAND_BLOCK) ||
		(a_BlockType == E_BLOCK_REPEATING_COMMAND_BLOCK)
	);
}





bool cCommandBlockEntity::UsedBy(cPlayer * a_Player)
{
	// A 1.9+ client opens the command block GUI locally on use, but keeps its "Done" button disabled
	// until it receives this block entity's data (MC-87209). Vanilla answers the use with an
	// Update Block Entity (0x09, Action 2); without it the GUI can never be saved:
	if (a_Player != nullptr)
	{
		auto ClientHandle = a_Player->GetClientHandle();
		if (ClientHandle != nullptr)
		{
			ClientHandle->SendUpdateBlockEntity(*this);
		}
	}
	return true;
}





void cCommandBlockEntity::SetCommand(const AString & a_Cmd)
{
	m_Command = a_Cmd;
}





void cCommandBlockEntity::SetLastOutput(const AString & a_LastOut)
{
	m_LastOutput = a_LastOut;
}





void cCommandBlockEntity::SetResult(const NIBBLETYPE a_Result)
{
	SetResultCount(a_Result);
}





void cCommandBlockEntity::SetResultCount(int a_ResultCount)
{
	m_Result = a_ResultCount;
}





void cCommandBlockEntity::SetTrackOutput(bool a_TrackOutput)
{
	m_TrackOutput = a_TrackOutput;
}





bool cCommandBlockEntity::GetTrackOutput(void) const
{
	return m_TrackOutput;
}





void cCommandBlockEntity::SetAuto(bool a_Auto)
{
	m_Auto = a_Auto;
}





bool cCommandBlockEntity::IsAuto(void) const
{
	return m_Auto;
}





void cCommandBlockEntity::SetPowered(bool a_Powered)
{
	m_Powered = a_Powered;
}





bool cCommandBlockEntity::IsPowered(void) const
{
	return m_Powered;
}





void cCommandBlockEntity::SetConditionMet(bool a_ConditionMet)
{
	m_ConditionMet = a_ConditionMet;
}





bool cCommandBlockEntity::IsConditionMet(void) const
{
	return m_ConditionMet;
}





void cCommandBlockEntity::SetUpdateLastExecution(bool a_UpdateLastExecution)
{
	m_UpdateLastExecution = a_UpdateLastExecution;
}





bool cCommandBlockEntity::GetUpdateLastExecution(void) const
{
	return m_UpdateLastExecution;
}





void cCommandBlockEntity::SetLastExecution(Int64 a_LastExecution)
{
	m_LastExecution = a_LastExecution;
}





Int64 cCommandBlockEntity::GetLastExecution(void) const
{
	return m_LastExecution;
}





const AString & cCommandBlockEntity::GetCommand(void) const
{
	return m_Command;
}





const AString & cCommandBlockEntity::GetLastOutput(void) const
{
	return m_LastOutput;
}





NIBBLETYPE cCommandBlockEntity::GetResult(void) const
{
	return static_cast<NIBBLETYPE>(GetResultCount());
}





int cCommandBlockEntity::GetResultCount(void) const
{
	return m_Result;
}





void cCommandBlockEntity::Activate(void)
{
	m_ShouldExecute = true;
}





void cCommandBlockEntity::CopyFrom(const cBlockEntity & a_Src)
{
	Super::CopyFrom(a_Src);
	auto & src = static_cast<const cCommandBlockEntity &>(a_Src);
	m_Command = src.m_Command;
	m_LastOutput = src.m_LastOutput;
	m_Result = src.m_Result;
	m_ShouldExecute = src.m_ShouldExecute;
	m_TrackOutput = src.m_TrackOutput;
	m_Auto = src.m_Auto;
	m_Powered = src.m_Powered;
	m_ConditionMet = src.m_ConditionMet;
	m_UpdateLastExecution = src.m_UpdateLastExecution;
	m_LastExecution = src.m_LastExecution;
}





bool cCommandBlockEntity::Tick(std::chrono::milliseconds a_Dt, cChunk & a_Chunk)
{
	UNUSED(a_Dt);

	const auto Type = CommandBlockTypeFromBlockType(GetBlockType());
	if (Type == eCommandBlockType::Repeat)
	{
		// Repeating blocks run every tick while activated ("Needs Redstone" or "Always Active"):
		if (!m_Powered && !m_Auto)
		{
			return false;
		}
	}
	// Impulse and chain blocks run once per activation:
	else if (!m_ShouldExecute)
	{
		return false;
	}
	else
	{
		m_ShouldExecute = false;
	}

	// Do not run the command here: this is called while cChunk::Tick() iterates the chunk's block
	// entities, and the command may add or remove them. Register with the chunk instead; it runs the
	// block right after that loop and before the redstone simulation, so a comparator reading the
	// success count later in this same tick sees the new value (spec: strict C, section 3.1):
	a_Chunk.QueueCommandBlockExecution(GetRelPos());
	return true;
}





void cCommandBlockEntity::SendTo(cClientHandle & a_Client)
{
	a_Client.SendUpdateBlockEntity(*this);
}





namespace
{

/** Runs a_Callback on the command block at a_Pos, if there is one there.
Unlike cWorld::DoWithBlockEntityAt(), this does not queue the block entity for a resend to every client:
a command block's output and success count change on every execution, so re-broadcasting them makes an
open command block GUI reset its options on every tick and the player can never save it. */
template <typename Fn>
bool DoWithCommandBlockAt(cWorld & a_World, Vector3i a_Pos, Fn && a_Callback)
{
	return a_World.DoWithChunkAt(a_Pos, [&](cChunk & a_Chunk)
	{
		const auto BlockEntity = a_Chunk.GetBlockEntity(a_Pos);
		if ((BlockEntity == nullptr) || !IsCommandBlockType(BlockEntity->GetBlockType()))
		{
			return false;
		}
		a_Callback(static_cast<cCommandBlockEntity &>(*BlockEntity));
		return true;
	});
}





class CommandBlockOutCb :
	public cLogCommandOutputCallback
{
	cWorld * m_World;
	Vector3i m_Pos;

	/** Runs a_Callback on the command block at our position, if it is still a command block.
	The command may destroy its own block, so never keep the pointer. */
	template <typename Fn>
	void DoWithBlock(Fn && a_Callback)
	{
		DoWithCommandBlockAt(*m_World, m_Pos, a_Callback);
	}

public:
	CommandBlockOutCb(cWorld * a_World, Vector3i a_Pos) :
		m_World(a_World),
		m_Pos(a_Pos)
	{
	}

	virtual void Out(const AString & a_Text) override
	{
		DoWithBlock([&](cCommandBlockEntity & a_CmdBlock)
		{
			// Only store the output when the block is tracking it (TrackOutput):
			if (a_CmdBlock.GetTrackOutput())
			{
				a_CmdBlock.SetLastOutput(cClientHandle::FormatChatPrefix(m_World->ShouldUseChatPrefixes(), "SUCCESS", cChatColor::Green, cChatColor::White) + a_Text);
			}
		});
	}

	virtual void CommandExecuted(int a_SuccessCount) override
	{
		// Vanilla Java 1.12.2 stores the command's affected-object count here, and the
		// comparator reflects the last executed command (spec sections 2.2 and 2.13):
		DoWithBlock([&](cCommandBlockEntity & a_CmdBlock)
		{
			a_CmdBlock.SetResultCount(a_SuccessCount);
		});
	}
};


}  // anonymous namespace





void cCommandBlockEntity::Execute()
{
	ASSERT(m_World != nullptr);  // Execute should not be called before the command block is attached to a world

	if (!m_World->AreCommandBlocksEnabled())
	{
		return;
	}

	// Conditional blocks only run when the block behind them (opposite the facing direction) is a
	// command block whose last execution succeeded (spec section 2.2). The success count itself is
	// the accepted "success 1, failure 0" approximation (deviation (b)):
	BLOCKTYPE MetaType;
	NIBBLETYPE Meta;
	if (m_World->GetBlockTypeMeta(m_Pos, MetaType, Meta) && CommandBlockIsConditional(Meta))
	{
		bool BehindSucceeded = false;
		DoWithCommandBlockAt(*m_World, m_Pos - CommandBlockFacingOffset(Meta), [&](cCommandBlockEntity & a_Behind)
		{
			BehindSucceeded = (a_Behind.GetResultCount() > 0);
		});

		if (!CommandBlockShouldExecute(true, true, BehindSucceeded))
		{
			SetConditionMet(false);
			m_Result = 0;
			return;
		}
		SetConditionMet(true);
	}

	if (m_Command.empty())
	{
		return;
	}


	AString RealCommand = m_Command;

	// Remove leading slash if it exists, since console commands don't use them
	if (RealCommand[0] == '/')
	{
		RealCommand = RealCommand.substr(1, RealCommand.length());
	}

	// Administrator commands are not executable by command blocks:
	if (
		(RealCommand != "stop") &&
		(RealCommand != "restart") &&
		(RealCommand != "kick") &&
		(RealCommand != "ban") &&
		(RealCommand != "ipban")
	)
	{
		cServer * Server = cRoot::Get()->GetServer();
		cWorld * World = GetWorld();
		const Vector3i Pos = m_Pos;  // Capture before running: the command may destroy this block.
		LOGD("cCommandBlockEntity: Executing command %s", m_Command.c_str());

		// A command block executes at its own block position; the exact offset is an approximation
		// until verified against 1.12.2 (spec: speculated / to be verified). It has no executor
		// (vanilla has no "@s" for a command block).
		const cCommandOrigin Origin(
			GetWorld(),
			Vector3d(m_Pos.x + 0.5, m_Pos.y + 1, m_Pos.z + 0.5),
			nullptr
		);

		// Reset the success count; the command reports the real one through the output callback:
		m_Result = 0;

		// Run the command right here. cChunk::Tick calls this from within cChunkMap::Tick, so this is
		// the world thread with the chunkmap lock already held: the only lock order is world -> plugin,
		// and a comparator reads the fresh success count later in this same tick (spec section 3.1):
		CommandBlockOutCb Output(World, Pos);
		Server->ExecuteConsoleCommand(RealCommand, Output, Origin);

		// Vanilla notifies the chain command block this block faces; the chain block then runs on its
		// next tick here (vanilla runs the whole chain in the same tick - see the spec). Position-based,
		// because the command may have destroyed this block:
		TriggerFacingChainBlockAt(World, Pos);
	}
	else
	{
		SetLastOutput(cClientHandle::FormatChatPrefix(GetWorld()->ShouldUseChatPrefixes(), "FAILURE", cChatColor::Rose, cChatColor::White) + "Adminstration commands can not be executed");
		LOGD("cCommandBlockEntity: Prevented execution of administration command %s", m_Command.c_str());

		// A command that cannot run stores no success (spec section 2.2):
		m_Result = 0;
	}
}





void cCommandBlockEntity::TriggerFacingChainBlockAt(cWorld * a_World, Vector3i a_Pos)
{
	if (a_World == nullptr)
	{
		return;
	}

	// Read the current metadata - it can change through the GUI:
	BLOCKTYPE Type;
	NIBBLETYPE Meta;
	if (!a_World->GetBlockTypeMeta(a_Pos, Type, Meta))
	{
		return;
	}

	const Vector3i Target = a_Pos + CommandBlockFacingOffset(Meta);
	DoWithCommandBlockAt(*a_World, Target, [](cCommandBlockEntity & a_BlockEntity)
	{
		if (a_BlockEntity.GetBlockType() != E_BLOCK_CHAIN_COMMAND_BLOCK)
		{
			return;
		}

		a_BlockEntity.Activate();
	});
}




