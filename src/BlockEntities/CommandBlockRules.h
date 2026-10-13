// CommandBlockRules.h

// Declares the pure type / metadata / activation rules of the 1.12.2 command block, so that they can be
// unit-tested without constructing a world.
// Behavior spec: "vanilla-1.12.2-command-blocks.md" in the specs folder, sections 2.1, 2.4, 2.5.
// Primary sources:
//  - https://minecraft.wiki/w/Command_Block?oldid=1150335 (1.9-1.12 semantics, block entity id)
//  - https://minecraft.wiki/w/Java_Edition_1.9 (type semantics: impulse / chain / repeat)
//  - the metadata encoding was verified against a 1.12.2 client save + F3 (see spec section 2.4)

#pragma once

#include <string_view>

#include "../BlockType.h"
#include "../Vector3.h"





/** The three 1.9+ command block types; each type has its own block ID. */
enum class eCommandBlockType
{
	Impulse,
	Chain,
	Repeat,
};





/** Returns the command block type represented by the given block ID.
Block IDs documented in BlockType.h (impulse = 137, repeating = 210, chain = 211); a non-command block
falls back to Impulse, which matches "no special behaviour". */
inline eCommandBlockType CommandBlockTypeFromBlockType(BLOCKTYPE a_BlockType)
{
	switch (a_BlockType)
	{
		case E_BLOCK_CHAIN_COMMAND_BLOCK:     return eCommandBlockType::Chain;
		case E_BLOCK_REPEATING_COMMAND_BLOCK: return eCommandBlockType::Repeat;
		default:                              return eCommandBlockType::Impulse;
	}
}





/** Returns true if the block ID is one of the three command block types. */
inline bool IsCommandBlockType(BLOCKTYPE a_BlockType)
{
	return (
		(a_BlockType == E_BLOCK_COMMAND_BLOCK) ||
		(a_BlockType == E_BLOCK_CHAIN_COMMAND_BLOCK) ||
		(a_BlockType == E_BLOCK_REPEATING_COMMAND_BLOCK)
	);
}





/** Returns the block ID that represents the given command block type. */
inline BLOCKTYPE CommandBlockBlockTypeFromType(eCommandBlockType a_Type)
{
	switch (a_Type)
	{
		case eCommandBlockType::Chain:  return E_BLOCK_CHAIN_COMMAND_BLOCK;
		case eCommandBlockType::Repeat: return E_BLOCK_REPEATING_COMMAND_BLOCK;
		default:                        return E_BLOCK_COMMAND_BLOCK;
	}
}





/** Returns the command block type selected by the "Mode" field of the 1.9+ MC|AutoCmd packet.
The field is a string enum "SEQUENCE" / "AUTO" / "REDSTONE" (wiki.vg Plugin channels);
the mapping was verified against a live 1.12.2 client (changing the type to chain sent "SEQUENCE",
to repeating sent "AUTO", so "REDSTONE" is the impulse default). */
inline eCommandBlockType CommandBlockTypeFromMode(std::string_view a_Mode)
{
	if (a_Mode == "SEQUENCE")
	{
		return eCommandBlockType::Chain;
	}
	if (a_Mode == "AUTO")
	{
		return eCommandBlockType::Repeat;
	}
	return eCommandBlockType::Impulse;
}





/** Returns the facing encoded in the low three metadata bits (0-5).
The encoding is the same one pistons use (cBlockPistonHandler::MetaDataToDirection) and was verified
against a 1.12.2 client save: 0 = down, 1 = up, 2 = north, 3 = south, 4 = west, 5 = east. */
inline NIBBLETYPE CommandBlockFacingFromMeta(NIBBLETYPE a_Meta)
{
	return (a_Meta & 0x7);
}





/** Returns true if the conditional bit (0x8) is set. */
inline bool CommandBlockIsConditional(NIBBLETYPE a_Meta)
{
	return ((a_Meta & 0x8) != 0);
}





/** Returns the unit offset towards which a block with the given metadata faces. */
inline Vector3i CommandBlockFacingOffset(NIBBLETYPE a_Meta)
{
	switch (CommandBlockFacingFromMeta(a_Meta))
	{
		case 0: return { 0, -1,  0 };  // Down
		case 1: return { 0,  1,  0 };  // Up
		case 2: return { 0,  0, -1 };  // North
		case 3: return { 0,  0,  1 };  // South
		case 4: return { -1, 0,  0 };  // West
		default: return { 1, 0,  0 };  // East
	}
}





/** Chain command blocks default to "Always Active"; impulse and repeat blocks to "Needs Redstone". */
inline bool CommandBlockDefaultAuto(eCommandBlockType a_Type)
{
	return (a_Type == eCommandBlockType::Chain);
}





/** Returns whether the block runs its command on this activation.
"Activated" already folds in the redstone signal and the "Always Active" (auto) setting; a conditional
block additionally requires the block behind it (opposite the facing direction) to have succeeded. */
inline bool CommandBlockShouldExecute(bool a_Activated, bool a_Conditional, bool a_BehindSucceeded)
{
	if (!a_Activated)
	{
		return false;
	}
	return (!a_Conditional || a_BehindSucceeded);
}





/** Clamps a command's success count to the 0-15 signal range a redstone comparator can carry. */
inline NIBBLETYPE CommandBlockComparatorSignal(int a_SuccessCount)
{
	if (a_SuccessCount <= 0)
	{
		return 0;
	}
	if (a_SuccessCount >= 15)
	{
		return 15;
	}
	return static_cast<NIBBLETYPE>(a_SuccessCount);
}
