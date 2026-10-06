// EndCityGen.cpp

// Implements the cEndCityGen class representing the End City finisher generator

#include "Globals.h"
#include "EndCityGen.h"
#include "ComposableGenerator.h"
#include "EndCityBlueprintData.h"
#include "Prefab.h"
#include "../BlockInfo.h"
#include "../StringUtils.h"

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

/** The number of straight bridge segments, and the extra random range. */
static constexpr int END_CITY_BRIDGE_MIN_SEGMENTS = 1;
static constexpr int END_CITY_BRIDGE_EXTRA_SEGMENTS = 2;

/** How far the small tower's ladder shaft descends into the room below. */
static constexpr int END_CITY_SMALL_TOWER_LADDER_DEPTH = 3;

/** The minimum, and the extra random range, of a tower's middle section count. */
static constexpr int END_CITY_TOWER_MIN_MIDDLE = 2;
static constexpr int END_CITY_TOWER_EXTRA_MIDDLE = 3;

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

/** Purpur pillar axis metadata. */
static constexpr NIBBLETYPE END_CITY_PILLAR_VERTICAL = 0;
static constexpr NIBBLETYPE END_CITY_PILLAR_X = 4;
static constexpr NIBBLETYPE END_CITY_PILLAR_Z = 8;

/** Purpur slab half metadata. */
static constexpr NIBBLETYPE END_CITY_SLAB_BOTTOM = 0;
static constexpr NIBBLETYPE END_CITY_SLAB_TOP = 8;





/** A prefab together with the oriented coordinate of its minimum corner.
Used when a blueprint built along the +Z axis is rotated into an arbitrary horizontal direction. */
struct sOrientedPrefab
{
	/** The prefab itself. */
	std::unique_ptr<cPrefab> m_Prefab;

	/** The oriented coordinate that ends up at the prefab's minimum corner. */
	Vector3i m_MinOffset;
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

	// Entity sprites and the wool reference markers are not blocks:
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
		return true;
	}
	if (Name == "Ender Chest")
	{
		a_Type = E_BLOCK_ENDER_CHEST;
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





/** Crops the area to its non-air bounding box. Returns false if the area is empty. */
static bool CropToContent(cBlockArea & a_Area)
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
	a_Area.Crop(
		MinX, a_Area.GetSizeX() - 1 - MaxX,
		MinY, a_Area.GetSizeY() - 1 - MaxY,
		MinZ, a_Area.GetSizeZ() - 1 - MaxZ
	);
	return true;
}





/** Builds a block area from a blueprint, cropped to its non-air bounding box.
Returns nullptr if the blueprint holds no blocks. */
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

	if (!CropToContent(*Area))
	{
		return nullptr;
	}
	return Area;
}





/** Orients an area whose length runs along +Z into the given horizontal direction.
Returns the prefab together with the oriented coordinate of its minimum corner. */
static sOrientedPrefab OrientAreaZ(const cBlockArea & a_Area, int a_DirX, int a_DirZ)
{
	const int Length = a_Area.GetSizeZ();
	const int Width = a_Area.GetSizeX();
	const int CenterX = Width / 2;

	int MinX = 0;
	int MinZ = 0;
	int MaxX = 0;
	int MaxZ = 0;
	bool First = true;
	for (int z = 0; z < Length; z++)
	{
		for (int x = 0; x < Width; x++)
		{
			const int OffsetX = x - CenterX;
			const int OutX = (a_DirX * z) + (-a_DirZ * OffsetX);
			const int OutZ = (a_DirZ * z) + (a_DirX * OffsetX);
			if (First)
			{
				MinX = MaxX = OutX;
				MinZ = MaxZ = OutZ;
				First = false;
				continue;
			}
			MinX = std::min(MinX, OutX);
			MaxX = std::max(MaxX, OutX);
			MinZ = std::min(MinZ, OutZ);
			MaxZ = std::max(MaxZ, OutZ);
		}
	}

	cBlockArea Out;
	Out.Create(MaxX - MinX + 1, a_Area.GetSizeY(), MaxZ - MinZ + 1);
	for (int z = 0; z < Length; z++)
	{
		for (int y = 0; y < a_Area.GetSizeY(); y++)
		{
			for (int x = 0; x < Width; x++)
			{
				BLOCKTYPE Type;
				NIBBLETYPE Meta;
				a_Area.GetRelBlockTypeMeta(x, y, z, Type, Meta);
				if (Type == E_BLOCK_AIR)
				{
					continue;
				}
				const int OffsetX = x - CenterX;
				const int OutX = (a_DirX * z) + (-a_DirZ * OffsetX);
				const int OutZ = (a_DirZ * z) + (a_DirX * OffsetX);
				Out.SetRelBlockTypeMeta(OutX - MinX, y, OutZ - MinZ, Type, Meta);
			}
		}
	}
	auto Prefab = std::make_unique<cPrefab>(Out);
	Prefab->SetMergeStrategy(cBlockArea::msImprint);
	return { std::move(Prefab), Vector3i(MinX, 0, MinZ) };
}





/** Holds one prefab for every End City piece used by the generator. */
class cEndCityPieces
{
public:

	cEndCityPieces()
	{
		m_EmptyRoom = MakePrefab("EmptyRoom");
		m_BaseRoom = MakePrefab("BaseRoom");
		m_SmallTowerBase = MakePrefab("SmallTowerBase");
		m_SmallTowerExtension = MakePrefab("SmallTowerExtension");
		m_LargeTower = MakePrefab("LargeTower");
		m_SmallRoom = MakePrefab("SmallRoom");
		m_LootRoom = MakePrefab("LootRoom");
		m_LargeRoomTwoStorey = MakePrefab("LargeRoomTwoStorey");
		m_LargeRoomThreeStorey = MakePrefab("LargeRoomThreeStorey");

		// Extend the bottom piece down to the terrain so that slopes do not leave a gap:
		if (m_EmptyRoom != nullptr)
		{
			m_EmptyRoom->SetExtendFloorStrategy(cPrefab::efsRepeatBottomTillSolid);
		}
		else if (m_BaseRoom != nullptr)
		{
			m_BaseRoom->SetExtendFloorStrategy(cPrefab::efsRepeatBottomTillSolid);
		}

		// The bridge and the ship are authored along +Z, so build one orientation per direction:
		const sEndCityBlueprint * BridgeBlueprint = FindBlueprint("Bridge");
		const sEndCityBlueprint * ShipBlueprint = FindBlueprint("Ship");
		auto BridgeArea = (BridgeBlueprint != nullptr) ? MakeBlueprintArea(*BridgeBlueprint) : nullptr;
		auto ShipArea = (ShipBlueprint != nullptr) ? MakeBlueprintArea(*ShipBlueprint) : nullptr;
		for (int i = 0; i < END_CITY_DIR_COUNT; i++)
		{
			if (BridgeArea != nullptr)
			{
				m_Bridge[i] = OrientAreaZ(*BridgeArea, END_CITY_DIR_X[i], END_CITY_DIR_Z[i]);
			}
			if (ShipArea != nullptr)
			{
				m_Ship[i] = OrientAreaZ(*ShipArea, END_CITY_DIR_X[i], END_CITY_DIR_Z[i]);
			}
		}
	}

	std::unique_ptr<cPrefab> m_EmptyRoom;
	std::unique_ptr<cPrefab> m_BaseRoom;
	std::unique_ptr<cPrefab> m_SmallTowerBase;
	std::unique_ptr<cPrefab> m_SmallTowerExtension;
	std::unique_ptr<cPrefab> m_LargeTower;
	std::unique_ptr<cPrefab> m_SmallRoom;
	std::unique_ptr<cPrefab> m_LootRoom;
	std::unique_ptr<cPrefab> m_LargeRoomTwoStorey;
	std::unique_ptr<cPrefab> m_LargeRoomThreeStorey;

	sOrientedPrefab m_Bridge[END_CITY_DIR_COUNT];
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
	}

protected:

	/** A single prefab placed at absolute world coordinates. */
	struct sPlacedPiece
	{
		const cPrefab * m_Prefab;
		Vector3i m_Pos;
	} ;

	/** All the pieces of this city. */
	std::vector<sPlacedPiece> m_Pieces;


	/** Adds a piece at the specified minimum-corner coordinates. */
	void Add(const cPrefab * a_Prefab, const Vector3i & a_Pos)
	{
		m_Pieces.push_back({a_Prefab, a_Pos});
	}

	/** Adds a piece centered horizontally on the specified coordinates. */
	void AddCentered(const cPrefab * a_Prefab, int a_CenterX, int a_Y, int a_CenterZ)
	{
		const Vector3i Size = PrefabSize(*a_Prefab);
		Add(a_Prefab, Vector3i(a_CenterX - (Size.x / 2), a_Y, a_CenterZ - (Size.z / 2)));
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

		// The entrance and the base room:
		int Y = BaseY;
		if (P.m_EmptyRoom != nullptr)
		{
			AddCentered(P.m_EmptyRoom.get(), m_OriginX, Y, m_OriginZ);
			Y += PrefabSize(*P.m_EmptyRoom).y;
		}
		if (P.m_BaseRoom != nullptr)
		{
			AddCentered(P.m_BaseRoom.get(), m_OriginX, Y, m_OriginZ);

			// The base room's top layer is only decorative end rods; let the tower sit on its roof:
			Y += PrefabSize(*P.m_BaseRoom).y - 1;
		}

		// Choose and stack a tower:
		const bool Fat = ((Rng() % 2) == 0);
		const int TowerBaseY = Y;
		if (Fat && (P.m_LargeTower != nullptr))
		{
			const int Repeats = END_CITY_TOWER_MIN_MIDDLE + static_cast<int>(Rng() % END_CITY_TOWER_EXTRA_MIDDLE);
			for (int i = 0; i < Repeats; i++)
			{
				AddCentered(P.m_LargeTower.get(), m_OriginX, Y, m_OriginZ);
				Y += PrefabSize(*P.m_LargeTower).y;
			}

			// Cap the fat tower with its loot room:
			if (P.m_LootRoom != nullptr)
			{
				AddCentered(P.m_LootRoom.get(), m_OriginX, Y, m_OriginZ);
				Y += PrefabSize(*P.m_LootRoom).y;
			}
		}
		else if (!Fat && (P.m_SmallTowerBase != nullptr))
		{
			// The small tower base starts with a ladder shaft, let it descend into the room below:
			AddCentered(P.m_SmallTowerBase.get(), m_OriginX, Y - END_CITY_SMALL_TOWER_LADDER_DEPTH, m_OriginZ);
			Y = Y - END_CITY_SMALL_TOWER_LADDER_DEPTH + PrefabSize(*P.m_SmallTowerBase).y;
			const int Repeats = END_CITY_TOWER_MIN_MIDDLE + static_cast<int>(Rng() % END_CITY_TOWER_EXTRA_MIDDLE);
			for (int i = 0; i < Repeats; i++)
			{
				if (P.m_SmallTowerExtension == nullptr)
				{
					break;
				}
				AddCentered(P.m_SmallTowerExtension.get(), m_OriginX, Y, m_OriginZ);
				Y += PrefabSize(*P.m_SmallTowerExtension).y;
			}

			// Cap the small tower with a small room:
			if (P.m_SmallRoom != nullptr)
			{
				AddCentered(P.m_SmallRoom.get(), m_OriginX, Y, m_OriginZ);
				Y += PrefabSize(*P.m_SmallRoom).y;
			}
		}

		// Branch each tower side into a bridge, at most one of which carries a ship:
		const int TowerSize = (Fat ? PrefabSize(*P.m_LargeTower).x : PrefabSize(*P.m_SmallTowerBase).x);
		const int Half = TowerSize / 2;
		const int BridgeY = TowerBaseY;
		bool ShipPlaced = false;
		for (int Dir = 0; Dir < END_CITY_DIR_COUNT; Dir++)
		{
			if ((Rng() % END_CITY_BRIDGE_DENOMINATOR) != 0)
			{
				continue;
			}
			const sOrientedPrefab & Bridge = P.m_Bridge[Dir];
			if (Bridge.m_Prefab == nullptr)
			{
				continue;
			}
			const int DirX = END_CITY_DIR_X[Dir];
			const int DirZ = END_CITY_DIR_Z[Dir];
			const int SegmentCount = END_CITY_BRIDGE_MIN_SEGMENTS + static_cast<int>(Rng() % END_CITY_BRIDGE_EXTRA_SEGMENTS);
			const Vector3i BridgeSize = PrefabSize(*Bridge.m_Prefab);
			const int SegmentLength = (DirX != 0) ? BridgeSize.x : BridgeSize.z;

			Vector3i Edge(m_OriginX + (DirX * Half), BridgeY, m_OriginZ + (DirZ * Half));
			for (int Segment = 0; Segment < SegmentCount; Segment++)
			{
				const Vector3i BridgePos(Edge.x + Bridge.m_MinOffset.x, BridgeY, Edge.z + Bridge.m_MinOffset.z);
				Add(Bridge.m_Prefab.get(), BridgePos);
				Edge.x += DirX * SegmentLength;
				Edge.z += DirZ * SegmentLength;
			}

			// Ship at the far end of the bridge:
			const sOrientedPrefab & Ship = P.m_Ship[Dir];
			if (ShipPlaced || (Ship.m_Prefab == nullptr) || ((Rng() % END_CITY_SHIP_DENOMINATOR) != 0))
			{
				continue;
			}
			const Vector3i ShipPos(Edge.x + Ship.m_MinOffset.x, BridgeY, Edge.z + Ship.m_MinOffset.z);
			Add(Ship.m_Prefab.get(), ShipPos);
			ShipPlaced = true;
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
