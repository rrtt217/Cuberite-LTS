// EndCityGen.cpp

// Implements the cEndCityGen class representing the End City finisher generator

#include "Globals.h"
#include "EndCityGen.h"
#include "ComposableGenerator.h"
#include "EndCityBlueprintData.h"
#include "Prefab.h"
#include "../BlockInfo.h"
#include "../StringUtils.h"
#include "EndCityLoot.h"
#include "../BlockEntities/ChestEntity.h"
#include "../Entities/ItemFrame.h"

#include <algorithm>
#include <memory>
#include <random>
#include <vector>





/** The grid cell size, in blocks. Vanilla places End cities once per 20 chunks. */
static constexpr int END_CITY_GRID_SIZE = 20 * cChunkDef::Width;

/** The maximum offset of the structure origin from the grid point, in blocks. */
static constexpr int END_CITY_MAX_OFFSET = 8 * cChunkDef::Width;

/** The maximum theoretical size of a city, in blocks. */
static constexpr int END_CITY_MAX_SIZE = 12 * cChunkDef::Width;

/** The maximum number of structures kept in the grid cache. */
static constexpr size_t END_CITY_MAX_CACHE = 64;

/** Number of chunk offsets an origin may take within its cell; vanilla uses chunks 0 .. 8. */
static constexpr int END_CITY_ORIGIN_CHUNK_RANGE = 9;

/** Horizontal distance from the centre beyond which the End's outer islands lie. */
static constexpr double END_CITY_MIN_DISTANCE = 1024.0;

/** Highest surface Y that still counts as the void instead of land. */
static constexpr int END_CITY_MIN_LAND_Y = 15;

/** Side length of the square footprint checked for flatness. */
static constexpr int END_CITY_FOOTPRINT = 13;

/** Largest height difference within the footprint that still counts as flat ground. */
static constexpr int END_CITY_MAX_HEIGHT_DIFFERENCE = 4;

/** The chance (1 in N) that a tower grows a bridge in a given direction. */
static constexpr int END_CITY_BRIDGE_DENOMINATOR = 2;

/** The chance (1 in N) that a bridge ends in an End ship. */
static constexpr int END_CITY_SHIP_DENOMINATOR = 8;

/** The chance (1 in N) that a two or three storey loot room carries a small tower on its roof. */
static constexpr int END_CITY_ROOM_TOWER_DENOMINATOR = 2;

/** The number of straight bridge segments, and the extra random range. */
static constexpr int END_CITY_BRIDGE_MIN_SEGMENTS = 2;
static constexpr int END_CITY_BRIDGE_EXTRA_SEGMENTS = 3;

/** How far the small tower's ladder shaft descends into the room below. */
static constexpr int END_CITY_SMALL_TOWER_LADDER_DEPTH = 3;

/** The width and depth of the base room's declared frame. Its content does not fill that frame, so
centring on the cropped bounding box would shift its internal spiral ladder and floor openings. */
static constexpr int END_CITY_BASE_FRAME = 18;

/** The base room's topmost ladder cell within that frame, and the small tower's ladder cell within its
own frame. Placing the tower so the two coincide continues the room's spiral ladder into the tower. */
static constexpr int END_CITY_BASE_LADDER_X = 10;
static constexpr int END_CITY_BASE_LADDER_Z = 9;
static constexpr int END_CITY_TOWER_LADDER_X = 3;
static constexpr int END_CITY_TOWER_LADDER_Z = 4;

/** The minimum fill percentage of a layer for it to count as the structural top when stacking pieces. */
static constexpr int END_CITY_STACK_MIN_PERCENT = 3;

/** The number of horizontal directions a tower can branch into. */
static constexpr int END_CITY_DIR_COUNT = 4;



/** The horizontal direction vectors used for branches. */
static const int END_CITY_DIR_X[END_CITY_DIR_COUNT] = {1, 0, -1, 0};
static const int END_CITY_DIR_Z[END_CITY_DIR_COUNT] = {0, 1, 0, -1};

/** Seed offsets used to decouple the layout randomness from the origin randomness. */
static constexpr int END_CITY_SEED_OFFSET_X = 7919;
static constexpr int END_CITY_SEED_OFFSET_Z = 104729;

/** End rod facing metadata. */
static constexpr NIBBLETYPE END_CITY_END_ROD_DOWN = 0;
static constexpr NIBBLETYPE END_CITY_END_ROD_UP = 1;
static constexpr NIBBLETYPE END_CITY_END_ROD_NORTH = 2;
static constexpr NIBBLETYPE END_CITY_END_ROD_SOUTH = 3;

/** Ladder and wall banner facing metadata. These use the vanilla facing values. */
static constexpr NIBBLETYPE END_CITY_FACING_NORTH = 2;
static constexpr NIBBLETYPE END_CITY_FACING_SOUTH = 3;
static constexpr NIBBLETYPE END_CITY_FACING_WEST = 4;
static constexpr NIBBLETYPE END_CITY_FACING_EAST = 5;

/** Purpur pillar axis metadata. */
static constexpr NIBBLETYPE END_CITY_PILLAR_VERTICAL = 0;
static constexpr NIBBLETYPE END_CITY_PILLAR_X = 4;
static constexpr NIBBLETYPE END_CITY_PILLAR_Z = 8;

/** Purpur slab half metadata. */
static constexpr NIBBLETYPE END_CITY_SLAB_BOTTOM = 0;
static constexpr NIBBLETYPE END_CITY_SLAB_TOP = 8;





/** A generated entity marker (for example the ship's item frame) in a prefab's local coordinates. */
struct sEntityMarker
{
	/** The cell the entity occupies. */
	Vector3i m_Pos;

	/** The support block face the entity hangs on. */
	eBlockFace m_Face;
} ;





/** A generated mob head (for example the ship's dragon head) in a prefab's local coordinates. */
struct sMobHeadMarker
{
	/** The cell the head occupies. */
	Vector3i m_Pos;

	/** The direction the head faces. */
	eMobHeadRotation m_Rotation;
} ;





/** A prefab together with the oriented coordinate of its minimum corner.
Used when a blueprint built along the +Z axis is rotated into an arbitrary horizontal direction. */
struct sOrientedPrefab
{
	/** The prefab itself. */
	std::unique_ptr<cPrefab> m_Prefab;

	/** The oriented coordinate that ends up at the prefab's minimum corner. */
	Vector3i m_MinOffset;

	/** Offsets of the loot chests, relative to the prefab's minimum corner. */
	std::vector<Vector3i> m_Chests;

	/** Item frame markers, relative to the prefab's minimum corner. */
	std::vector<sEntityMarker> m_ItemFrames;

	/** Offsets of the brewing stands, relative to the prefab's minimum corner. */
	std::vector<Vector3i> m_BrewingStands;

	/** Mob head markers, relative to the prefab's minimum corner. */
	std::vector<sMobHeadMarker> m_MobHeads;
} ;





/** Returns a deterministic seed for one grid cell. */
static UInt32 MakeCellSeed(int a_Seed, int a_CellX, int a_CellZ)
{
	return static_cast<UInt32>(a_Seed)
		^ (static_cast<UInt32>(a_CellX) * 73856093u)
		^ (static_cast<UInt32>(a_CellZ) * 19349663u);
}





/** Returns the size of a prefab. cPrefab narrows the access of cPiece::GetSize(), so query the base. */
static Vector3i PrefabSize(const cPrefab & a_Prefab)
{
	return static_cast<const cPiece &>(a_Prefab).GetSize();
}





/** Returns true if the world XZ coordinates are inside the chunk with the specified minimum corner. */
static bool IsInChunk(const Vector3i & a_Pos, int a_ChunkMinX, int a_ChunkMinZ)
{
	const int RelX = a_Pos.x - a_ChunkMinX;
	const int RelZ = a_Pos.z - a_ChunkMinZ;
	return (RelX >= 0) && (RelX < cChunkDef::Width) && (RelZ >= 0) && (RelZ < cChunkDef::Width);
}





/** The special-content filler installed by the loot module. When it is absent (for example in the
test build, which does not link the item or entity systems) generated chests are left empty and no
entities are spawned. */
static EndCityContentsFiller g_EndCityContentsFiller = nullptr;

void SetEndCityContentsFiller(EndCityContentsFiller a_Filler)
{
	g_EndCityContentsFiller = a_Filler;
}





/** Returns the offsets, relative to the area's minimum corner, of the blocks of the specified type. */
static std::vector<Vector3i> CollectBlocks(const cBlockArea & a_Area, BLOCKTYPE a_Type)
{
	std::vector<Vector3i> Positions;
	for (int y = 0; y < a_Area.GetSizeY(); y++)
	{
		for (int z = 0; z < a_Area.GetSizeZ(); z++)
		{
			for (int x = 0; x < a_Area.GetSizeX(); x++)
			{
				if (a_Area.GetRelBlockType(x, y, z) == a_Type)
				{
					Positions.push_back(Vector3i(x, y, z));
				}
			}
		}
	}
	return Positions;
}





/** Returns the purpur stair metadata for the specified clockwise rotation. */
static NIBBLETYPE StairMetaForRotation(int a_Degrees)
{
	switch (a_Degrees)
	{
		case 90:  return E_BLOCK_STAIRS_XM;
		case 180: return E_BLOCK_STAIRS_ZM;
		case 270: return E_BLOCK_STAIRS_XP;
		default:  return E_BLOCK_STAIRS_ZP;
	}
}





/** Returns the End rod metadata for the specified clockwise rotation. */
static NIBBLETYPE EndRodMetaForRotation(int a_Degrees)
{
	switch (a_Degrees)
	{
		case 90:  return END_CITY_END_ROD_NORTH;
		case 180: return END_CITY_END_ROD_DOWN;
		case 270: return END_CITY_END_ROD_SOUTH;
		default:  return END_CITY_END_ROD_UP;
	}
}





/** Resolves a blueprint block name (and optional orientation suffix) into a Cuberite block type and metadata.
Reference markers (wool) and entity sprites resolve to air. Returns true if the cell holds a block. */
static bool ResolveBlock(const AString & a_Name, BLOCKTYPE & a_Type, NIBBLETYPE & a_Meta)
{
	a_Type = E_BLOCK_AIR;
	a_Meta = 0;

	// The ship's item frame becomes a placeholder block so that the prefab builders can pass its
	// position and facing through the crop, rotation and orientation:
	if (a_Name == "EntitySprite:Item Frame")
	{
		a_Type = E_BLOCK_BEDROCK;
		return true;
	}

	// Other entity sprites and the wool reference markers are not blocks:
	if (
		(a_Name.compare(0, 13, "EntitySprite:") == 0) ||
		(a_Name.find("Wool") != AString::npos)
	)
	{
		return false;
	}

	// Split off the "@modifier" and "-rotNNN" suffixes:
	AString Name = a_Name;
	AString Modifier;
	const size_t ModPos = Name.find('@');
	if (ModPos != AString::npos)
	{
		Modifier = Name.substr(ModPos + 1);
		Name = Name.substr(0, ModPos);
	}
	int Rotation = 0;
	const size_t RotPos = Name.find("-rot");
	if (RotPos != AString::npos)
	{
		Rotation = std::atoi(Name.substr(RotPos + 4).c_str());
		Name = Name.substr(0, RotPos);
	}

	if (Name == "Purpur Block")
	{
		a_Type = E_BLOCK_PURPUR_BLOCK;
		return true;
	}
	if (Name == "Purpur Pillar")
	{
		a_Type = E_BLOCK_PURPUR_PILLAR;
		a_Meta = (Modifier == "horizontal") ? END_CITY_PILLAR_Z : END_CITY_PILLAR_VERTICAL;
		return true;
	}
	if (Name == "Purpur Slab")
	{
		a_Type = E_BLOCK_PURPUR_SLAB;
		a_Meta = (Modifier == "top") ? END_CITY_SLAB_TOP : END_CITY_SLAB_BOTTOM;
		return true;
	}
	if (Name == "Purpur Stairs")
	{
		a_Type = E_BLOCK_PURPUR_STAIRS;
		a_Meta = StairMetaForRotation(Rotation);
		return true;
	}
	if (Name == "End Stone Bricks")
	{
		a_Type = E_BLOCK_END_BRICKS;
		return true;
	}
	if (Name == "Magenta Stained Glass")
	{
		a_Type = E_BLOCK_STAINED_GLASS;
		a_Meta = E_META_STAINED_GLASS_MAGENTA;
		return true;
	}
	if (Name == "Purple Stained Glass")
	{
		a_Type = E_BLOCK_STAINED_GLASS;
		a_Meta = E_META_STAINED_GLASS_PURPLE;
		return true;
	}
	if (Name == "End Rod")
	{
		a_Type = E_BLOCK_END_ROD;
		a_Meta = EndRodMetaForRotation(Rotation);
		return true;
	}
	if (Name == "Ladder")
	{
		a_Type = E_BLOCK_LADDER;
		a_Meta = END_CITY_FACING_NORTH;
		return true;
	}
	if (Name == "Magenta Wall Banner")
	{
		a_Type = E_BLOCK_WALL_BANNER;
		a_Meta = END_CITY_FACING_NORTH;
		return true;
	}
	if (Name == "Chest")
	{
		a_Type = E_BLOCK_CHEST;
		a_Meta = E_META_CHEST_FACING_ZM;
		return true;
	}
	if (Name == "Ender Chest")
	{
		a_Type = E_BLOCK_ENDER_CHEST;
		a_Meta = E_META_CHEST_FACING_ZM;
		return true;
	}
	if (Name == "Brewing Stand")
	{
		a_Type = E_BLOCK_BREWING_STAND;
		return true;
	}
	if (Name == "Obsidian")
	{
		a_Type = E_BLOCK_OBSIDIAN;
		return true;
	}
	if (Name == "Dragon Head")
	{
		a_Type = E_BLOCK_HEAD;
		a_Meta = E_META_HEAD_DRAGON;
		return true;
	}
	return false;
}





/** Returns the height at which the next piece should be stacked: one above the highest layer that is
filled enough to be structural. Decorative top layers (e.g. a few end rods) are overlapped instead. */
static int ComputeStackHeight(const sEndCityBlueprint & a_Blueprint)
{
	// Parse the char map to know which characters are blocks:
	bool IsBlock[256];
	for (size_t i = 0; i < ARRAYCOUNT(IsBlock); i++)
	{
		IsBlock[i] = false;
	}
	for (const auto & Entry: StringSplit(a_Blueprint.m_CharMap, "|"))
	{
		if ((Entry.size() >= 2) && (Entry[1] == '='))
		{
			BLOCKTYPE Type;
			NIBBLETYPE Meta;
			IsBlock[static_cast<unsigned char>(Entry[0])] = ResolveBlock(Entry.substr(2), Type, Meta);
		}
	}

	// Find the highest layer that is at least 10% filled:
	for (int i = a_Blueprint.m_Height - 1; i >= 0; i--)
	{
		const auto Rows = StringSplit(a_Blueprint.m_Layers[i].m_Rows, "|");
		int Count = 0;
		int Total = 0;
		for (const auto & Row: Rows)
		{
			for (const char C: Row)
			{
				Total++;
				if (IsBlock[static_cast<unsigned char>(C)])
				{
					Count++;
				}
			}
		}
		// Only the trailing decorative layers (a few end rods) are sparse enough to be overlapped;
		// a room's wall layers are also sparse, so the threshold must stay below them:
		if ((Total > 0) && ((Count * 100) > (Total * END_CITY_STACK_MIN_PERCENT)))
		{
			return i + 1;
		}
	}
	return a_Blueprint.m_Height;
}





/** Returns the stacking height of the named blueprint, or its full height if it is unknown. */
static int StackHeightForName(const AString & a_Name)
{
	for (int i = 0; i < g_NumEndCityBlueprints; i++)
	{
		if (a_Name == g_EndCityBlueprints[i].m_Name)
		{
			return ComputeStackHeight(g_EndCityBlueprints[i]);
		}
	}
	return 0;
}





/** Crops the area to its content. When a_KeepFrame is set the declared X and Z frame is kept and only
empty top and bottom layers are trimmed, so the piece's frame coordinates stay meaningful. Returns
false if the area is empty. */
static bool CropToContent(cBlockArea & a_Area, bool a_KeepFrame = false)
{
	int MinX = a_Area.GetSizeX();
	int MaxX = -1;
	int MinY = a_Area.GetSizeY();
	int MaxY = -1;
	int MinZ = a_Area.GetSizeZ();
	int MaxZ = -1;
	for (int y = 0; y < a_Area.GetSizeY(); y++)
	{
		for (int z = 0; z < a_Area.GetSizeZ(); z++)
		{
			for (int x = 0; x < a_Area.GetSizeX(); x++)
			{
				BLOCKTYPE Type;
				NIBBLETYPE Meta;
				a_Area.GetRelBlockTypeMeta(x, y, z, Type, Meta);
				if (Type == E_BLOCK_AIR)
				{
					continue;
				}
				MinX = std::min(MinX, x);
				MaxX = std::max(MaxX, x);
				MinY = std::min(MinY, y);
				MaxY = std::max(MaxY, y);
				MinZ = std::min(MinZ, z);
				MaxZ = std::max(MaxZ, z);
			}
		}
	}
	if (MaxX < 0)
	{
		return false;
	}
	if (a_KeepFrame)
	{
		a_Area.Crop(0, 0, MinY, a_Area.GetSizeY() - 1 - MaxY, 0, 0);
	}
	else
	{
		a_Area.Crop(
			MinX, a_Area.GetSizeX() - 1 - MaxX,
			MinY, a_Area.GetSizeY() - 1 - MaxY,
			MinZ, a_Area.GetSizeZ() - 1 - MaxZ
		);
	}
	return true;
}





/** Builds a block area from a blueprint, cropped to its non-air bounding box.
Returns nullptr if the blueprint holds no blocks. */
/** Chests and ladders need a valid facing meta and the blueprints do not store one. A chest faces
away from an adjacent solid block; a ladder's meta names the side its support block is on. This is
idempotent, so it can be re-run after a rotation that may not have rotated the metas. */
static void FixFacingMetas(cBlockArea & a_Area)
{
	const int SizeX = a_Area.GetSizeX();
	const int SizeZ = a_Area.GetSizeZ();
	for (int y = 0; y < a_Area.GetSizeY(); y++)
	{
		for (int z = 0; z < SizeZ; z++)
		{
			for (int x = 0; x < SizeX; x++)
			{
				const BLOCKTYPE Type = a_Area.GetRelBlockType(x, y, z);
				const bool IsChest = (Type == E_BLOCK_CHEST) || (Type == E_BLOCK_ENDER_CHEST);
				const bool IsLadder = (Type == E_BLOCK_LADDER);
				if (!IsChest && !IsLadder)
				{
					continue;
				}

				const bool West = (x > 0) && (a_Area.GetRelBlockType(x - 1, y, z) != E_BLOCK_AIR);
				const bool East = (x < SizeX - 1) && (a_Area.GetRelBlockType(x + 1, y, z) != E_BLOCK_AIR);
				const bool North = (z > 0) && (a_Area.GetRelBlockType(x, y, z - 1) != E_BLOCK_AIR);
				const bool South = (z < SizeZ - 1) && (a_Area.GetRelBlockType(x, y, z + 1) != E_BLOCK_AIR);

				NIBBLETYPE Facing = E_META_CHEST_FACING_ZM;
				if (IsChest)
				{
					if (West)
					{
						Facing = E_META_CHEST_FACING_XP;
					}
					else if (East)
					{
						Facing = E_META_CHEST_FACING_XM;
					}
					else if (South)
					{
						Facing = E_META_CHEST_FACING_ZM;
					}
					else if (North)
					{
						Facing = E_META_CHEST_FACING_ZP;
					}
				}
				else
				{
					// The ladder's meta is the direction it faces, which is away from the block it is
					// attached to (cBlockLadderHandler::CanBeAt checks the opposite neighbour):
					Facing = END_CITY_FACING_NORTH;
					if (West)
					{
						Facing = END_CITY_FACING_EAST;
					}
					else if (East)
					{
						Facing = END_CITY_FACING_WEST;
					}
					else if (North)
					{
						Facing = END_CITY_FACING_SOUTH;
					}
					else if (South)
					{
						Facing = END_CITY_FACING_NORTH;
					}
				}
				a_Area.SetRelBlockTypeMeta(x, y, z, Type, Facing);
			}
		}
	}
}





static std::unique_ptr<cBlockArea> MakeBlueprintArea(const sEndCityBlueprint & a_Blueprint)
{
	auto Area = std::make_unique<cBlockArea>();
	Area->Create(a_Blueprint.m_SizeX, a_Blueprint.m_Height, a_Blueprint.m_SizeZ);

	// Parse the char map:
	BLOCKTYPE Types[256];
	NIBBLETYPE Metas[256];
	for (size_t i = 0; i < ARRAYCOUNT(Types); i++)
	{
		Types[i] = E_BLOCK_AIR;
		Metas[i] = 0;
	}
	for (const auto & Entry: StringSplit(a_Blueprint.m_CharMap, "|"))
	{
		if ((Entry.size() < 2) || (Entry[1] != '='))
		{
			continue;
		}
		const unsigned char Key = static_cast<unsigned char>(Entry[0]);
		ResolveBlock(Entry.substr(2), Types[Key], Metas[Key]);
	}

	// Fill the layers:
	for (int i = 0; i < a_Blueprint.m_Height; i++)
	{
		const sEndCityBlueprintLayer & Layer = a_Blueprint.m_Layers[i];
		const auto Rows = StringSplit(Layer.m_Rows, "|");
		for (size_t z = 0; z < Rows.size(); z++)
		{
			for (size_t x = 0; x < Rows[z].size(); x++)
			{
				const unsigned char Key = static_cast<unsigned char>(Rows[z][x]);
				if (Types[Key] != E_BLOCK_AIR)
				{
					Area->SetRelBlockTypeMeta(static_cast<int>(x), Layer.m_Y, static_cast<int>(z), Types[Key], Metas[Key]);
				}
			}
		}
	}

	FixFacingMetas(*Area);

	if (!CropToContent(*Area, a_Blueprint.m_KeepFrame))
	{
		return nullptr;
	}
	return Area;
}





/** Orients an area whose length runs along +Z into the given horizontal direction.
Returns the prefab together with the offset of its minimum corner; the area's near end (the middle of
its local z = 0 edge) ends up at the requested edge. Rotating with cBlockArea::RotateCCW keeps the
stairs and pillars facing the right way, which a plain coordinate transform would not. */
static sOrientedPrefab OrientAreaZ(const cBlockArea & a_Area, int a_DirX, int a_DirZ)
{
	// The base area runs along +Z, so a quarter turn maps +Z to +X:
	int Turns = 0;
	if (a_DirX > 0)
	{
		Turns = 1;
	}
	else if (a_DirX < 0)
	{
		Turns = 3;
	}
	else if (a_DirZ < 0)
	{
		Turns = 2;
	}

	// Track where the near end ends up so the caller can place the prefab's minimum corner:
	int SizeX = a_Area.GetSizeX();
	int SizeZ = a_Area.GetSizeZ();
	int NearX = SizeX / 2;
	int NearZ = 0;
	for (int i = 0; i < Turns; i++)
	{
		const int NewX = NearZ;
		const int NewZ = SizeX - NearX - 1;
		NearX = NewX;
		NearZ = NewZ;
		std::swap(SizeX, SizeZ);
	}

	// cBlockArea is not copyable, so copy through CopyFrom:
	cBlockArea Out;
	Out.CopyFrom(a_Area);
	for (int i = 0; i < Turns; i++)
	{
		Out.RotateCCW();
	}
	FixFacingMetas(Out);

	// The ship's item frame marker is a placeholder block; record it and remove the placeholder:
	std::vector<sEntityMarker> ItemFrames;
	for (int y = 0; y < Out.GetSizeY(); y++)
	{
		for (int z = 0; z < Out.GetSizeZ(); z++)
		{
			for (int x = 0; x < Out.GetSizeX(); x++)
			{
				if (Out.GetRelBlockType(x, y, z) != E_BLOCK_BEDROCK)
				{
					continue;
				}
				// The frame hangs on an adjacent solid block, so face away from it:
				eBlockFace Face = BLOCK_FACE_ZP;
				if ((x > 0) && (Out.GetRelBlockType(x - 1, y, z) != E_BLOCK_AIR))
				{
					Face = BLOCK_FACE_XP;
				}
				else if ((x < Out.GetSizeX() - 1) && (Out.GetRelBlockType(x + 1, y, z) != E_BLOCK_AIR))
				{
					Face = BLOCK_FACE_XM;
				}
				else if ((z > 0) && (Out.GetRelBlockType(x, y, z - 1) != E_BLOCK_AIR))
				{
					Face = BLOCK_FACE_ZP;
				}
				else if ((z < Out.GetSizeZ() - 1) && (Out.GetRelBlockType(x, y, z + 1) != E_BLOCK_AIR))
				{
					Face = BLOCK_FACE_ZM;
				}
				ItemFrames.push_back({Vector3i(x, y, z), Face});
				Out.SetRelBlockType(x, y, z, E_BLOCK_AIR);
			}
		}
	}

	// The ship's dragon head points away from the ship; the base blueprint has it facing -Z, so face
	// it along the ship's forward direction. A floor head at rotation 0 renders facing +Z, the opposite
	// of the value's name, so the local -Z facing is rotation 8 and the other directions follow:
	eMobHeadRotation HeadRotation = SKULL_ROTATION_SOUTH;
	if (a_DirX > 0)
	{
		HeadRotation = SKULL_ROTATION_EAST;
	}
	else if (a_DirX < 0)
	{
		HeadRotation = SKULL_ROTATION_WEST;
	}
	else if (a_DirZ < 0)
	{
		HeadRotation = SKULL_ROTATION_NORTH;
	}
	std::vector<sMobHeadMarker> MobHeads;
	for (const auto & HeadPos: CollectBlocks(Out, E_BLOCK_HEAD))
	{
		MobHeads.push_back({HeadPos, HeadRotation});
	}

	auto Prefab = std::make_unique<cPrefab>(Out);
	Prefab->SetMergeStrategy(cBlockArea::msImprint);
	return { std::move(Prefab), Vector3i(-NearX, 0, -NearZ), CollectBlocks(Out, E_BLOCK_CHEST), ItemFrames, CollectBlocks(Out, E_BLOCK_BREWING_STAND), MobHeads };
}





/** A prefab and the side its doorway opens onto (END_CITY_DIR_* index, or -1 when it has none). */
struct sRotatedPrefab
{
	/** The prefab itself. */
	std::unique_ptr<cPrefab> m_Prefab;

	/** The horizontal side the doorway opens onto, or -1 when the piece has no doorway. */
	int m_DoorwaySide = -1;

	/** The doorway centre in the prefab's local X and Z coordinates. */
	int m_DoorX = 0;
	int m_DoorZ = 0;

	/** Offsets of the loot chests, relative to the prefab's minimum corner. */
	std::vector<Vector3i> m_Chests;

	/** The X and Z of the ladder column, relative to the prefab's minimum corner, or -1 when the
	piece has no ladder. A tower stacked on the room continues this column. */
	int m_LadderX = -1;
	int m_LadderZ = -1;
} ;




/** Returns the side (an END_CITY_DIR_* index) with the most open wall cells near the floor, or -1.
Used to find which way a room's doorway faces so the room can be rotated toward a bridge. */
static int DoorwaySide(const cBlockArea & a_Area, int & a_DoorX, int & a_DoorZ)
{
	const int SizeX = a_Area.GetSizeX();
	const int SizeY = a_Area.GetSizeY();
	const int SizeZ = a_Area.GetSizeZ();
	const int MaxY = std::min(4, SizeY);

	// The roof of a room is wider than its lower walls, so the walls are not at the area's edge.
	// Find the wall ring near the floor first, then look for a gap in each of its four sides:
	int MinX = SizeX;
	int MaxX = -1;
	int MinZ = SizeZ;
	int MaxZ = -1;
	for (int y = 1; y < MaxY; y++)
	{
		for (int z = 0; z < SizeZ; z++)
		{
			for (int x = 0; x < SizeX; x++)
			{
				if (a_Area.GetRelBlockType(x, y, z) != E_BLOCK_AIR)
				{
					MinX = std::min(MinX, x);
					MaxX = std::max(MaxX, x);
					MinZ = std::min(MinZ, z);
					MaxZ = std::max(MaxZ, z);
				}
			}
		}
	}
	if (MaxX < MinX)
	{
		return -1;
	}

	int Air[END_CITY_DIR_COUNT] = {0, 0, 0, 0};
	for (int y = 1; y < MaxY; y++)
	{
		for (int z = MinZ; z <= MaxZ; z++)
		{
			if (a_Area.GetRelBlockType(MaxX, y, z) == E_BLOCK_AIR)
			{
				Air[0]++;  // +X
			}
			if (a_Area.GetRelBlockType(MinX, y, z) == E_BLOCK_AIR)
			{
				Air[2]++;  // -X
			}
		}
		for (int x = MinX; x <= MaxX; x++)
		{
			if (a_Area.GetRelBlockType(x, y, MaxZ) == E_BLOCK_AIR)
			{
				Air[1]++;  // +Z
			}
			if (a_Area.GetRelBlockType(x, y, MinZ) == E_BLOCK_AIR)
			{
				Air[3]++;  // -Z
			}
		}
	}
	int Best = 0;
	for (int i = 1; i < END_CITY_DIR_COUNT; i++)
	{
		if (Air[i] > Air[Best])
		{
			Best = i;
		}
	}
	if (Air[Best] == 0)
	{
		return -1;
	}

	// The doorway centre, so a caller can line the opening up with a bridge:
	long SumX = 0;
	long SumZ = 0;
	long Count = 0;
	for (int y = 1; y < MaxY; y++)
	{
		if (Best == 0)
		{
			for (int z = MinZ; z <= MaxZ; z++)
			{
				if (a_Area.GetRelBlockType(MaxX, y, z) == E_BLOCK_AIR)
				{
					SumX += MaxX;
					SumZ += z;
					Count++;
				}
			}
		}
		else if (Best == 2)
		{
			for (int z = MinZ; z <= MaxZ; z++)
			{
				if (a_Area.GetRelBlockType(MinX, y, z) == E_BLOCK_AIR)
				{
					SumX += MinX;
					SumZ += z;
					Count++;
				}
			}
		}
		else if (Best == 1)
		{
			for (int x = MinX; x <= MaxX; x++)
			{
				if (a_Area.GetRelBlockType(x, y, MaxZ) == E_BLOCK_AIR)
				{
					SumX += x;
					SumZ += MaxZ;
					Count++;
				}
			}
		}
		else
		{
			for (int x = MinX; x <= MaxX; x++)
			{
				if (a_Area.GetRelBlockType(x, y, MinZ) == E_BLOCK_AIR)
				{
					SumX += x;
					SumZ += MinZ;
					Count++;
				}
			}
		}
	}
	if (Count > 0)
	{
		a_DoorX = static_cast<int>(SumX / Count);
		a_DoorZ = static_cast<int>(SumZ / Count);
	}
	return Best;
}




/** Holds one prefab for every End City piece used by the generator. */
class cEndCityPieces
{
public:

	cEndCityPieces()
	{
		m_BaseRoom = MakePrefab("BaseRoom");
		m_TowerBase = MakePrefab("TowerBase");
		m_TowerPiece = MakePrefab("TowerPiece");
		m_TowerFloor = MakePrefab("TowerFloor");
		m_TowerTop = MakePrefab("TowerTop");
		m_FatTower = MakePrefab("FatTower");
		m_FatTowerTop = MakePrefab("FatTowerTop");
		m_FatTowerTopChests = ChestsOf("FatTowerTop");
		MakeRotatedPrefabs("EmptyRoom", m_EmptyRoom);
		MakeRotatedPrefabs("LootRoom1", m_LootRoom1);
		MakeRotatedPrefabs("LootRoom2", m_LootRoom2);
		MakeRotatedPrefabs("LootRoom3", m_LootRoom3);

		// Extend the bottom piece down to the terrain so that slopes do not leave a gap:
		if (m_BaseRoom != nullptr)
		{
			m_BaseRoom->SetExtendFloorStrategy(cPrefab::efsRepeatBottomTillSolid);
		}

		// The bridge and ship pieces are authored along +Z, so build one orientation per direction:
		OrientForAllDirections("BridgePiece", m_Bridge);
		OrientForAllDirections("BridgeGentleStairs", m_BridgeGentle);
		OrientForAllDirections("BridgeSteepStairs", m_BridgeSteep);
		OrientForAllDirections("BridgeEnd", m_BridgeEnd);
		OrientForAllDirections("Ship", m_Ship);
	}

	std::unique_ptr<cPrefab> m_BaseRoom;
	std::unique_ptr<cPrefab> m_TowerBase;
	std::unique_ptr<cPrefab> m_TowerPiece;
	std::unique_ptr<cPrefab> m_TowerFloor;
	std::unique_ptr<cPrefab> m_TowerTop;
	std::unique_ptr<cPrefab> m_FatTower;
	std::unique_ptr<cPrefab> m_FatTowerTop;
	std::vector<Vector3i> m_FatTowerTopChests;
	sRotatedPrefab m_EmptyRoom[END_CITY_DIR_COUNT];
	sRotatedPrefab m_LootRoom1[END_CITY_DIR_COUNT];
	sRotatedPrefab m_LootRoom2[END_CITY_DIR_COUNT];
	sRotatedPrefab m_LootRoom3[END_CITY_DIR_COUNT];

	sOrientedPrefab m_Bridge[END_CITY_DIR_COUNT];
	sOrientedPrefab m_BridgeGentle[END_CITY_DIR_COUNT];
	sOrientedPrefab m_BridgeSteep[END_CITY_DIR_COUNT];
	sOrientedPrefab m_BridgeEnd[END_CITY_DIR_COUNT];
	sOrientedPrefab m_Ship[END_CITY_DIR_COUNT];

protected:

	/** Returns the blueprint with the specified logical name, or nullptr. */
	static const sEndCityBlueprint * FindBlueprint(const AString & a_Name)
	{
		for (int i = 0; i < g_NumEndCityBlueprints; i++)
		{
			if (a_Name == g_EndCityBlueprints[i].m_Name)
			{
				return &g_EndCityBlueprints[i];
			}
		}
		return nullptr;
	}

	/** Builds the named blueprint oriented into all four horizontal directions. */
	static void OrientForAllDirections(const AString & a_Name, sOrientedPrefab (&a_Out)[END_CITY_DIR_COUNT])
	{
		const sEndCityBlueprint * Blueprint = FindBlueprint(a_Name);
		if (Blueprint == nullptr)
		{
			return;
		}
		auto Area = MakeBlueprintArea(*Blueprint);
		if (Area == nullptr)
		{
			return;
		}
		for (int i = 0; i < END_CITY_DIR_COUNT; i++)
		{
			a_Out[i] = OrientAreaZ(*Area, END_CITY_DIR_X[i], END_CITY_DIR_Z[i]);
		}
	}





	/** Returns the loot-chest offsets of the named blueprint, relative to its minimum corner. */
	static std::vector<Vector3i> ChestsOf(const AString & a_Name)
	{
		const sEndCityBlueprint * Blueprint = FindBlueprint(a_Name);
		if (Blueprint == nullptr)
		{
			return {};
		}
		auto Area = MakeBlueprintArea(*Blueprint);
		if (Area == nullptr)
		{
			return {};
		}
		return CollectBlocks(*Area, E_BLOCK_CHEST);
	}





	/** Builds a prefab from the named blueprint, or nullptr if it is missing or empty. */
	static std::unique_ptr<cPrefab> MakePrefab(const AString & a_Name)
	{
		const sEndCityBlueprint * Blueprint = FindBlueprint(a_Name);
		if (Blueprint == nullptr)
		{
			return nullptr;
		}
		auto Area = MakeBlueprintArea(*Blueprint);
		if (Area == nullptr)
		{
			return nullptr;
		}
		// Air cells must behave like structure voids: they must not erase blocks placed by other pieces:
		auto Prefab = std::make_unique<cPrefab>(*Area);
		Prefab->SetMergeStrategy(cBlockArea::msImprint);
		return Prefab;
	}





	/** Builds the named blueprint in all four rotations and records which side each doorway faces. */
	static void MakeRotatedPrefabs(const AString & a_Name, sRotatedPrefab (&a_Out)[END_CITY_DIR_COUNT])
	{
		const sEndCityBlueprint * Blueprint = FindBlueprint(a_Name);
		if (Blueprint == nullptr)
		{
			return;
		}
		for (int r = 0; r < END_CITY_DIR_COUNT; r++)
		{
			// cBlockArea is not copyable, so build a fresh area for every rotation:
			auto Rotated = MakeBlueprintArea(*Blueprint);
			if (Rotated == nullptr)
			{
				return;
			}
			for (int k = 0; k < r; k++)
			{
				Rotated->RotateCCW();
			}
			FixFacingMetas(*Rotated);
			a_Out[r].m_DoorwaySide = DoorwaySide(*Rotated, a_Out[r].m_DoorX, a_Out[r].m_DoorZ);
			a_Out[r].m_Chests = CollectBlocks(*Rotated, E_BLOCK_CHEST);
			a_Out[r].m_LadderX = -1;
			a_Out[r].m_LadderZ = -1;
			for (int y = Rotated->GetSizeY() - 1; y >= 0; y--)
			{
				bool Found = false;
				for (int z = 0; z < Rotated->GetSizeZ(); z++)
				{
					for (int x = 0; x < Rotated->GetSizeX(); x++)
					{
						if (Rotated->GetRelBlockType(x, y, z) == E_BLOCK_LADDER)
						{
							a_Out[r].m_LadderX = x;
							a_Out[r].m_LadderZ = z;
							Found = true;
							break;
						}
					}
					if (Found)
					{
						break;
					}
				}
				if (Found)
				{
					break;
				}
			}
			auto Prefab = std::make_unique<cPrefab>(*Rotated);
			Prefab->SetMergeStrategy(cBlockArea::msImprint);
			a_Out[r].m_Prefab = std::move(Prefab);
		}
	}
} ;





/** Returns the shared, immutable piece set. Generation is single-threaded, so no locking is needed. */
static const cEndCityPieces & GetEndCityPieces(void)
{
	static const cEndCityPieces Pieces;
	return Pieces;
}





class cEndCityGen::cEndCity:
	public cGridStructGen::cStructure
{
	using Super = cGridStructGen::cStructure;

public:

	cEndCity(int a_Seed, int a_GridX, int a_GridZ, int a_OriginX, int a_OriginZ, cTerrainHeightGen & a_HeightGen) :
		Super(a_GridX, a_GridZ, a_OriginX, a_OriginZ)
	{
		Build(a_Seed, a_HeightGen);
	}

	// cGridStructGen::cStructure override:
	virtual void DrawIntoChunk(cChunkDesc & a_Chunk) override
	{
		for (const auto & Piece: m_Pieces)
		{
			Piece.m_Prefab->Draw(a_Chunk, Piece.m_Pos, 0);
		}

		// Carve the doorways only after all the pieces have been drawn:
		const int ChunkX = a_Chunk.GetChunkX();
		const int ChunkZ = a_Chunk.GetChunkZ();
		const int ChunkMinX = ChunkX * cChunkDef::Width;
		const int ChunkMinZ = ChunkZ * cChunkDef::Width;
		for (const auto & Carve: m_Carves)
		{
			const int MinX = std::max(Carve.m_MinX, ChunkMinX);
			const int MaxX = std::min(Carve.m_MaxX, ChunkMinX + cChunkDef::Width - 1);
			const int MinZ = std::max(Carve.m_MinZ, ChunkMinZ);
			const int MaxZ = std::min(Carve.m_MaxZ, ChunkMinZ + cChunkDef::Width - 1);
			for (int y = Carve.m_MinY; y <= Carve.m_MaxY; y++)
			{
				for (int z = MinZ; z <= MaxZ; z++)
				{
					for (int x = MinX; x <= MaxX; x++)
					{
					a_Chunk.SetBlockTypeMeta(x - ChunkMinX, y, z - ChunkMinZ, E_BLOCK_AIR, 0);
					}
				}
			}
		}

		// Hand this chunk's special contents (chests, brewing stands, item frames) to the filler:
		if (g_EndCityContentsFiller != nullptr)
		{
			std::vector<sEndCityContent> Contents;
			for (const auto & Piece: m_Pieces)
			{
				for (const auto & Chest: Piece.m_Chests)
				{
					const Vector3i World = Piece.m_Pos + Chest;
					if (IsInChunk(World, ChunkMinX, ChunkMinZ))
					{
						Contents.push_back({ecctChest, World, BLOCK_FACE_NONE, 0});
					}
				}
				for (const auto & Stand: Piece.m_BrewingStands)
				{
					const Vector3i World = Piece.m_Pos + Stand;
					if (IsInChunk(World, ChunkMinX, ChunkMinZ))
					{
						Contents.push_back({ecctBrewingStand, World, BLOCK_FACE_NONE, 0});
					}
				}
				for (const auto & Frame: Piece.m_ItemFrames)
				{
					const Vector3i World = Piece.m_Pos + Frame.m_Pos;
					if (IsInChunk(World, ChunkMinX, ChunkMinZ))
					{
						Contents.push_back({ecctItemFrame, World, Frame.m_Face, 0});
					}
				}
				for (const auto & Head: Piece.m_MobHeads)
				{
					const Vector3i World = Piece.m_Pos + Head.m_Pos;
					if (IsInChunk(World, ChunkMinX, ChunkMinZ))
					{
						Contents.push_back({ecctMobHead, World, BLOCK_FACE_NONE, static_cast<int>(Head.m_Rotation)});
					}
				}
			}
			g_EndCityContentsFiller(a_Chunk, Contents);
		}
	}

protected:

	/** A single prefab placed at absolute world coordinates. */
	struct sPlacedPiece
	{
		const cPrefab * m_Prefab;
		Vector3i m_Pos;
		std::vector<Vector3i> m_Chests;
		std::vector<Vector3i> m_BrewingStands;
		std::vector<sEntityMarker> m_ItemFrames;
		std::vector<sMobHeadMarker> m_MobHeads;
	} ;

	/** An axis-aligned box that is cleared to air after the pieces are drawn, to open doorways. */
	struct sCarve
	{
		int m_MinX;
		int m_MaxX;
		int m_MinY;
		int m_MaxY;
		int m_MinZ;
		int m_MaxZ;
	} ;

	/** All the pieces of this city. */
	std::vector<sPlacedPiece> m_Pieces;

	/** The boxes cleared after drawing, to connect the pieces. */
	std::vector<sCarve> m_Carves;


	/** Clears a 3 x 3 x 3 doorway above the specified floor coords. */
	void AddCarve(int a_X, int a_Y, int a_Z)
	{
		m_Carves.push_back({a_X - 1, a_X + 1, a_Y, a_Y + 2, a_Z - 1, a_Z + 1});
	}


	/** Adds a piece at the specified minimum-corner coordinates, tracking its loot chests and entity markers. */
	void Add(const cPrefab * a_Prefab, const Vector3i & a_Pos, const std::vector<Vector3i> & a_Chests = {}, const std::vector<Vector3i> & a_BrewingStands = {}, const std::vector<sEntityMarker> & a_ItemFrames = {}, const std::vector<sMobHeadMarker> & a_MobHeads = {})
	{
		m_Pieces.push_back({a_Prefab, a_Pos, a_Chests, a_BrewingStands, a_ItemFrames, a_MobHeads});
	}

	/** Adds a piece centered horizontally, tracking its loot chests and entity markers. */
	void AddCentered(const cPrefab * a_Prefab, int a_CenterX, int a_Y, int a_CenterZ, const std::vector<Vector3i> & a_Chests = {}, const std::vector<Vector3i> & a_BrewingStands = {}, const std::vector<sEntityMarker> & a_ItemFrames = {}, const std::vector<sMobHeadMarker> & a_MobHeads = {})
	{
		const Vector3i Size = PrefabSize(*a_Prefab);
		Add(a_Prefab, Vector3i(a_CenterX - (Size.x / 2), a_Y, a_CenterZ - (Size.z / 2)), a_Chests, a_BrewingStands, a_ItemFrames, a_MobHeads);
	}

	/** Adds a piece centered on a shared blueprint frame rather than its own cropped bounding box, so
	pieces whose content does not fill the frame keep their blueprint coordinates. */
	void AddCenteredFrame(const cPrefab * a_Prefab, int a_FrameX, int a_FrameZ, int a_CenterX, int a_Y, int a_CenterZ)
	{
		Add(a_Prefab, Vector3i(a_CenterX - (a_FrameX / 2), a_Y, a_CenterZ - (a_FrameZ / 2)));
	}

	/** Builds the whole city layout, or leaves it empty if the location is not suitable. */
	void Build(int a_Seed, cTerrainHeightGen & a_HeightGen)
	{
		// Check that the footprint is flat land rather than the void:
		int MinH = cChunkDef::Height;
		int MaxH = 0;
		for (int dz = -(END_CITY_FOOTPRINT / 2); dz <= (END_CITY_FOOTPRINT / 2); dz++)
		{
			for (int dx = -(END_CITY_FOOTPRINT / 2); dx <= (END_CITY_FOOTPRINT / 2); dx++)
			{
				const int H = a_HeightGen.GetHeightAt(m_OriginX + dx, m_OriginZ + dz);
				MinH = std::min(MinH, H);
				MaxH = std::max(MaxH, H);
			}
		}
		if ((MaxH < END_CITY_MIN_LAND_Y) || ((MaxH - MinH) > END_CITY_MAX_HEIGHT_DIFFERENCE))
		{
			return;
		}
		const int BaseY = MaxH;

		const cEndCityPieces & P = GetEndCityPieces();
		std::minstd_rand Rng(MakeCellSeed(a_Seed, m_GridX + END_CITY_SEED_OFFSET_X, m_GridZ + END_CITY_SEED_OFFSET_Z));

		// The base tower is one combined room whose spiral ladder and floor openings are internally
		// consistent. Place it on its declared frame so its contents keep their blueprint coordinates:
		int Y = BaseY;
		if (P.m_BaseRoom != nullptr)
		{
			AddCenteredFrame(P.m_BaseRoom.get(), END_CITY_BASE_FRAME, END_CITY_BASE_FRAME, m_OriginX, Y, m_OriginZ);
			Y += StackHeightForName("BaseRoom");
		}

		// The wiki: the base room always carries a small tower (three, four or five storeys), and a
		// large tower only ever generates on top of a small tower, never directly on the base room:
		const bool Fat = ((Rng() % 2) == 0);
		const int TowerBaseY = Y;
		std::vector<int> StoreyYs;
		int StoreyCount = 0;

		// The small tower is shifted so its ladder continues the base room's shaft, so it is not
		// centred on the city origin. Everything stacked above must follow its centre:
		int TowerCenterX = m_OriginX;
		int TowerCenterZ = m_OriginZ;

		// The small tower always goes on the base room, its ladder shaft descending through the roof:
		if ((P.m_TowerBase != nullptr) && (P.m_TowerPiece != nullptr))
		{
			StoreyCount = 3 + static_cast<int>(Rng() % 3);  // 3, 4 or 5

			// The tower base starts with a ladder shaft, let it descend into the room below. Shift the
			// tower so its shaft continues the room's spiral ladder:
			const int SmallTowerX = m_OriginX - (END_CITY_BASE_FRAME / 2) + END_CITY_BASE_LADDER_X - END_CITY_TOWER_LADDER_X;
			const int SmallTowerZ = m_OriginZ - (END_CITY_BASE_FRAME / 2) + END_CITY_BASE_LADDER_Z - END_CITY_TOWER_LADDER_Z;
			TowerCenterX = SmallTowerX + (PrefabSize(*P.m_TowerBase).x / 2);
			TowerCenterZ = SmallTowerZ + (PrefabSize(*P.m_TowerBase).z / 2);
			StoreyYs.push_back(TowerBaseY);
			Add(P.m_TowerBase.get(), Vector3i(SmallTowerX, TowerBaseY - END_CITY_SMALL_TOWER_LADDER_DEPTH, SmallTowerZ));
			Y = TowerBaseY - END_CITY_SMALL_TOWER_LADDER_DEPTH + StackHeightForName("TowerBase");
			for (int i = 1; i < StoreyCount; i++)
			{
				StoreyYs.push_back(Y);
				Add(P.m_TowerPiece.get(), Vector3i(SmallTowerX, Y, SmallTowerZ));
				Y += StackHeightForName("TowerPiece");
			}

			if (Fat)
			{
				// The large tower replaces the small tower's roof, so it carries the bridges instead:
				StoreyCount = 0;
				StoreyYs.clear();
			}
			else if (P.m_TowerTop != nullptr)
			{
				// Cap the small tower with its banner roof:
				const int TopOffset = (PrefabSize(*P.m_TowerTop).x - PrefabSize(*P.m_TowerPiece).x) / 2;
				Add(P.m_TowerTop.get(), Vector3i(SmallTowerX - TopOffset, Y, SmallTowerZ - TopOffset));
				Y += StackHeightForName("TowerTop");
			}
		}

		// A large tower, when rolled, sits on top of the small tower:
		if (Fat && (P.m_FatTower != nullptr))
		{
			StoreyCount = 3 + (2 * static_cast<int>(Rng() % 3));  // 3, 5 or 7
			StoreyYs.clear();
			for (int i = 0; i < StoreyCount; i++)
			{
				StoreyYs.push_back(Y);
				AddCentered(P.m_FatTower.get(), TowerCenterX, Y, TowerCenterZ);
				Y += StackHeightForName("FatTower");
			}

			// Cap the fat tower with its loot room:
			if (P.m_FatTowerTop != nullptr)
			{
				AddCentered(P.m_FatTowerTop.get(), TowerCenterX, Y, TowerCenterZ, P.m_FatTowerTopChests);
				Y += StackHeightForName("FatTowerTop");
			}
		}

		// The storeys that may grow bridges (0-based), as the wiki documents them:
		std::vector<int> BranchStoreys;
		if (Fat)
		{
			if (StoreyCount == 5)
			{
				BranchStoreys.push_back(3);  // 4th storey
			}
			else if (StoreyCount == 7)
			{
				BranchStoreys.push_back(3);  // 4th storey
				BranchStoreys.push_back(5);  // 6th storey
			}
		}
		else if (StoreyCount >= 3)
		{
			// The 2nd up to the (StoreyCount - 1)-th storey, one of them:
			BranchStoreys.push_back(1 + static_cast<int>(Rng() % (StoreyCount - 2)));
		}

		// Grow a bridge from the tower's wall in one direction; it ends in a ship or another tower:
		const int TowerSize = Fat ? PrefabSize(*P.m_FatTower).x : PrefabSize(*P.m_TowerBase).x;
		const int Half = TowerSize / 2;
		const int SecondaryHalf = (P.m_TowerBase != nullptr) ? (PrefabSize(*P.m_TowerBase).x / 2) : 0;
		bool ShipPlaced = false;

		auto AddTowerBranch = [&](int a_Dir, int a_BranchY)
		{
			const sOrientedPrefab & Straight = P.m_Bridge[a_Dir];
			if (Straight.m_Prefab == nullptr)
			{
				return;
			}
			const int DirX = END_CITY_DIR_X[a_Dir];
			const int DirZ = END_CITY_DIR_Z[a_Dir];

			// The length of an oriented piece along the branch direction:
			auto LengthOf = [&](const sOrientedPrefab & a_Piece) -> int
			{
				if (a_Piece.m_Prefab == nullptr)
				{
					return 0;
				}
				const Vector3i Size = PrefabSize(*a_Piece.m_Prefab);
				return (DirX != 0) ? Size.x : Size.z;
			};

			// The bridge starts at the tower's outer wall. The staircase piece raises the far end
			// cumulatively, so bridges can reach towers at other heights:
			Vector3i Edge(TowerCenterX + (DirX * Half), a_BranchY, TowerCenterZ + (DirZ * Half));

			const int StraightCount = END_CITY_BRIDGE_MIN_SEGMENTS + static_cast<int>(Rng() % END_CITY_BRIDGE_EXTRA_SEGMENTS);
			for (int i = 0; i < StraightCount; i++)
			{
				Add(Straight.m_Prefab.get(), Vector3i(Edge.x + Straight.m_MinOffset.x, Edge.y, Edge.z + Straight.m_MinOffset.z));
				const int Step = LengthOf(Straight);
				Edge.x += DirX * Step;
				Edge.z += DirZ * Step;
			}

			const bool Gentle = ((Rng() % 2) == 0);
			const sOrientedPrefab & Stairs = Gentle ? P.m_BridgeGentle[a_Dir] : P.m_BridgeSteep[a_Dir];
			if (Stairs.m_Prefab != nullptr)
			{
				Add(Stairs.m_Prefab.get(), Vector3i(Edge.x + Stairs.m_MinOffset.x, Edge.y, Edge.z + Stairs.m_MinOffset.z));
				const int Step = LengthOf(Stairs);
				Edge.x += DirX * Step;
				Edge.z += DirZ * Step;
				Edge.y += StackHeightForName(Gentle ? "BridgeGentleStairs" : "BridgeSteepStairs") - 1;
			}

			const sOrientedPrefab & End = P.m_BridgeEnd[a_Dir];
			if (End.m_Prefab != nullptr)
			{
				Add(End.m_Prefab.get(), Vector3i(Edge.x + End.m_MinOffset.x, Edge.y, Edge.z + End.m_MinOffset.z));
				const int Step = LengthOf(End);
				Edge.x += DirX * Step;
				Edge.z += DirZ * Step;
			}

			// Open a doorway through the tower wall where the bridge meets it:
			AddCarve(TowerCenterX + (DirX * Half), a_BranchY + 1, TowerCenterZ + (DirZ * Half));

			// An End ship may end the bridge instead of another tower:
			const sOrientedPrefab & Ship = P.m_Ship[a_Dir];
			if (!ShipPlaced && (Ship.m_Prefab != nullptr) && ((Rng() % END_CITY_SHIP_DENOMINATOR) == 0))
			{
				Add(Ship.m_Prefab.get(), Vector3i(Edge.x + Ship.m_MinOffset.x, Edge.y, Edge.z + Ship.m_MinOffset.z), Ship.m_Chests, Ship.m_BrewingStands, Ship.m_ItemFrames, Ship.m_MobHeads);
				ShipPlaced = true;
				return;
			}

			// A loot room may end the bridge. The wiki gives one, two and three storey variants; the
			// two and three storey ones may carry a small tower on their roof:
			int RoomStoreys = 1;
			bool RoomTower = false;
			const sRotatedPrefab * RoomSet = nullptr;
			if ((Rng() % 4) == 0)
			{
				// The wiki notes that base_floor also forms the "empty rooms" found higher up:
				RoomSet = P.m_EmptyRoom;
			}
			else
			{
				RoomStoreys = 1 + static_cast<int>(Rng() % 3);
				RoomTower = (RoomStoreys >= 2) &&
					(P.m_TowerBase != nullptr) && (P.m_TowerPiece != nullptr) &&
					((Rng() % END_CITY_ROOM_TOWER_DENOMINATOR) == 0);

				if (RoomStoreys == 1)
				{
					RoomSet = P.m_LootRoom1;
				}
				else if (RoomStoreys == 2)
				{
					RoomSet = P.m_LootRoom2;
				}
				else
				{
					RoomSet = P.m_LootRoom3;
				}
			}

			// Rotate the room so that its doorway faces back toward the bridge:
			const int RequiredSide = (a_Dir + (END_CITY_DIR_COUNT / 2)) % END_CITY_DIR_COUNT;
			const sRotatedPrefab * Room = nullptr;
			for (int r = 0; r < END_CITY_DIR_COUNT; r++)
			{
				if ((RoomSet[r].m_Prefab != nullptr) && (RoomSet[r].m_DoorwaySide == RequiredSide))
				{
					Room = &RoomSet[r];
					break;
				}
			}
			if ((Room == nullptr) && (RoomSet[0].m_Prefab != nullptr))
			{
				Room = &RoomSet[0];
			}
			if (Room != nullptr)
			{
				// Line the room's doorway up with the bridge's last block. A room's roof is wider than
				// its lower walls, so centring on the bounding box would leave the bridge pointing at
				// the roof edge instead of at the opening:
				const int PosX = Edge.x - DirX - Room->m_DoorX;
				const int PosZ = Edge.z - DirZ - Room->m_DoorZ;
				Add(Room->m_Prefab.get(), Vector3i(PosX, Edge.y, PosZ), Room->m_Chests);

				// The taller rooms may carry a small tower on their roof. The tower continues the room's
				// own ladder column, so line the tower's ladder up with the room's ladder instead of
				// centring the tower on the room:
				if (RoomTower)
				{
					const AString RoomName = (RoomStoreys == 2) ? "LootRoom2" : "LootRoom3";
					const int TowerX = PosX + Room->m_LadderX - END_CITY_TOWER_LADDER_X;
					const int TowerZ = PosZ + Room->m_LadderZ - END_CITY_TOWER_LADDER_Z;
					// The tower's base carries the ladder entrance, so let it descend into the room below:
					const int RoomTopY = Edge.y + StackHeightForName(RoomName);
					Add(P.m_TowerBase.get(), Vector3i(TowerX, RoomTopY - END_CITY_SMALL_TOWER_LADDER_DEPTH, TowerZ));
					int TowerY = RoomTopY - END_CITY_SMALL_TOWER_LADDER_DEPTH + StackHeightForName("TowerBase");
					const int TowerStoreys = 3 + static_cast<int>(Rng() % 3);
					for (int i = 1; i < TowerStoreys; i++)
					{
						Add(P.m_TowerPiece.get(), Vector3i(TowerX, TowerY, TowerZ));
						TowerY += StackHeightForName("TowerPiece");
					}
					if (P.m_TowerTop != nullptr)
					{
						const int TopOffset = (PrefabSize(*P.m_TowerTop).x - PrefabSize(*P.m_TowerPiece).x) / 2;
						Add(P.m_TowerTop.get(), Vector3i(TowerX - TopOffset, TowerY, TowerZ - TopOffset));
					}
				}
				AddCarve(Edge.x, Edge.y + 1, Edge.z);
				return;
			}

			// Otherwise grow a small tower at the far end, connected through a doorway. Its bottom is
			// the solid-floored tower_floor, not the ladder entrance, so no hole opens to the void:
			if ((P.m_TowerFloor == nullptr) || (P.m_TowerPiece == nullptr))
			{
				return;
			}
			// Overlap the last bridge block by one so that the tower is not separated by a gap:
			const int Overlap = (SecondaryHalf == 0) ? 0 : (SecondaryHalf - 1);
			const Vector3i SecondaryCenter(Edge.x + (DirX * Overlap), Edge.y, Edge.z + (DirZ * Overlap));
			int SecondaryY = Edge.y;
			AddCentered(P.m_TowerFloor.get(), SecondaryCenter.x, SecondaryY, SecondaryCenter.z);
			SecondaryY += StackHeightForName("TowerFloor");
			const int SecondaryStoreys = 3 + static_cast<int>(Rng() % 3);
			for (int i = 1; i < SecondaryStoreys; i++)
			{
				AddCentered(P.m_TowerPiece.get(), SecondaryCenter.x, SecondaryY, SecondaryCenter.z);
				SecondaryY += StackHeightForName("TowerPiece");
			}
			if (P.m_TowerTop != nullptr)
			{
				AddCentered(P.m_TowerTop.get(), SecondaryCenter.x, SecondaryY, SecondaryCenter.z);
			}
			AddCarve(Edge.x, Edge.y + 1, Edge.z);
		};

		// Bridges leave only from the storeys the wiki documents:
		for (const int StoreyIndex: BranchStoreys)
		{
			if ((StoreyIndex < 0) || (StoreyIndex >= static_cast<int>(StoreyYs.size())))
			{
				continue;
			}
			for (int Dir = 0; Dir < END_CITY_DIR_COUNT; Dir++)
			{
				if ((Rng() % END_CITY_BRIDGE_DENOMINATOR) != 0)
				{
					continue;
				}
				AddTowerBranch(Dir, StoreyYs[StoreyIndex]);
			}
		}
	}
} ;





cEndCityGen::cEndCityGen(int a_Seed, cTerrainHeightGen & a_HeightGen) :
	Super(
		a_Seed,
		END_CITY_GRID_SIZE, END_CITY_GRID_SIZE,
		END_CITY_MAX_OFFSET, END_CITY_MAX_OFFSET,
		END_CITY_MAX_SIZE, END_CITY_MAX_SIZE,
		END_CITY_MAX_CACHE
	),
	m_HeightGen(a_HeightGen)
{
}





Vector3i cEndCityGen::GetCellOrigin(int a_Seed, int a_GridX, int a_GridZ)
{
	std::minstd_rand Rng(MakeCellSeed(a_Seed, a_GridX, a_GridZ));
	const int OriginX = a_GridX + (static_cast<int>(Rng() % END_CITY_ORIGIN_CHUNK_RANGE) * cChunkDef::Width);
	const int OriginZ = a_GridZ + (static_cast<int>(Rng() % END_CITY_ORIGIN_CHUNK_RANGE) * cChunkDef::Width);
	return Vector3i(OriginX, 0, OriginZ);
}





cGridStructGen::cStructurePtr cEndCityGen::CreateStructure(int a_GridX, int a_GridZ, int a_OriginX, int a_OriginZ)
{
	// cGridStructGen computes its own jittered origin; the vanilla origin range is different, so derive ours.
	// The unused parameters are part of the cGridStructGen interface:
	UNUSED(a_OriginX);
	UNUSED(a_OriginZ);
	const Vector3i Origin = GetCellOrigin(m_Seed, a_GridX, a_GridZ);

	// End cities only generate on the outer islands:
	if (Vector3d(Origin.x, 0, Origin.z).Length() <= END_CITY_MIN_DISTANCE)
	{
		return cStructurePtr();
	}
	return std::make_shared<cEndCity>(m_Seed, a_GridX, a_GridZ, Origin.x, Origin.z, m_HeightGen);
}
