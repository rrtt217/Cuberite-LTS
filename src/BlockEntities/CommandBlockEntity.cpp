
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
	m_Result = a_Result;
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
	UNUSED(a_Chunk);

	const auto Type = CommandBlockTypeFromBlockType(GetBlockType());
	if (Type == eCommandBlockType::Repeat)
	{
		// Repeating blocks run every tick while activated ("Needs Redstone" or "Always Active"):
		if (!m_Powered && !m_Auto)
		{
			return false;
		}

		Execute();
		return true;
	}

	// Impulse and chain blocks run once per activation:
	if (!m_ShouldExecute)
	{
		return false;
	}

	m_ShouldExecute = false;
	Execute();
	return true;
}





void cCommandBlockEntity::SendTo(cClientHandle & a_Client)
{
	a_Client.SendUpdateBlockEntity(*this);
}





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
		m_World->DoWithBlockEntityAt(m_Pos - CommandBlockFacingOffset(Meta), [&](cBlockEntity & a_BlockEntity)
		{
			const auto BehindType = a_BlockEntity.GetBlockType();
			if (
				(BehindType == E_BLOCK_COMMAND_BLOCK) ||
				(BehindType == E_BLOCK_CHAIN_COMMAND_BLOCK) ||
				(BehindType == E_BLOCK_REPEATING_COMMAND_BLOCK)
			)
			{
				BehindSucceeded = (static_cast<cCommandBlockEntity &>(a_BlockEntity).GetResult() > 0);
			}
			return true;
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

	class CommandBlockOutCb :
		public cLogCommandDeleteSelfOutputCallback
	{
		cCommandBlockEntity * m_CmdBlock;

	public:
		CommandBlockOutCb(cCommandBlockEntity * a_CmdBlock) : m_CmdBlock(a_CmdBlock) {}

		virtual void Out(const AString & a_Text) override
		{
			// Only store the output when the block is tracking it (TrackOutput):
			if (m_CmdBlock->GetTrackOutput())
			{
				m_CmdBlock->SetLastOutput(cClientHandle::FormatChatPrefix(m_CmdBlock->GetWorld()->ShouldUseChatPrefixes(), "SUCCESS", cChatColor::Green, cChatColor::White) + a_Text);
			}
		}

		virtual void CommandExecuted(bool a_Success) override
		{
			// Deviation (b): the success count is approximated as 1 on success and 0 on failure
			// (spec section 2.2):
			m_CmdBlock->SetResult(a_Success ? 1 : 0);
		}
	};

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
		LOGD("cCommandBlockEntity: Executing command %s", m_Command.c_str());

		// A command block executes at its own block position; the exact offset is an approximation
		// until verified against 1.12.2 (spec: speculated / to be verified). It has no executor
		// (vanilla has no "@s" for a command block).
		const cCommandOrigin Origin(
			GetWorld(),
			Vector3d(m_Pos.x + 0.5, m_Pos.y + 1, m_Pos.z + 0.5),
			nullptr
		);
		Server->QueueExecuteConsoleCommand(RealCommand, *new CommandBlockOutCb(this), Origin);

		// Vanilla notifies the chain command block this block faces; the chain block then runs on its
		// next tick here (vanilla runs the whole chain in the same tick - see the spec):
		TriggerFacingChainBlock();
	}
	else
	{
		SetLastOutput(cClientHandle::FormatChatPrefix(GetWorld()->ShouldUseChatPrefixes(), "FAILURE", cChatColor::Rose, cChatColor::White) + "Adminstration commands can not be executed");
		LOGD("cCommandBlockEntity: Prevented execution of administration command %s", m_Command.c_str());
	}

	// TODO 2014-01-18 xdot: Update the signal strength.
	m_Result = 0;
}





void cCommandBlockEntity::TriggerFacingChainBlock()
{
	if (m_World == nullptr)
	{
		return;
	}

	// Read the current metadata - it can change through the GUI:
	BLOCKTYPE Type;
	NIBBLETYPE Meta;
	if (!m_World->GetBlockTypeMeta(m_Pos, Type, Meta))
	{
		return;
	}

	const Vector3i Target = m_Pos + CommandBlockFacingOffset(Meta);
	m_World->DoWithBlockEntityAt(Target, [](cBlockEntity & a_BlockEntity)
	{
		if (a_BlockEntity.GetBlockType() != E_BLOCK_CHAIN_COMMAND_BLOCK)
		{
			return false;
		}

		static_cast<cCommandBlockEntity &>(a_BlockEntity).Activate();
		return true;
	});
}




