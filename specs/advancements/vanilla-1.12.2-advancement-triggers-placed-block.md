# 1.12.2 进度触发器：placed_block spec

> **状态**：实现中（feature/advancements-triggers-placed-block，叠在 feature/advancements-triggers-consumed-item 之上）。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 `minecraft:placed_block` 小节。

## 1. 原版行为（文档）

玩家放置方块后触发。条件：

- `block`：方块 ID 字符串（如 `minecraft:stone`）；
- `item`：放置时使用的物品 predicate；
- `location`：方块位置 predicate；
- `state`：方块状态表。

## 2. 实现

- trigger 白名单加入 `minecraft:placed_block`。
- 加载期只接受 `block` / `item`；出现 `location` / `state` 或其他键时 warning 并跳过（fail-closed）。
- 匹配：
  - `block`：方块名用 `ItemToString(cItem(BlockType))`（如 `minecraft:stone`），两边都去掉 `minecraft:` 前缀后比较；
  - `item`：M3 的 `MatchItemPredicate`，对放置时手持物品求值。
- 调用点：`cPlayer::PlaceBlocks()` 在每个方块 `m_World->PlaceBlock()` 成功之后、插件 "placed" hook 之前调用 `TriggerPlacedBlock(*this, Block.m_BlockType, GetEquippedItem())`；一次多方块放置（如床）按方块逐个触发。
- demo：已移除（原 cuberite 测试数据由 1.12.2 原版数据取代，见 vanilla-1.12.2-advancement-delivery.md）。

## 3. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 启动日志 Loaded 15 advancement(s)；无 unsupported 条件 warning |
| 触发授予（bot 路径） | 用 Lua SetBlock 在 (-4,99,-2) 造支撑；bot 持石头（slot 37），`mcc_place_block({-4,100,-2, auto})` → 目标块变为石头(1)，`HasAdvancement("cuberite:m5/place_stone")` = true |
| 客户端 | 公告/树更新〔待验〕（toast 在 M1 已记录为客户端侧未观测到） |

## 4. 已知偏差 / 后续

- `location` 与 `state` 未实现；含这些键的 criterion 加载期被跳过。
- 方块名用 `ItemToString(cItem(BlockType))`（meta 0），对带 metadata 的方块变体（如 stone 的不同变体）不能区分；`state` 支持后再补。
- 一次放置多个方块时逐块触发；vanilla 是否逐块触发〔待核〕。
- `item` 谓词的 nbt/enchantments 未实现（与 M3/M5 一致）。
