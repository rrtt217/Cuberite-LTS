// EndCityLoot.h

// Declares the End City chest loot table and the hook that installs it

#pragma once

class cItemGrid;

/** A function that fills an End City chest's contents from the treasure table. */
typedef void (*EndCityChestFiller)(cItemGrid & a_Contents, int a_Seed);

/** Installs the filler that the End City generator uses to fill chests. It is called by this
module's static initialiser, so the generator stays free of the item system. */
void SetEndCityChestFiller(EndCityChestFiller a_Filler);
