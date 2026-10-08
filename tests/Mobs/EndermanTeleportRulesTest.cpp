#include "Globals.h"
#include "Mobs/EndermanTeleportRules.h"
#include "../TestHelpers.h"

/** What this test verifies:
(1) The chase-teleport geometry of specs/vanilla-1.12.2-enderman.md section 3.6.2 -
the aim point of a chasing enderman's teleport is the point 16 blocks horizontally and
17 blocks vertically towards the target, snapped onto the target's own XZ / Y when closer;
(2) The sunlight-teleport per-tick probability formula of section 3.6.3,
P = (11 * Li - 120) / (225 * (20 - Li)) for internal light level Li >= 13, and 0 below.

Invariants:
- The aim point never exceeds TELEPORT_CHASE_HORIZONTAL_RANGE / TELEPORT_CHASE_VERTICAL_RANGE
  towards the target;
- Closer targets snap the aim point onto their coordinates;
- Horizontal and vertical clamping are independent of each other;
- The sunlight chance is zero below the threshold and strictly increasing above it.

Why: these constants and the formula are the only numerically testable parts of the teleport
behaviour without a world harness, so they are extracted into pure functions. */

/** Returns true if the two values differ by less than the given epsilon. */
static bool IsCloseTo(double a_Value, double a_Other, double a_Epsilon = 1e-9)
{
	return std::abs(a_Value - a_Other) < a_Epsilon;
}





/** Tests the horizontal clamping of the chase teleport center. */
static void testHorizontal(void)
{
	// Target exactly at range:
	TEST_EQUAL(EndermanChaseTeleportCenter(Vector3d(10, 70, 10), Vector3d(42, 70, 10)), Vector3d(26, 70, 10));

	// Target at exactly 16 blocks is not "beyond the range", it snaps onto the target:
	TEST_EQUAL(EndermanChaseTeleportCenter(Vector3d(0, 70, 0), Vector3d(0, 70, -16)), Vector3d(0, 70, -16));

	// Target within range snaps onto its XZ:
	TEST_EQUAL(EndermanChaseTeleportCenter(Vector3d(0, 70, 0), Vector3d(3, 70, 4)), Vector3d(3, 70, 4));

	// Negative direction clamps the same way:
	TEST_EQUAL(EndermanChaseTeleportCenter(Vector3d(42, 70, 10), Vector3d(10, 70, 10)), Vector3d(26, 70, 10));

	// Diagonal target 32*sqrt(2) away horizontally: the result is 16 blocks towards it on the XZ plane,
	// split equally between the two equal axes:
	const Vector3d Center = EndermanChaseTeleportCenter(Vector3d(0, 70, 0), Vector3d(32, 70, 32));
	TEST_TRUE(IsCloseTo(Center.x, Center.z));
	TEST_TRUE(IsCloseTo(Center.x * Center.x + Center.z * Center.z, TELEPORT_CHASE_HORIZONTAL_RANGE * TELEPORT_CHASE_HORIZONTAL_RANGE));
}





/** Tests the vertical clamping of the chase teleport center. */
static void testVertical(void)
{
	// Target far below, same column: vertical clamps, XZ snaps onto the target:
	TEST_EQUAL(EndermanChaseTeleportCenter(Vector3d(5, 70, 5), Vector3d(5, 40, 5)), Vector3d(5, 53, 5));

	// Target far above:
	TEST_EQUAL(EndermanChaseTeleportCenter(Vector3d(5, 70, 5), Vector3d(5, 100, 5)), Vector3d(5, 87, 5));

	// Vertical distance within the range snaps onto the target's Y...
	TEST_EQUAL(EndermanChaseTeleportCenter(Vector3d(0, 70, 0), Vector3d(32, 80, 0)).y, 80);

	// ...independently of the horizontal clamping happening in the same call:
	TEST_EQUAL(EndermanChaseTeleportCenter(Vector3d(0, 70, 0), Vector3d(32, 80, 0)).x, 16);

	// Diagonal in vertical direction as well (64 chosen so that the scaling is exact in binary floats):
	const Vector3d Center = EndermanChaseTeleportCenter(Vector3d(0, 70, 0), Vector3d(0, 130, 64));
	TEST_EQUAL(Center.y, 70 + TELEPORT_CHASE_VERTICAL_RANGE);
	TEST_EQUAL(Center.x, 0);
	TEST_EQUAL(Center.z, 16);
}





/** Tests the degenerate case of the target standing on the enderman. */
static void testCoincident(void)
{
	// Zero distance: the center is the target itself, no division by zero:
	TEST_EQUAL(EndermanChaseTeleportCenter(Vector3d(7, 70, 3), Vector3d(7, 70, 3)), Vector3d(7, 70, 3));
}





/** Tests the per-tick probability formula of the sunlight teleport. */
static void testSunlightChance(void)
{
	// Below the minimum light level there is no chance at all:
	TEST_EQUAL(EndermanSunlightTeleportChance(0), 0);
	TEST_EQUAL(EndermanSunlightTeleportChance(12), 0);

	// The threshold value itself already has a chance: 23 / 1575 at light level 13:
	TEST_TRUE(IsCloseTo(EndermanSunlightTeleportChance(13), 23.0 / 1575.0));

	// Full daylight, eye block lit by the unobstructed sun: 45 / 1125 = 1 / 25 per tick:
	TEST_TRUE(IsCloseTo(EndermanSunlightTeleportChance(15), 1.0 / 25.0));

	// The chance grows with the light level:
	TEST_TRUE(EndermanSunlightTeleportChance(14) > EndermanSunlightTeleportChance(13));
	TEST_TRUE(EndermanSunlightTeleportChance(15) > EndermanSunlightTeleportChance(14));
}





IMPLEMENT_TEST_MAIN("EndermanTeleportRulesTest",
	testHorizontal();
	testVertical();
	testCoincident();
	testSunlightChance();
)
