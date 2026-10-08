// ConnectorRotationTest.cpp

// Implements the geometric consistency test for rotating a piece's connectors

/*
The rotation tables in cPiece::cConnector are pure functions whose ground truth is cPiece::RotatePos():
a rotated connector's position and direction must stay consistent. This test therefore derives the expectation
from the rotated position instead of copying the tables, so a wrong table entry fails here rather than being
mirrored by the test.

Why the existing PieceRotation test never caught such an entry: it checks algebraic properties only - that CW
and CCW are inverses, that four rotations are the identity and that each rotation is a permutation. The
180-degree table returned both Z-facing directions unchanged, which is still invertible, still the identity
after two applications and still a permutation; it only disagreed with the geometry.

Invariants checked for every connector and every rotation (0 .. 3, whether or not the piece allows it - the
tables are total functions):

	1. Stepping one block along the rotated direction leaves the rotated bounding box. The piece generator
	   places the neighbour's connector exactly at that point; when it lands inside the box, every candidate
	   piece intersects the parent's hitbox, so the structure cannot grow at all.
	2. Every axis the direction refers to is saturated with the matching sign: a z- connector sits at or
	   beyond the box's z- face, never inside it.

See specs/piece-connector-rotation.md for the specification, and specs/vanilla-1.12.2-stronghold.md section 5.7
for how the 180-degree defect was found (a stronghold whose starting piece drew rotation 2 was stubbed).
*/

#include "Globals.h"
#include <set>
#include "Generating/PiecePool.h"
#include "Generating/Prefab.h"
#include "Generating/PrefabPiecePool.h"
#include "OSSupport/File.h"
#include "StringUtils.h"
#include "../TestHelpers.h"

/** Shortcut for the connector direction type. */
using eDirection = cPiece::cConnector::eDirection;

/** A cubeset with a single 3x3x3 piece holding one connector in each of the 14 directions.
Each connector sits on the face, edge or corner that its direction refers to, so every direction is checked
whether or not the production data happens to use it. */
static const char * TEST_ALL_DIRECTIONS_CUBESET = R"(
Cubeset =
{
	Metadata =
	{
		CubesetFormatVersion = 1,
		["IntendedUse"] = "PieceStructures",
	},

	Pieces =
	{
		{
			Size =
			{
				x = 3,
				y = 3,
				z = 3,
			},
			Hitbox =
			{
				MinX = 0,
				MinY = 0,
				MinZ = 0,
				MaxX = 2,
				MaxY = 2,
				MaxZ = 2,
			},
			BlockDefinitions =
			{
				"a: 1: 0",
			},
			BlockData =
			{
				"aaa", "aaa", "aaa",
				"aaa", "aaa", "aaa",
				"aaa", "aaa", "aaa",
			},
			Connectors =
			{
				{ Type = 0, RelX = 0, RelY = 1, RelZ = 1, Direction = "x-" },
				{ Type = 0, RelX = 2, RelY = 1, RelZ = 1, Direction = "x+" },
				{ Type = 0, RelX = 1, RelY = 0, RelZ = 1, Direction = "y-" },
				{ Type = 0, RelX = 1, RelY = 2, RelZ = 1, Direction = "y+" },
				{ Type = 0, RelX = 1, RelY = 1, RelZ = 0, Direction = "z-" },
				{ Type = 0, RelX = 1, RelY = 1, RelZ = 2, Direction = "z+" },
				{ Type = 0, RelX = 0, RelY = 0, RelZ = 0, Direction = "y-x-z-" },
				{ Type = 0, RelX = 0, RelY = 0, RelZ = 2, Direction = "y-x-z+" },
				{ Type = 0, RelX = 2, RelY = 0, RelZ = 0, Direction = "y-x+z-" },
				{ Type = 0, RelX = 2, RelY = 0, RelZ = 2, Direction = "y-x+z+" },
				{ Type = 0, RelX = 0, RelY = 2, RelZ = 0, Direction = "y+x-z-" },
				{ Type = 0, RelX = 0, RelY = 2, RelZ = 2, Direction = "y+x-z+" },
				{ Type = 0, RelX = 2, RelY = 2, RelZ = 0, Direction = "y+x+z-" },
				{ Type = 0, RelX = 2, RelY = 2, RelZ = 2, Direction = "y+x+z+" },
			},
			Metadata =
			{
				["DefaultWeight"] = "100",
				["AllowedRotations"] = "7",
				["MergeStrategy"] = "msSpongePrint",
				["IsStarting"] = "1",
				["VerticalStrategy"] = "Fixed|150",
			},
		},
	},
}
)";

/** The axes a connector direction refers to, and the sign it points along each of them. */
struct sDirectionAxes
{
	bool m_HasX;
	int  m_SignX;
	bool m_HasY;
	int  m_SignY;
	bool m_HasZ;
	int  m_SignZ;
};





/** Returns the axes the specified direction refers to, as documented on cPiece::cConnector::eDirection. */
static sDirectionAxes getDirectionAxes(eDirection a_Direction)
{
	switch (a_Direction)
	{
		case eDirection::dirXM:       return {true, -1, false,  0, false,  0};
		case eDirection::dirXP:       return {true, +1, false,  0, false,  0};
		case eDirection::dirYM:       return {false, 0, true,  -1, false,  0};
		case eDirection::dirYP:       return {false, 0, true,  +1, false,  0};
		case eDirection::dirZM:       return {false, 0, false,  0, true,  -1};
		case eDirection::dirZP:       return {false, 0, false,  0, true,  +1};
		case eDirection::dirYM_XM_ZM: return {true, -1, true,  -1, true,  -1};
		case eDirection::dirYM_XM_ZP: return {true, -1, true,  -1, true,  +1};
		case eDirection::dirYM_XP_ZM: return {true, +1, true,  -1, true,  -1};
		case eDirection::dirYM_XP_ZP: return {true, +1, true,  -1, true,  +1};
		case eDirection::dirYP_XM_ZM: return {true, -1, true,  +1, true,  -1};
		case eDirection::dirYP_XM_ZP: return {true, -1, true,  +1, true,  +1};
		case eDirection::dirYP_XP_ZM: return {true, +1, true,  +1, true,  -1};
		case eDirection::dirYP_XP_ZP: return {true, +1, true,  +1, true,  +1};
	}
	TEST_FAIL(fmt::format("Unknown connector direction {}", static_cast<int>(a_Direction)));
	return {false, 0, false, 0, false, 0};
}





/** Checks that the coordinate on the specified axis is saturated in the direction's sense.
a_Sign is -1 for the box's minimum (the "-" side) and +1 for its maximum. */
static void checkAxis(const AString & a_Where, const char * a_Axis, int a_Coord, int a_Min, int a_Max, int a_Sign)
{
	const int Boundary = (a_Sign < 0) ? a_Min : a_Max;
	const bool Ok = (a_Sign < 0) ? (a_Coord <= Boundary) : (a_Coord >= Boundary);
	TEST_EQUAL_MSG(Ok, true, fmt::format("{}: the connector sits at {}={}, which is inside the box ({} .. {}) instead of on or beyond the {}{} side",
		a_Where, a_Axis, a_Coord, a_Min, a_Max, a_Axis, (a_Sign < 0) ? '-' : '+'
	));
}





/** Checks both invariants for one connector rotated by the specified number of CCW rotations. */
static void checkRotatedConnector(const cPiece & a_Piece, const cPiece::cConnector & a_Connector, int a_NumCCWRotations, const AString & a_Where)
{
	cCuboid Box = a_Piece.RotateMoveHitBox(a_NumCCWRotations, 0, 0, 0);
	Box.Sort();
	const auto Rotated = a_Piece.RotateMoveConnector(a_Connector, a_NumCCWRotations, 0, 0, 0);
	const Vector3i Next = cPiece::cConnector::AddDirection(Rotated.m_Pos, Rotated.m_Direction);
	const AString Where = fmt::format("{}, connector ({}, {}, {}) type {} direction {} rotated by {}",
		a_Where, a_Connector.m_Pos.x, a_Connector.m_Pos.y, a_Connector.m_Pos.z,
		a_Connector.m_Type, static_cast<int>(a_Connector.m_Direction), a_NumCCWRotations
	);

	// Invariant 1 - the neighbour's connector must be placed outside this piece's box:
	TEST_EQUAL_MSG(Box.IsInside(Next), false, fmt::format(
		"{}: stepping along the rotated direction reaches ({}, {}, {}), which is inside the box ({}, {}, {}) .. ({}, {}, {})",
		Where, Next.x, Next.y, Next.z, Box.p1.x, Box.p1.y, Box.p1.z, Box.p2.x, Box.p2.y, Box.p2.z
	));

	// Invariant 2 - every axis the direction refers to must be saturated in the matching sense:
	const sDirectionAxes Axes = getDirectionAxes(Rotated.m_Direction);
	if (Axes.m_HasX)
	{
		checkAxis(Where, "x", Rotated.m_Pos.x, Box.p1.x, Box.p2.x, Axes.m_SignX);
	}
	if (Axes.m_HasY)
	{
		checkAxis(Where, "y", Rotated.m_Pos.y, Box.p1.y, Box.p2.y, Axes.m_SignY);
	}
	if (Axes.m_HasZ)
	{
		checkAxis(Where, "z", Rotated.m_Pos.z, Box.p1.z, Box.p2.z, Axes.m_SignZ);
	}
}





/** Checks every connector of the piece in all four rotations. */
static void checkPiece(const cPiece & a_Piece, const AString & a_Where)
{
	for (const auto & Connector : a_Piece.GetConnectors())
	{
		for (int NumRotations = 0; NumRotations < 4; NumRotations++)
		{
			checkRotatedConnector(a_Piece, Connector, NumRotations, a_Where);
		}
	}
}





/** Returns all .cubeset files in the specified folder, recursively. */
static AStringVector getAllCubesets(const AString & a_Folder)
{
	AStringVector res;
	for (const auto & Entry : cFile::GetFolderContents(a_Folder))
	{
		const AString Path = a_Folder + "/" + Entry;
		if (cFile::IsFolder(Path))
		{
			AStringVector Sub = getAllCubesets(Path);
			res.insert(res.end(), Sub.begin(), Sub.end());
		}
		else if ((Path.length() > 8) && (Path.substr(Path.length() - 8) == ".cubeset"))
		{
			res.push_back(Path);
		}
	}
	return res;
}





/** Collects every piece reachable from the pool's starting pieces through its connectors.
Pieces that no structure can ever place do not affect generation, so they are not checked. */
static std::vector<const cPiece *> getReachablePieces(cPrefabPiecePool & a_Pool)
{
	std::vector<const cPiece *> res;
	std::set<const cPiece *> Seen;
	std::vector<const cPiece *> Queue;
	for (const auto * Piece : a_Pool.GetStartingPieces())
	{
		Queue.push_back(Piece);
	}
	while (!Queue.empty())
	{
		const cPiece * Piece = Queue.back();
		Queue.pop_back();
		if (!Seen.insert(Piece).second)
		{
			continue;
		}
		res.push_back(Piece);
		for (const auto & Connector : Piece->GetConnectors())
		{
			for (const int Type : {Connector.m_Type, -Connector.m_Type})
			{
				for (const auto * Next : a_Pool.GetPiecesWithConnector(Type))
				{
					Queue.push_back(Next);
				}
			}
		}
	}
	return res;
}





/** Verifies both invariants on a synthetic piece that uses all 14 directions. */
static void testAllDirections()
{
	cPrefabPiecePool Pool;
	TEST_TRUE(Pool.LoadFromCubeset(TEST_ALL_DIRECTIONS_CUBESET, "AllDirections.cubeset", true));

	// Starting pieces are not entered into the per-connector map, so take the piece from the starting list:
	int NumChecked = 0;
	for (const auto * Piece : Pool.GetStartingPieces())
	{
		checkPiece(*Piece, "the synthetic all-directions cubeset");
		NumChecked += static_cast<int>(Piece->GetConnectors().size());
	}
	TEST_EQUAL_MSG(NumChecked, 14, "the synthetic piece must provide one connector per direction");
}





/** Verifies both invariants on every connector of every piece reachable in the production cubesets. */
static void testProductionCubesets()
{
	int NumCubesets = 0;
	for (const auto & FileName : getAllCubesets("Prefabs"))
	{
		cPrefabPiecePool Pool;
		if (!Pool.LoadFromFile(FileName, true))
		{
			// Some prefabs are intentionally not piece structures (SinglePieceStructures, villages, ...);
			// their loading is covered by the tests that own them, so only check what did load:
			continue;
		}
		NumCubesets += 1;
		for (const auto * Piece : getReachablePieces(Pool))
		{
			checkPiece(*Piece, FileName);
		}
	}
	TEST_GREATER_THAN_OR_EQUAL(NumCubesets, 1);
}





IMPLEMENT_TEST_MAIN("ConnectorRotationTest",
	testAllDirections();
	testProductionCubesets();
)
