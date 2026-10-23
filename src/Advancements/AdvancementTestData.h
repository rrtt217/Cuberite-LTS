// AdvancementTestData.h

// Declares the temporary two-node advancement tree used to smoke-test the 1.12.2 Advancements packet.
// This file is removed once the JSON-driven cAdvancementManager lands.

#pragma once

#include "Advancement.h"





/** Returns the temporary tree sent to 1.12+ clients on join. */
cAdvancementUpdate BuildAdvancementTestTree();
