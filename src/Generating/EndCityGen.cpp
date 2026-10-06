// EndCityGen.cpp

// Implements the cEndCityGen class representing the End City finisher generator

#include "Globals.h"
#include "EndCityGen.h"
#include "ComposableGenerator.h"
#include "Prefab.h"
#include "../BlockInfo.h"

#include <algorithm>
#include <memory>
#include <random>
#include <vector>





/** The grid cell size, in blocks. Vanilla places End cities once per 20 chunks. */
static constexpr int END_CITY_GRID_SIZE = 20 * cChunkDef::Width;

/** The maximum offset of the structure origin from the grid point, in blocks. */
static constexpr int END_CITY_MAX_OFFSET = 8 * cChunkDef::Width;

/** The maximum theoretical size of a city, in blocks. */
static constexpr int END_CITY_MAX_SIZE = 10 * cChunkDef::Width;

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

/** The minimum, and the extra random range, of a tower's middle segment count. */
static constexpr int END_CITY_TOWER_MIN_MIDDLE = 2;
static constexpr int END_CITY_TOWER_EXTRA_MIDDLE = 3;

/** The horizontal size of the two tower types. */
static constexpr int END_CITY_SMALL_TOWER_SIZE = 7;
static constexpr int END_CITY_FAT_TOWER_SIZE = 13;

/** The length (along its main axis) and the width of a bridge piece. */
static constexpr int END_CITY_BRIDGE_LENGTH = 7;
static constexpr int END_CITY_BRIDGE_WIDTH = 5;
static constexpr int END_CITY_BRIDGE_HEIGHT = 3;

/** The deck height and the top of the mast of the End ship. */
static constexpr int END_CITY_SHIP_DECK_Y = 5;
static constexpr int END_CITY_SHIP_MAST_TOP = 16;

/** The number of horizontal directions a tower can branch into. */
static constexpr int END_CITY_DIR_COUNT = 4;

/** The horizontal direction vectors used for branches. */
static const int END_CITY_DIR_X[END_CITY_DIR_COUNT] = {1, 0, -1, 0};
static const int END_CITY_DIR_Z[END_CITY_DIR_COUNT] = {0, 1, 0, -1};

/** Seed offsets used to decouple the layout randomness from the origin randomness. */
static constexpr int END_CITY_SEED_OFFSET_X = 7919;
static constexpr int END_CITY_SEED_OFFSET_Z = 104729;

/** End rod metadata for an upward-facing rod. */
static constexpr NIBBLETYPE END_CITY_END_ROD_UP = 1;





/** A prefab together with the oriented coordinate of its minimum corner.
Used when a prefab built along the +X axis is rotated into an arbitrary horizontal direction. */
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





/** Builds a square room: a solid floor, hollow perimeter walls, magenta glass windows, an optional
door on the -X face, and an optional ceiling with a stair parapet. */
static std::unique_ptr<cPrefab> MakeRoom(int a_Size, int a_WallHeight, bool a_HasDoor, bool a_HasWindows, bool a_HasCeiling)
{
	const int SizeY = a_WallHeight + 1 + (a_HasCeiling ? 2 : 0);
	cBlockArea Area;
	Area.Create(a_Size, SizeY, a_Size);

	// Floor:
	for (int x = 0; x < a_Size; x++)
	{
		for (int z = 0; z < a_Size; z++)
		{
			Area.SetRelBlockTypeMeta(x, 0, z, E_BLOCK_END_BRICKS, 0);
		}
	}

	// Walls:
	for (int y = 1; y <= a_WallHeight; y++)
	{
		for (int x = 0; x < a_Size; x++)
		{
			for (int z = 0; z < a_Size; z++)
			{
				const bool IsCorner = ((x == 0) || (x == a_Size - 1)) && ((z == 0) || (z == a_Size - 1));
				const bool IsPerimeter = (x == 0) || (z == 0) || (x == a_Size - 1) || (z == a_Size - 1);
				if (!IsPerimeter)
				{
					continue;
				}
				Area.SetRelBlockTypeMeta(x, y, z, IsCorner ? E_BLOCK_PURPUR_PILLAR : E_BLOCK_PURPUR_BLOCK, 0);
			}
		}
	}

	// Windows along the middle wall row:
	if (a_HasWindows)
	{
		const int WindowY = std::max(2, a_WallHeight - 1);
		for (int x = 2; x < a_Size - 1; x += 2)
		{
			Area.SetRelBlockTypeMeta(x, WindowY, 0, E_BLOCK_STAINED_GLASS, E_META_STAINED_GLASS_MAGENTA);
			Area.SetRelBlockTypeMeta(x, WindowY, a_Size - 1, E_BLOCK_STAINED_GLASS, E_META_STAINED_GLASS_MAGENTA);
		}
		for (int z = 2; z < a_Size - 1; z += 2)
		{
			Area.SetRelBlockTypeMeta(0, WindowY, z, E_BLOCK_STAINED_GLASS, E_META_STAINED_GLASS_MAGENTA);
			Area.SetRelBlockTypeMeta(a_Size - 1, WindowY, z, E_BLOCK_STAINED_GLASS, E_META_STAINED_GLASS_MAGENTA);
		}
	}

	// Door on the -X face:
	if (a_HasDoor)
	{
		const int DoorZ = a_Size / 2;
		Area.SetRelBlockTypeMeta(0, 1, DoorZ, E_BLOCK_AIR, 0);
		Area.SetRelBlockTypeMeta(0, 2, DoorZ, E_BLOCK_AIR, 0);
	}

	// Ceiling with a stair parapet and a corner end rod:
	if (a_HasCeiling)
	{
		const int CeilY = a_WallHeight + 1;
		for (int x = 0; x < a_Size; x++)
		{
			for (int z = 0; z < a_Size; z++)
			{
				Area.SetRelBlockTypeMeta(x, CeilY, z, E_BLOCK_PURPUR_BLOCK, 0);
			}
		}
		const int ParapetY = CeilY + 1;
		for (int x = 0; x < a_Size; x++)
		{
			Area.SetRelBlockTypeMeta(x, ParapetY, 0, E_BLOCK_PURPUR_STAIRS, E_BLOCK_STAIRS_ZM);
			Area.SetRelBlockTypeMeta(x, ParapetY, a_Size - 1, E_BLOCK_PURPUR_STAIRS, E_BLOCK_STAIRS_ZP);
		}
		for (int z = 0; z < a_Size; z++)
		{
			Area.SetRelBlockTypeMeta(0, ParapetY, z, E_BLOCK_PURPUR_STAIRS, E_BLOCK_STAIRS_XM);
			Area.SetRelBlockTypeMeta(a_Size - 1, ParapetY, z, E_BLOCK_PURPUR_STAIRS, E_BLOCK_STAIRS_XP);
		}
		Area.SetRelBlockTypeMeta(0, ParapetY, 0, E_BLOCK_END_ROD, END_CITY_END_ROD_UP);
		Area.SetRelBlockTypeMeta(a_Size - 1, ParapetY, 0, E_BLOCK_END_ROD, END_CITY_END_ROD_UP);
		Area.SetRelBlockTypeMeta(0, ParapetY, a_Size - 1, E_BLOCK_END_ROD, END_CITY_END_ROD_UP);
		Area.SetRelBlockTypeMeta(a_Size - 1, ParapetY, a_Size - 1, E_BLOCK_END_ROD, END_CITY_END_ROD_UP);
	}

	return std::make_unique<cPrefab>(Area);
}





/** Orients an area built along the +X axis into the given horizontal direction.
Returns the prefab together with the oriented coordinate of its minimum corner.
The input area's Z axis is its width; it is centered on Width / 2. */
static sOrientedPrefab OrientArea(const cBlockArea & a_Area, int a_DirX, int a_DirZ)
{
	const int Length = a_Area.GetSizeX();
	const int Width = a_Area.GetSizeZ();
	const int CenterZ = Width / 2;

	int MinX = 0;
	int MinZ = 0;
	int MaxX = 0;
	int MaxZ = 0;
	bool First = true;
	for (int x = 0; x < Length; x++)
	{
		for (int z = 0; z < Width; z++)
		{
			const int OffsetZ = z - CenterZ;
			const int OutX = (a_DirX * x) + (-a_DirZ * OffsetZ);
			const int OutZ = (a_DirZ * x) + (a_DirX * OffsetZ);
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
	for (int x = 0; x < Length; x++)
	{
		for (int y = 0; y < a_Area.GetSizeY(); y++)
		{
			for (int z = 0; z < Width; z++)
			{
				BLOCKTYPE Type;
				NIBBLETYPE Meta;
				a_Area.GetRelBlockTypeMeta(x, y, z, Type, Meta);
				if (Type == E_BLOCK_AIR)
				{
					continue;
				}
				const int OffsetZ = z - CenterZ;
				const int OutX = (a_DirX * x) + (-a_DirZ * OffsetZ);
				const int OutZ = (a_DirZ * x) + (a_DirX * OffsetZ);
				Out.SetRelBlockTypeMeta(OutX - MinX, y, OutZ - MinZ, Type, Meta);
			}
		}
	}
	return { std::make_unique<cPrefab>(Out), Vector3i(MinX, 0, MinZ) };
}





/** Builds a bridge along the +X axis: a purpur floor with purpur railing and pillar posts. */
static void BuildBridgeArea(cBlockArea & a_Area)
{
	const int Length = a_Area.GetSizeX();
	const int Width = a_Area.GetSizeZ();
	for (int x = 0; x < Length; x++)
	{
		for (int z = 0; z < Width; z++)
		{
			a_Area.SetRelBlockTypeMeta(x, 0, z, E_BLOCK_PURPUR_BLOCK, 0);
			if ((z == 0) || (z == Width - 1))
			{
				a_Area.SetRelBlockTypeMeta(x, 1, z, E_BLOCK_PURPUR_BLOCK, 0);
				a_Area.SetRelBlockTypeMeta(x, 2, z, E_BLOCK_PURPUR_PILLAR, 0);
			}
		}
	}
}





static sOrientedPrefab MakeBridge(int a_DirX, int a_DirZ)
{
	cBlockArea Area;
	Area.Create(END_CITY_BRIDGE_LENGTH, END_CITY_BRIDGE_HEIGHT, END_CITY_BRIDGE_WIDTH);
	BuildBridgeArea(Area);
	return OrientArea(Area, a_DirX, a_DirZ);
}





/** Builds the End ship along the +X axis: an obsidian-bottomed hull, a deck, a stern cabin and a mast. */
static void BuildShipArea(cBlockArea & a_Area)
{
	const int Length = a_Area.GetSizeX();
	const int Width = a_Area.GetSizeZ();

	// Hull bottom, doubling as the treasure room floor:
	for (int x = 0; x < Length; x++)
	{
		for (int z = 0; z < Width; z++)
		{
			a_Area.SetRelBlockTypeMeta(x, 0, z, E_BLOCK_OBSIDIAN, 0);
		}
	}

	// Hull walls:
	for (int y = 1; y < END_CITY_SHIP_DECK_Y; y++)
	{
		for (int x = 0; x < Length; x++)
		{
			for (int z = 0; z < Width; z++)
			{
				const bool IsPerimeter = (x == 0) || (z == 0) || (x == Length - 1) || (z == Width - 1);
				if (!IsPerimeter)
				{
					continue;
				}
				a_Area.SetRelBlockTypeMeta(x, y, z, E_BLOCK_PURPUR_BLOCK, 0);
			}
		}
	}

	// Deck:
	for (int x = 0; x < Length; x++)
	{
		for (int z = 0; z < Width; z++)
		{
			a_Area.SetRelBlockTypeMeta(x, END_CITY_SHIP_DECK_Y, z, E_BLOCK_PURPUR_BLOCK, 0);
		}
	}

	// Magenta glass windows:
	for (int x = 2; x < Length - 2; x += 3)
	{
		a_Area.SetRelBlockTypeMeta(x, 2, 0, E_BLOCK_STAINED_GLASS, E_META_STAINED_GLASS_MAGENTA);
		a_Area.SetRelBlockTypeMeta(x, 2, Width - 1, E_BLOCK_STAINED_GLASS, E_META_STAINED_GLASS_MAGENTA);
	}

	// Stern cabin:
	const int CabinMinX = Length - 6;
	const int CabinMaxX = Length - 2;
	const int CabinMinZ = 1;
	const int CabinMaxZ = Width - 2;
	for (int y = END_CITY_SHIP_DECK_Y + 1; y <= END_CITY_SHIP_DECK_Y + 4; y++)
	{
		for (int x = CabinMinX; x <= CabinMaxX; x++)
		{
			for (int z = CabinMinZ; z <= CabinMaxZ; z++)
			{
				const bool IsPerimeter = (x == CabinMinX) || (x == CabinMaxX) || (z == CabinMinZ) || (z == CabinMaxZ);
				if (!IsPerimeter)
				{
					continue;
				}
				a_Area.SetRelBlockTypeMeta(x, y, z, E_BLOCK_PURPUR_BLOCK, 0);
			}
		}
	}
	for (int x = CabinMinX; x <= CabinMaxX; x++)
	{
		for (int z = CabinMinZ; z <= CabinMaxZ; z++)
		{
			a_Area.SetRelBlockTypeMeta(x, END_CITY_SHIP_DECK_Y + 5, z, E_BLOCK_PURPUR_BLOCK, 0);
		}
	}
	a_Area.SetRelBlockTypeMeta(CabinMinX, END_CITY_SHIP_DECK_Y + 1, (CabinMinZ + CabinMaxZ) / 2, E_BLOCK_AIR, 0);

	// Mast and bow dragon head:
	for (int y = END_CITY_SHIP_DECK_Y + 1; y <= END_CITY_SHIP_MAST_TOP; y++)
	{
		a_Area.SetRelBlockTypeMeta(Length / 2, y, Width / 2, E_BLOCK_PURPUR_BLOCK, 0);
	}
	a_Area.SetRelBlockTypeMeta(0, END_CITY_SHIP_DECK_Y + 1, Width / 2, E_BLOCK_HEAD, E_META_HEAD_DRAGON);
}





static sOrientedPrefab MakeShip(int a_DirX, int a_DirZ)
{
	cBlockArea Area;
	Area.Create(21, END_CITY_SHIP_MAST_TOP + 1, 9);
	BuildShipArea(Area);
	return OrientArea(Area, a_DirX, a_DirZ);
}





/** Holds one prefab for every End City room type. Built once, then shared read-only by all cities. */
class cEndCityPieces
{
public:

	cEndCityPieces()
	{
		// Base floors, each wider than the one below it:
		m_BaseFloor[0] = MakeRoom(9, 4, true, true, false);
		m_BaseFloor[1] = MakeRoom(11, 4, true, true, false);
		m_BaseFloor[2] = MakeRoom(13, 4, true, true, false);
		for (int i = 0; i < 3; i++)
		{
			m_BaseFloor[i]->SetExtendFloorStrategy(cPrefab::efsRepeatBottomTillSolid);
		}

		m_Roof = MakeRoom(13, 0, false, false, true);

		// The two tower types and their top sections:
		m_SmallTowerBase = MakeRoom(END_CITY_SMALL_TOWER_SIZE, 4, true, false, false);
		m_SmallTowerPiece = MakeRoom(END_CITY_SMALL_TOWER_SIZE, 4, false, false, false);
		m_SmallTowerTop = MakeRoom(END_CITY_SMALL_TOWER_SIZE, 4, false, true, true);
		m_FatTowerBase = MakeRoom(END_CITY_FAT_TOWER_SIZE, 5, true, false, false);
		m_FatTowerMiddle = MakeRoom(END_CITY_FAT_TOWER_SIZE, 5, false, true, false);
		m_FatTowerTop = MakeRoom(END_CITY_FAT_TOWER_SIZE, 5, false, true, true);

		// One orientation variant per horizontal direction:
		for (int i = 0; i < END_CITY_DIR_COUNT; i++)
		{
			m_Bridge[i] = MakeBridge(END_CITY_DIR_X[i], END_CITY_DIR_Z[i]);
			m_Ship[i] = MakeShip(END_CITY_DIR_X[i], END_CITY_DIR_Z[i]);
		}
	}

	/** The three stacked base floors, from the narrowest to the widest. */
	std::unique_ptr<cPrefab> m_BaseFloor[3];

	/** The roof that caps the base floors and carries the tower. */
	std::unique_ptr<cPrefab> m_Roof;

	std::unique_ptr<cPrefab> m_SmallTowerBase;
	std::unique_ptr<cPrefab> m_SmallTowerPiece;
	std::unique_ptr<cPrefab> m_SmallTowerTop;
	std::unique_ptr<cPrefab> m_FatTowerBase;
	std::unique_ptr<cPrefab> m_FatTowerMiddle;
	std::unique_ptr<cPrefab> m_FatTowerTop;

	/** Bridge and ship prefabs, one per horizontal direction. */
	sOrientedPrefab m_Bridge[END_CITY_DIR_COUNT];
	sOrientedPrefab m_Ship[END_CITY_DIR_COUNT];
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

		// Stack the three base floors, then cap them with the roof:
		int Y = BaseY;
		for (int i = 0; i < 3; i++)
		{
			AddCentered(P.m_BaseFloor[i].get(), m_OriginX, Y, m_OriginZ);
			Y += PrefabSize(*P.m_BaseFloor[i]).y;
		}
		AddCentered(P.m_Roof.get(), m_OriginX, Y, m_OriginZ);
		Y += PrefabSize(*P.m_Roof).y;

		// Choose and stack a tower:
		const bool Fat = ((Rng() % 2) == 0);
		const int TowerMidY = Y;
		const int TowerSize = Fat ? END_CITY_FAT_TOWER_SIZE : END_CITY_SMALL_TOWER_SIZE;
		const cPrefab * TowerBase = Fat ? P.m_FatTowerBase.get() : P.m_SmallTowerBase.get();
		const cPrefab * TowerMiddle = Fat ? P.m_FatTowerMiddle.get() : P.m_SmallTowerPiece.get();
		const cPrefab * TowerTop = Fat ? P.m_FatTowerTop.get() : P.m_SmallTowerTop.get();
		AddCentered(TowerBase, m_OriginX, Y, m_OriginZ);
		Y += PrefabSize(*TowerBase).y;
		const int MiddleCount = END_CITY_TOWER_MIN_MIDDLE + static_cast<int>(Rng() % END_CITY_TOWER_EXTRA_MIDDLE);
		for (int i = 0; i < MiddleCount; i++)
		{
			AddCentered(TowerMiddle, m_OriginX, Y, m_OriginZ);
			Y += PrefabSize(*TowerMiddle).y;
		}
		AddCentered(TowerTop, m_OriginX, Y, m_OriginZ);

		// Branch each tower side into a bridge, at most one of which carries a ship:
		const int Half = TowerSize / 2;
		bool ShipPlaced = false;
		for (int Dir = 0; Dir < END_CITY_DIR_COUNT; Dir++)
		{
			if ((Rng() % END_CITY_BRIDGE_DENOMINATOR) != 0)
			{
				continue;
			}
			const int DirX = END_CITY_DIR_X[Dir];
			const int DirZ = END_CITY_DIR_Z[Dir];
			const sOrientedPrefab & Bridge = P.m_Bridge[Dir];
			const Vector3i Edge(m_OriginX + (DirX * Half), TowerMidY, m_OriginZ + (DirZ * Half));
			const Vector3i BridgePos(Edge.x + Bridge.m_MinOffset.x, TowerMidY, Edge.z + Bridge.m_MinOffset.z);
			Add(Bridge.m_Prefab.get(), BridgePos);

			if (ShipPlaced || ((Rng() % END_CITY_SHIP_DENOMINATOR) != 0))
			{
				continue;
			}
			const sOrientedPrefab & Ship = P.m_Ship[Dir];
			const Vector3i BridgeFar(
				Edge.x + (DirX * (END_CITY_BRIDGE_LENGTH - 1)),
				TowerMidY,
				Edge.z + (DirZ * (END_CITY_BRIDGE_LENGTH - 1))
			);
			const Vector3i ShipEdge(BridgeFar.x + DirX, TowerMidY, BridgeFar.z + DirZ);
			const Vector3i ShipPos(ShipEdge.x + Ship.m_MinOffset.x, TowerMidY, ShipEdge.z + Ship.m_MinOffset.z);
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
