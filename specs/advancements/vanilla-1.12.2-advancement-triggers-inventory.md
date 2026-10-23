# 1.12.2 进度触发器：inventory_changed（M3 第一簇）spec

> **状态**：实现中（feature/advancements-triggers-inventory，叠在 feature/advancements-core 之上）。
> **范围**：只做 `minecraft:inventory_changed` 这一簇 + 物品条件子集；其他 27 个 trigger 留给后续簇。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 `minecraft:inventory_changed` 小节与 item predicate 示例；[同步实测记录](vanilla-1.12.2-advancements-observation.md) 里的可见性/更新包形态。

## 1. 原版行为（文档）

- 触发时机：玩家背包发生**任何**变化之后（拾取、合成、放入/取出、丢弃等）。
- 条件（都可省略）：
  - `items`：物品谓词列表。**列表里的每一项都必须在背包里存在**（不要求背包里只有这些物品）。
  - `slots`：`empty` / `full` / `occupied` 三个槽位计数，每个可以是整数或 `{min,max}` 范围。
- 物品谓词（item predicate）支持 `item`（id 字符串）、`data`、`count`（整数或 `{min,max}`）、`durability`（剩余耐久，整数或范围）、`nbt`、`enchantments` 等；示例：

    "conditions": {
      "items": [
        { "item": "minecraft:stone", "data": 2, "count": { "min": 5, "max": 10 } },
        { "item": "minecraft:wool", "data": 5 },
        { "item": "minecraft:wooden_sword", "durability": { "min": 50 } }
      ],
      "slots": { "occupied": 3 }
    }

## 2. 实现

- 加载时把每个 criterion 的 `trigger` 与 `conditions` 存进 `cAdvancementManager` 的 trigger 索引（`std::multimap<trigger, {advancement, criterion, conditions}>`）；校验阶段会丢弃 advancement 的条目也会从索引里过滤掉。
- `cAdvancementManager::TriggerInventoryChanged(cPlayer &)`：
  - 取 `minecraft:inventory_changed` 的所有条目；
  - 已达成该 criterion 的跳过（幂等）；
  - 用当前背包整体求值条件；
  - 命中则调用已有的 `cPlayer::GrantAdvancementCriterion(advancement, criterion)`（M1 的授予/公告/奖励/可见性更新都复用）。
- 调用点：`cInventory::OnSlotChanged` 的末尾（[Inventory.cpp:835](../../src/Inventory.cpp)），在 `SendSlot` 之后。
  - `OnSlotChanged` 已有 `m_Owner.IsTicking()` 守卫，读档/初始化阶段不会触发（避免在客户端登录前发进度包）。
- 条件子集（M3）：`items` 的 `item`/`data`/`count`/`durability`，`slots` 的 `empty`/`full`/`occupied`。 `nbt`/`enchantments` 暂不匹配（忽略，等价于不限制）；未知键不参与匹配。
- 匹配语义：`items` 里的每个谓词都要在背包中找到**至少一个**匹配的堆叠（count 对单个堆叠的 `m_ItemCount` 求值；durability = `GetMaxDamage() - m_ItemDamage`）；`slots` 对 41 个背包槽（4 护甲 + 27 主背包 + 9 快捷栏 + 1 盾牌）计数。

## 3. demo 数据

Server/advancements/cuberite/m3/：
- `stick.json`：parent cuberite:m1/root，conditions items=[minecraft:stick]；
- `diamond.json`：items=[minecraft:diamond]；
- `five_sticks.json`：parent cuberite:m3/stick，items=[minecraft:stick, count.min=5]，frame challenge。

## 4. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 启动日志 `Loaded 7 advancement(s) from "advancements"`（M1 4 条 + M3 3 条） |
| 触发授予（1 个 stick） | execute_lua 给 TestBot `AddItem(cItem(280, 1))` → `HasAdvancement("cuberite:m3/stick")` = true，`cuberite:m3/diamond` = false |
| 触发授予（5 个 stick） | 再加 5 个 stick（总数 6）→ `HasAdvancement("cuberite:m3/five_sticks")` = true |
| 客户端 | 聊天公告/树节点更新（toast 在 M1 已记录为客户端侧未观测到）〔待验〕 |
| 可见性 | 未授予时 M3 节点属于 root 的 2 步邻域，随 root 授予一起可见；不额外制造页签 |

## 5. 已知偏差 / 后续

- 只实现 `inventory_changed`；其余 27 个 trigger 按战斗/世界/状态分簇。
- `nbt`/`enchantments`/`predicate` 条件未实现；含这些键的 criterion 会匹配得过宽（当前数据未使用）。
- 每次槽位变化扫描全部 `inventory_changed` 条目 + 41 槽；M3 数据量小，后续可按物品 id 建索引优化。
- 没有触发冷却/去抖；同一槽位连续变化会重复求值，但 criterion 幂等。
