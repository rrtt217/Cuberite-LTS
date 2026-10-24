# 1.12.2 进度触发器：item_durability_changed spec

> **状态**：实现中（feature/advancements-triggers-durability，叠在 feature/advancements-command 之上）。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 `minecraft:item_durability_changed` 小节。

## 1. 原版行为（文档）

玩家背包里的物品耐久变化后触发。条件：

- `delta`：耐久差值（整数或 `{min,max}`）；
- `durability`：剩余耐久（整数或 `{min,max}`）；
- `item`：物品**损坏前**的 item predicate（item/data/count/durability/…）。

## 2. 实现

- trigger 索引沿用 M3 的 `m_Triggers`；已实现 trigger 白名单加入 `item_durability_changed`。
- 加载期条件支持性检查：只接受 `item` / `delta` / `durability`；其他键 warning 并跳过该 criterion（fail-closed）。
- 匹配：`item` 用 M3 的 `MatchItemPredicate`（item/data/count/durability；nbt/enchantments 仍忽略），`delta` / `durability` 用 `MatchRange`。
- 调用点：`cInventory::DamageItem()`（[Inventory.cpp:516](../../src/Inventory.cpp)）：
  - 先复制损坏前的 `ItemBefore`；
  - `Grid->DamageItem()` 之后计算剩余耐久（未损坏 = `GetMaxDamage() - m_ItemDamage`，损坏 = 0）；
  - 槽位更新/移除后，若 owner 正在 ticking 且 `ItemBefore` 非空，调用 `TriggerItemDurabilityChanged(owner, ItemBefore, a_Amount, Remaining)`；
  - 命中后走 M1 的 `GrantAdvancementCriterion`。
- demo：已移除（原 cuberite 测试数据由 1.12.2 原版数据取代，见 vanilla-1.12.2-advancement-delivery.md）。

## 3. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 启动日志 Loaded 13 advancement(s)；无 unsupported 条件 warning |
| 触发授予 | execute_lua：`inv:SetSlot(36, cItem(276, 1, 100))` 后 `inv:DamageItem(36, 1)` → `broke=false`、`HasAdvancement("cuberite:m5/durability")` = true |
| 客户端 | 公告/树更新〔待验〕（toast 在 M1 已记录为客户端侧未观测到） |

## 4. 已知偏差 / 后续

- `delta` 用的是 `DamageItem` 的请求值 `a_Amount`，不是考虑 Unbreaking 后的实际耐久变化；含 Unbreaking 的物品可能偏差〔待核：在 vanilla 用 Unbreaking 物品对照〕。
- 护甲受击的耐久损失若不走 `cInventory::DamageItem`，不会触发该触发器；当前只覆盖显式 `DamageItem` 路径（Lua/API、物品使用等）。
- `item` 谓词的 nbt/enchantments 未实现（与 M3 相同）。
- 物品损坏（durability 归零）时剩余耐久按 0 传给条件；`item` 仍是损坏前的副本。
