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
## 5. 「未使用」的成因（维护者提问的核查结论）

逐条比对 2017 wiki 的 55 条进度描述与我们的 488 条数据后，结论分三类：

### 5.1 真正不需要（正常情况）

绝大多数进度的条件本来就只是 `entity.type`（如 `kill_a_mob`、`kill_all_mobs` 的每个 mob 各一条 criterion），
与 wiki 描述一致。这类「未使用」是正确的。

### 5.2 编写时的谓词缺失（已修）

- `adventure/sniper_duel`：wiki 描述明确要求「50 格以上用箭」，但数据只写了 `entity.type`，
  没有 `entity.distance`。这是**当时 distance 谓词未实现**导致的遗漏，已补 `{"min": 50}`。

### 5.3 当时用 impossible 占位（部分待修）

4 条 criterion 用了 `minecraft:impossible` 占位：

| 进度 | 原版触发 | 现状 |
|---|---|---|
| `husbandry/tame_an_animal` | `tame_animal` + `entity` | **已修**为 `tame_animal` + `entity.type = minecraft:wolf` |
| `adventure/shoot_arrow` | `player_hurt_entity` + `damage.type.direct_entity` | 占位未修（`damage.type` 子谓词尚未匹配） |
| `end/levitate` | `levitation`（距离 50）或 `effects_changed` | 占位未修（`levitation` 触发器未实现） |
| `recipes/root` | `impossible` | 正确（根节点本就用 `impossible`） |

因此对维护者问题的回答是：**两者都有**——大部分确实是原版就不需要；少数（sniper_duel、tame_an_animal、
shoot_arrow、levitate）是当时谓词/触发器没实现而遗漏或用占位符绕过的。
## 6. 仍缺项及其阻塞原因（逐条核对）

| 缺口 | 阻塞原因 | 建议 |
|---|---|---|
| `state`（`enter_block` / `placed_block`） | 1.12.2 用方块状态**名称**（如 `{"facing": "west"}`），本仓库只有 metadata；`src/Registries/BlockStates.*` 是 1.13+ 的状态 ID 生成物，不能直接反查名称 | 需要一张 metadata → 状态名的按方块映射表，或改走 1.13 状态 ID 路径 |
| `villager_trade` 触发器 | `src/Mobs/Villager.cpp` 没有交易逻辑，无调用点 | 依赖村民交易子系统 |
| `shoot_arrow` 的 `damage.type` | `TakeDamageInfo` 不携带 `direct_entity`（投射物实体），只有 `Attacker` | 需要给 `TakeDamageInfo` 增加投射物实体字段 |
| `end/levitate` 的 `levitation` | levitation 状态效果未实现 | 依赖状态效果子系统 |
| 实体谓词 `nbt` | 需要实体 NBT 与 SNBT 匹配器 | 独立工作量 |
| 实体谓词 `vehicle` / `passenger` | 本仓库未实现骑乘（`cEntity` 无 vehicle/passenger） | 依赖骑乘子系统 |
| 76 条 recipe 奖励 | `crafting.txt` 缺这些配方（16 色床、`slime` 等） | 配方系统缺口 |

以上均**未被现存 488 条数据使用**（`sniper_duel` 的 `distance`、`tame_an_animal` 的占位已在前面修复），因此不阻塞数据加载与主要触发路径。
