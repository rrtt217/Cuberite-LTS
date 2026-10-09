// EndermanBlockRules.h

// Declares the pure geometry and block list of the enderman's block-carrying behaviour,
// so that they can be unit-tested without constructing a world.
// Behavior spec: "vanilla-1.12.2-enderman.md" in the specs folder, section 3.7.
// Primary sources: https://minecraft.wiki/w/Enderman#Moving_blocks and
// https://zh.minecraft.wiki/w/%E6%9C%AB%E5%BD%B1%E4%BA%BA (section "Moving blocks").

#pragma once

#include "../BlockInfo.h"
#include "../BlockType.h"





/** Every tick an enderman without a carried block has a 1/20 chance to try to pick one up (spec 3.7,
wiki "Moving blocks": "Every tick, an enderman has a 1/20 (5%) chance to select a random block"). */
static constexpr int ENDERMAN_PICKUP_CHANCE_DENOMINATOR = 20;

/** Every tick an enderman carrying a block has a 1/2000 chance to try to place it (spec 3.7,
wiki "Moving blocks": "a 1/2000 (0.05%) chance every tick"). */
static constexpr int ENDERMAN_PLACE_CHANCE_DENOMINATOR = 2000;

/* The exact block offsets of the even-sized regions are not given by any allowed source, only their sizes
(spec 3.7, marked [needs-check]).  We take the lower half of the covered span, starting at the block the
enderman's feet are in for the vertical axis - the same convention for both regions. */

/** The pickup region is 4x3x4 horizontally centered on the enderman (spec 3.7): the horizontal offsets
cover 4 blocks, the two blocks towards -X/-Z and the two from the feet block towards +X/+Z. */
static constexpr int ENDERMAN_PICKUP_MIN_HORIZONTAL_OFFSET = -2;
static constexpr int ENDERMAN_PICKUP_MAX_HORIZONTAL_OFFSET = 1;

/** The pickup region vertically encompasses the enderman itself (spec 3.7): the three blocks its 2.9-tall
body occupies, i.e. the feet block and the two above it.  This is also why an enderman cannot pick up
blocks from a completely flat floor - the floor block below its feet is never sampled (wiki, "Moving
blocks", and the Chinese wiki "向上 4x3x4"). */
static constexpr int ENDERMAN_PICKUP_MIN_VERTICAL_OFFSET = 0;
static constexpr int ENDERMAN_PICKUP_MAX_VERTICAL_OFFSET = 2;

/** The placement region is 2x2x2 horizontally centered on the enderman and at its own level (spec 3.7):
the horizontal offsets cover 2 blocks, the feet block and the one towards -X/-Z. */
static constexpr int ENDERMAN_PLACE_MIN_HORIZONTAL_OFFSET = -1;
static constexpr int ENDERMAN_PLACE_MAX_HORIZONTAL_OFFSET = 0;

/** The placement region's vertical offsets cover two blocks starting at the enderman's own level
(spec 3.7).  The upper candidate almost always fails the "full block below" check, so in practice the
enderman places its block beside itself at its own feet level. */
static constexpr int ENDERMAN_PLACE_MIN_VERTICAL_OFFSET = 0;
static constexpr int ENDERMAN_PLACE_MAX_VERTICAL_OFFSET = 1;





/** Returns whether the block is on the 1.12.2 enderman "holdable" list (spec 3.7).
The current wiki list minus its 1.13+ additions, plus netherrack (added in 1.10 / 16w20a, removed in
1.16 / 20w07a):
grass block, dirt / coarse dirt / podzol, sand / red sand, gravel, clay, mushrooms, small flowers,
pumpkins (the carved pumpkin is not a separate block before 1.13), melons, TNT, cacti, mycelium, netherrack.
The metadata is only checked for the blocks that share theirs with variants that must not be picked up. */
inline bool IsEndermanHoldableBlock(BLOCKTYPE a_BlockType, NIBBLETYPE a_BlockMeta)
{
	switch (a_BlockType)
	{
		case E_BLOCK_GRASS:
		case E_BLOCK_SAND:  // Normal and red sand
		case E_BLOCK_GRAVEL:
		case E_BLOCK_CLAY:
		case E_BLOCK_BROWN_MUSHROOM:
		case E_BLOCK_RED_MUSHROOM:
		case E_BLOCK_PUMPKIN:
		case E_BLOCK_MELON:
		case E_BLOCK_TNT:
		case E_BLOCK_CACTUS:
		case E_BLOCK_MYCELIUM:
		case E_BLOCK_NETHERRACK:
		case E_BLOCK_DANDELION:
		{
			return true;
		}
		case E_BLOCK_DIRT:
		{
			// Dirt, coarse dirt and podzol share one block:
			return (
				(a_BlockMeta == E_META_DIRT_NORMAL) ||
				(a_BlockMeta == E_META_DIRT_COARSE) ||
				(a_BlockMeta == E_META_DIRT_PODZOL)
			);
		}
		case E_BLOCK_FLOWER:
		{
			// Small flowers only; the tall double plants are not holdable:
			return (a_BlockMeta <= E_META_FLOWER_OXEYE_DAISY);
		}
		default:
		{
			return false;
		}
	}
}





/** Returns whether a carried block may be placed at a position whose block is a_Target and whose block
below is a_Below (spec 3.7): the target must be air and the supporting block must be a full block.
The Chinese wiki additionally excludes bedrock there, but that is a 1.16.2 change (pre1: "Endermen no
longer place their held blocks onto bedrock blocks") and thus not part of the 1.12.2 behaviour. */
inline bool EndermanCanPlaceBlockAt(BLOCKTYPE a_Target, BLOCKTYPE a_Below)
{
	return ((a_Target == E_BLOCK_AIR) && cBlockInfo::FullyOccupiesVoxel(a_Below));
}
