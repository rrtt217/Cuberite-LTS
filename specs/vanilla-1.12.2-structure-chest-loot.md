# Vanilla Minecraft Java 1.12.2 - Structure chest loot

> Scope: filling generated structure chests with their vanilla loot. The immediate target is the
> End City chests that the End City generator currently places as empty chests.
>
> Provenance: the chest tables below come from the Minecraft Wiki's 1.12-era loot chest module
> (Minecraft Wiki, Module:LootChest, 1.12-era revision). Anything still not confirmed is marked
> **待确认**.

## 1. Current state in Cuberite-LTS

- Structure chests are filled by hardcoded tables, one per generator:
  - `src/Generating/DungeonRoomsFinisher.cpp` (dungeon chests),
  - `src/Generating/MineShafts.cpp` (mineshaft chests).
- The shared mechanism is
  `cItemGrid::GenerateRandomLootWithBooks(const cLootProbab *, size_t, int a_NumSlots, int a_Seed)`
  with `cLootProbab { cItem m_Item; int m_MinAmount, m_MaxAmount, m_Weight; }`.
  It picks `a_NumSlots` weighted items and drops each into a random slot, and it always seeds an
  enchanted book first (the "WithBooks" behaviour).
- The End City generator (`src/Generating/EndCityGen.cpp`) writes the chest block from the blueprint
  character `C` / `c` / `e`, but never fills the chest block entity, so End City chests generate
  empty.

## 2. Vanilla behaviour

### 2.1 The End City treasure table

End City chests use `minecraft:chests/end_city_treasure` (`end_city_treasure.json`). It has a
single pool of **2 to 6 rolls**; each roll is an **independent weighted draw with replacement** from
the entry list and produces one entry.

The generated items are then placed into the chest: the loot table itself does not decide the slots.
The 1.12.2-era Loot table article states it plainly - "the loot table does not decide the arrangement
inside the chest; this randomness is entirely based on the seed" - and that each roll picks one entry
by weight. So for 1.12.2 the **rolls decide which items appear, and the slot arrangement is random and
seed-driven** (same seed and table produce the same pattern).

1.12-era table, given as `item: min-max amount (weight)`:

| Item | Amount | Weight |
|---|---|---|
| diamond | 2-7 | 5 |
| iron ingot | 4-8 | 10 |
| gold ingot | 2-7 | 15 |
| emerald | 2-6 | 2 |
| beetroot seeds | 1-10 | 5 |
| saddle | 1 | 3 |
| iron horse armor | 1 | 1 |
| golden horse armor | 1 | 1 |
| diamond horse armor | 1 | 1 |
| enchanted diamond sword | 1 | 3 |
| enchanted diamond boots | 1 | 3 |
| enchanted diamond chestplate | 1 | 3 |
| enchanted diamond leggings | 1 | 3 |
| enchanted diamond helmet | 1 | 3 |
| enchanted diamond pickaxe | 1 | 3 |
| enchanted diamond shovel | 1 | 3 |
| enchanted iron sword | 1 | 3 |
| enchanted iron boots | 1 | 3 |
| enchanted iron chestplate | 1 | 3 |
| enchanted iron leggings | 1 | 3 |
| enchanted iron helmet | 1 | 3 |
| enchanted iron pickaxe | 1 | 3 |
| enchanted iron shovel | 1 | 3 |

Pool total weight = 85.

Note that the 1.12 table has **no** copper horse armor, **no** spear and **no** armor-trim pool;
those only appear in the wiki's current (post-1.12) module.

### 2.2 Enchantments

From the 1.12-era module notes:

- The enchanted entries are enchanted with the **same probability as one 20-39 level enchantment at
  an enchanting table** (`end-city-enchantment` note; the note explicitly says 20-39 even though
  the table normally stops at 30). Implementation: pick a level in [20, 39] and run the server's
  existing enchanting-table algorithm, `cItem::EnchantByXPLevels`, which already models the
  enchantability roll and the extra-enchantment chances.
- **Within one chest, the same kind of item always carries the same enchantments**
  (`end-city-enchantment` note).
- From 1.11 (16w39a) onwards **cursed enchantments** can appear on items in End City chests
  (Minecraft Wiki, End City - History). This applies to 1.12.2.

### 2.3 Where the chests are

The chest positions are already present in the wiki blueprints (the generator reads `C`, `c`, `e`):

| Piece | Chests |
|---|---|
| `fat_tower_top` | 2 chests ("generated with 2 chests in the corner") |
| `third_floor_2` | 1 chest and 1 ender chest ("the second loot room, with a regular chest and an ender chest") |
| `ship` | 2 chests (plus 1 brewing stand with two Potions of Healing II, 1 dragon head and 1 item frame with an elytra) |
| `base_floor`, other rooms, bridges | none |

Note that the generator currently maps the wiki `Loot Room` blueprint to both `FatTowerTop` and
`LootRoom1`, and the `third_floor_2` chest/ender-chest pair lives in the wiki `Loot Room`
blueprint; the exact piece-to-blueprint mapping should be re-checked while implementing.

## 3. Implementation plan (not code)

1. Add an End City loot table next to the generator (a `cLootProbab` array), with the 1.12 item set
   and weights from section 2.1 (total weight 85).
2. When the generator writes a chest block from the blueprint, also fill its block entity:
   - 2-6 rolls from the table (the vanilla roll count), placed into the chest slots,
   - apply `enchant_with_levels` at 20-39 to the enchanted entries, allow cursed enchantments, and
     keep same-kind items inside one chest consistent.
3. Keep the existing `cLootProbab` mechanism if its roll/slot semantics can express "N rolls, random
   slot"; otherwise extend it carefully (the current "WithBooks" version always seeds a book, which
   the End City table must not do).
4. Ender chests stay empty (ender chests never hold loot).
5. Out of scope for the first pass (tracked separately): ship brewing stand contents, item frame
   with elytra, dragon head.

## 4. Verification

- Unit test: generate an End City that contains a chest, read the chest's block entity, and assert
  that the contents are drawn from the table (each stack's item, count within the range, weight pool
  respected) and that ender chests are empty.
- Determinism: the same seed must produce the same contents.
- Gate: `cd src && lua CheckBasicStyle.lua`, build, `ctest` (see AGENTS.md section 4).

## 5. Sources

- Minecraft Wiki, [End City](https://minecraft.wiki/w/End_City) - Loot, Structure details, History.
- Minecraft Wiki, `Module:LootChest` (1.12-era revision) - the `end-city` entry and the
  `end-city-enchantment` note.
- Minecraft Wiki, [Loot table](https://zh.minecraft.wiki/w/%E6%88%98%E5%88%A9%E5%93%81%E8%A1%A8) -
  the 1.12.2-era revision (oldid=230915): rolls, weights, `enchant_with_levels`, and the seed-driven
  slot arrangement.
- Minecraft Wiki, [End City/Structure](https://minecraft.wiki/w/End_City/Structure) and the layered
  blueprint subpages - chest markers.
- Existing repository behaviour: `DungeonRoomsFinisher.cpp`, `MineShafts.cpp`, `ItemGrid.cpp`.

## 6. Uncertainties

1. Resolved for 1.12.2: the loot table does not decide the slots; the arrangement is random and
   seed-driven, and each roll is an independent weighted draw (Minecraft Wiki, Loot table, 1.12.2-era
   revision, oldid=230915). The only remaining detail is whether a later roll may overwrite an
   already-filled slot; with 2-6 rolls against 27 slots it is unlikely to matter, but it is still
   **待确认**.
2. The module note also says that within one chest the same kind of item carries the same
   enchantments; the reason is not documented and it is not implemented (enchantments are rolled per
   item). **待确认**.
2. The piece-to-blueprint mapping of `third_floor_2` vs `Loot Room` in this generator - needs a
   re-check during implementation.
