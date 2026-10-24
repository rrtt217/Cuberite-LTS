# 1.12.2 进度条件谓词审计

> **来源**：Minecraft Wiki 2017-09-27 修订版（revid 1151757）的「Advancement」页面，
> 其 Criteria 章节逐个列出 1.12.2 每个触发器的可用条件（用户给定的 zh wiki 两页当前版本已是
> 1.13+/数据组件时代的格式，不能用于 1.12.2 对照）。
> **状态**：审计完成；已实现项与缺口如下。

## 1. 触发器与条件键（1.12.2，共 28 个）

| 触发器 | 1.12.2 条件键 | 本实现 |
|---|---|---|
| bred_animals | child, parent, partner | ✅ |
| brewed_potion | potion | ✅ |
| changed_dimension | from, to | ✅ |
| construct_beacon | level | ✅ |
| consume_item | item | ✅ |
| cured_zombie_villager | villager, zombie | ✅ |
| effects_changed | effects（amplifier, duration） | ✅（glowing/levitation 效果未实现） |
| enchanted_item | item, levels | ✅ |
| enter_block | block, state | ⚠️ state 未匹配 |
| entity_hurt_player | damage | ✅ |
| entity_killed_player | entity, killing_blow | ⚠️ killing_blow 未匹配 |
| impossible | — | ✅ |
| inventory_changed | items, slots | ✅ |
| item_durability_changed | delta, durability, item | ✅ |
| levitation | distance | ⚠️ 触发器不存在（数据未使用） |
| location | （位置谓词） | ✅ |
| nether_travel | distance | ✅ |
| placed_block | block, item, location, state | ⚠️ item/location/state 未匹配 |
| player_hurt_entity | damage, entity | ⚠️ damage 未匹配 |
| player_killed_entity | entity, killing_blow | ⚠️ killing_blow 未匹配（数据未使用） |
| recipe_unlocked | recipe | ✅ |
| slept_in_bed | — | ✅ |
| summoned_entity | entity | ✅ |
| tame_animal | entity | ✅ |
| tick | — | ✅ |
| used_ender_eye | distance | ✅ |
| used_totem | item | ✅ |
| villager_trade | item, villager | ⚠️ 触发器未实现 |

## 2. 实体谓词的子键（1.12.2）

本实现支持：`type`、`distance`、`location.dimension`、`player`。

1.12.2 还定义但本实现未支持：`flags`（on_fire / sneaking / sprinting / invisible / baby）、
`equipment`（head / chest / legs / feet / mainhand / offhand）、`effects`、`nbt`、`team`、
`scores`、`advancements`、`vehicle`、`passenger`、`targeted_entity`。

现存 488 条数据只用到 `type`、`distance`、`player`、`location`（以及 1 处 `biome`/`feature`），
因此这些缺口不影响当前数据；属于「规格完整度」而非「数据阻塞」。

## 3. 数据实际使用 vs 支持

- 数据使用的条件键全部被支持（唯一例外是 `effects_changed` 的 2 条，依赖 glowing/levitation 效果）；
- 上表 ⚠️ 项均**未被当前数据使用**，但属于 1.12.2 规格的一部分。

## 4. 结论

- 无「数据写了但被静默丢弃」的条件；
- 规格层面仍有 7 处未匹配（killing_blow ×2、damage ×1、state ×2、item/location ×1、villager_trade 触发器）。
