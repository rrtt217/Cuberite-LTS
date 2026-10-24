# 1.12.2 进度触发器：brewed_potion spec

> **状态**：实现中（feature/advancements-triggers-brewed-potion，叠在 feature/advancements-triggers-cured-zombie-villager 之上）。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 `minecraft:brewed_potion` 小节。

## 1. 原版行为（文档）

玩家从酿造台里**取出任意物品**后触发。条件：

- `potion`：酿造出的药水 ID 字符串（如 `minecraft:strong_swiftness`）。

## 2. 实现

- trigger 白名单加入 `minecraft:brewed_potion`；加载期只支持**无条件** criterion；带 `potion` 条件的 criterion 会 warning 并跳过（药水 ID 映射尚未实现，fail-closed）。
- 调用点：`cBrewingstandWindow::DistributeStack()` 的「酿造台槽位 → 玩家背包」分支（shift-click 取出）：
  - 在 `DistributeStackToAreas()` 前后比较 `a_ItemStack.m_ItemCount`；
  - 只有 `a_ShouldApply` 且数量确实减少时才触发；
  - 用取出前的 `cItem` 副本调用 `TriggerBrewedPotion(a_Player, Taken)`。
- demo：`Server/advancements/cuberite/m5/brewed_potion.json`（无条件）。

## 3. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 启动日志 Loaded 29 advancement(s)；无 unsupported 条件 warning |
| 触发授予 | 在 (99,64,-24) 放酿造台，deposit 水瓶到 slot 0，再 `mcc_inventory_window_action({slot:0, clickMode:1})` shift-click 取出 → `HasAdvancement("cuberite:m5/brewed_potion")` = true（普通 click 的 `mcc_container_withdraw_item` 不触发，已记入偏差） |
| 客户端 | 公告/树更新〔待验〕（toast 在 M1 已记录为客户端侧未观测到） |

## 4. 已知偏差 / 后续

- 只覆盖 shift-click 取出；普通点击（先拿在光标上再放入背包）不经过 `DistributeStack`，不会触发〔待核：vanilla 的判定边界〕。
- `potion` 条件未实现；需要把 1.12.2 的 potion damage → ID 映射接入。
- 取出任意物品都会触发（与 wiki 的「any item」一致）；无条件 demo 不区分药水/材料/燃料。
