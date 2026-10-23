# 1.12.2 进度触发器：consume_item spec

> **状态**：实现中（feature/advancements-triggers-consumed-item，叠在 feature/advancements-triggers-durability 之上）。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 `minecraft:consume_item` 小节。

## 1. 原版行为（文档）

玩家消耗一个物品后触发。条件：

- `item`：被消耗物品的 item predicate（item/data/count/durability/…）。

## 2. 实现

- trigger 白名单加入 `minecraft:consume_item`；加载期只接受 `item`，其他键 warning 并跳过（fail-closed）。
- 匹配：`MatchItemPredicate`（item/data/count/durability；nbt/enchantments 仍忽略）。
- 调用点：`cPlayer::FinishEating()`（[Player.cpp:530](../../src/Entities/Player.cpp)）：
  - 复制当前手持物品为 count=1 的 `Item`；
  - `EatItem()` 成功、`OnFoodEaten()` 之后再调用 `TriggerConsumedItem(*this, Item)`；
  - `EatItem()` 失败（不饿/不可吃）时不触发。
- demo：`Server/advancements/cuberite/m5/eat_apple.json`（item=minecraft:apple）。

## 3. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 启动日志 Loaded 14 advancement(s)；无 unsupported 条件 warning |
| 触发授予（bot 路径） | bot 满食(food=20) 先用 `AddFoodExhaustion(1000)` 降到 13，持苹果；`mcc_hold_use(action=start, durationMs=2500)` → food 13→17、苹果消耗、`HasAdvancement("cuberite:m5/eat_apple")` = true |
| 客户端 | 公告/树更新〔待验〕（toast 在 M1 已记录为客户端侧未观测到） |

## 4. 已知偏差 / 后续

- 只覆盖 `cPlayer::FinishEating`（食物）；药水等若走别的消耗路径不会触发〔待核：原版药水是否算 consume_item〕。
- `item` 谓词的 nbt/enchantments 未实现（与 M3/M5 一致）。
- 食物计数：`Item` 固定 count=1，与一次消耗一份的行为一致。
- 注意：vanilla husbandry root 的 **criterion 名**是 `consumed_item`，但它对应的 **trigger** 是 `minecraft:consume_item`；本实现早期误用了 `consumed_item` 作为 trigger，已在 fix 分支改正。
