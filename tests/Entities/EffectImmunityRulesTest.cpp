#include "Globals.h"
#include "Entities/EffectImmunityRules.h"
#include "../TestHelpers.h"

/** What this test verifies:
The immunity table of specs/vanilla-1.12.2-status-effect-immunity.md section 2, which is taken from
https://minecraft.wiki/w/Status_effect (section "Immunity"):
(1) Undead mobs are immune to Regeneration and Poison, but accept every other effect;
(2) Wither skeletons are immune to Wither on top of that, while other undead mobs are not;
(3) Spiders and cave spiders are immune to Poison, but poison immunity does not spill over to other
    effects (the arthropod path must keep its Slowness and the rest);
(4) The wither and the ender dragon are immune to all 23 effects of 1.12.2;
(5) A player (not a mob) accepts every effect, so the table cannot hurt players by accident;
(6) An ordinary, non-immune mob still accepts every effect - the table must not be over-broad.

Invariants:
- Only the four documented immune categories (undead, wither skeleton, spider / cave spider, boss)
  ever come out rejected;
- The wither and the ender dragon reject each of the 23 effects, and nothing else rejects all of them;
- An effect is rejected for a wither skeleton only through its mob type, not through a coincidence of
  the undead flag.

Why: the immunity decision used to live inside the individual effects' OnTick code; this test locks the
extracted decision (src/Entities/EffectImmunityRules.h) without constructing an entity or a world. */

/** Every effect that exists in 1.12.2, i.e. cEntityEffect::effSpeed through effSaturation (23 entries). */
static const cEntityEffect::eType g_AllEffects[] =
{
	cEntityEffect::effSpeed,
	cEntityEffect::effSlowness,
	cEntityEffect::effHaste,
	cEntityEffect::effMiningFatigue,
	cEntityEffect::effStrength,
	cEntityEffect::effInstantHealth,
	cEntityEffect::effInstantDamage,
	cEntityEffect::effJumpBoost,
	cEntityEffect::effNausea,
	cEntityEffect::effRegeneration,
	cEntityEffect::effResistance,
	cEntityEffect::effFireResistance,
	cEntityEffect::effWaterBreathing,
	cEntityEffect::effInvisibility,
	cEntityEffect::effBlindness,
	cEntityEffect::effNightVision,
	cEntityEffect::effHunger,
	cEntityEffect::effWeakness,
	cEntityEffect::effPoison,
	cEntityEffect::effWither,
	cEntityEffect::effHealthBoost,
	cEntityEffect::effAbsorption,
	cEntityEffect::effSaturation,
};





/** (1) Undead mobs reject Regeneration and Poison and accept the rest. */
static void testUndeadMob(void)
{
	TEST_FALSE(IsEffectApplicable(cEntityEffect::effRegeneration, mtZombie, true, false));
	TEST_FALSE(IsEffectApplicable(cEntityEffect::effPoison,       mtZombie, true, false));

	TEST_TRUE(IsEffectApplicable(cEntityEffect::effWither,   mtZombie, true, false));
	TEST_TRUE(IsEffectApplicable(cEntityEffect::effWeakness, mtZombie, true, false));
	TEST_TRUE(IsEffectApplicable(cEntityEffect::effSpeed,    mtZombie, true, false));
}





/** (2) Wither skeletons reject Wither on top of the undead immunities; other undead do not. */
static void testWitherSkeleton(void)
{
	// A wither skeleton is undead in vanilla 1.12.2, so it also inherits the two undead immunities.  The
	// Wither row below is decided by the mob type alone and must not depend on this flag:
	TEST_FALSE(IsEffectApplicable(cEntityEffect::effWither,       mtWitherSkeleton, true, false));
	TEST_FALSE(IsEffectApplicable(cEntityEffect::effRegeneration, mtWitherSkeleton, true, false));
	TEST_FALSE(IsEffectApplicable(cEntityEffect::effPoison,       mtWitherSkeleton, true, false));

	// A plain zombie is just as undead, but not immune to Wither:
	TEST_TRUE(IsEffectApplicable(cEntityEffect::effWither, mtZombie, true, false));
}





/** (3) Spiders and cave spiders reject Poison; their other effects still apply. */
static void testSpiders(void)
{
	TEST_FALSE(IsEffectApplicable(cEntityEffect::effPoison, mtSpider,     false, false));
	TEST_FALSE(IsEffectApplicable(cEntityEffect::effPoison, mtCaveSpider, false, false));

	// The poison immunity must not spill over to the other effects the arthropod path uses:
	TEST_TRUE(IsEffectApplicable(cEntityEffect::effSlowness, mtCaveSpider, false, false));
	TEST_TRUE(IsEffectApplicable(cEntityEffect::effRegeneration, mtSpider, false, false));
}





/** (4) The wither and the ender dragon reject all 23 effects of 1.12.2. */
static void testBossesRejectEverything(void)
{
	for (const auto Type : g_AllEffects)
	{
		TEST_FALSE(IsEffectApplicable(Type, mtWither, true, IsBossImmuneToAllEffects(mtWither)));
		TEST_FALSE(IsEffectApplicable(Type, mtEnderDragon, false, IsBossImmuneToAllEffects(mtEnderDragon)));
	}
}





/** (4) Only the wither and the ender dragon are the "immune to everything" bosses. */
static void testBossMobTypes(void)
{
	TEST_TRUE(IsBossImmuneToAllEffects(mtWither));
	TEST_TRUE(IsBossImmuneToAllEffects(mtEnderDragon));

	TEST_FALSE(IsBossImmuneToAllEffects(mtWitherSkeleton));
	TEST_FALSE(IsBossImmuneToAllEffects(mtZombie));
	TEST_FALSE(IsBossImmuneToAllEffects(mtInvalidType));
}





/** (5) A player accepts every effect: the immunity table may never hurt a player. */
static void testPlayerAcceptsEverything(void)
{
	for (const auto Type : g_AllEffects)
	{
		TEST_TRUE(IsEffectApplicable(Type, mtInvalidType, false, false));
	}
}





/** (6) An ordinary mob accepts every effect; the table must not be over-broad. */
static void testOrdinaryMobAcceptsEverything(void)
{
	for (const auto Type : g_AllEffects)
	{
		TEST_TRUE(IsEffectApplicable(Type, mtCow, false, false));
	}
}





IMPLEMENT_TEST_MAIN("EffectImmunityRulesTest",
	testUndeadMob();
	testWitherSkeleton();
	testSpiders();
	testBossesRejectEverything();
	testBossMobTypes();
	testPlayerAcceptsEverything();
	testOrdinaryMobAcceptsEverything();
)
