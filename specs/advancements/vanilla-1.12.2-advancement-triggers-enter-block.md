# 1.12.2 进度触发器：enter_block spec

> **状态**：实现中（feature/advancements-triggers-enter-block，叠在 feature/advancements-triggers-tick 之上）。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 `minecraft:enter_block` 小节。

## 1. 原版行为（文档）

玩家进入一个方块后触发。条件：

- `block`：方块 ID 字符串（可带 state）。

## 2. 实现

- trigger 白名单加入 `minecraft:enter_block`；加载期只接受 `block`，其他键 warning 并跳过。
- 调用点：`cPlayer::Tick` 在 `Super::Tick` 之后比较玩家脚下方块与上一 tick 的记录：
  - 记录成员 `m_LastEnteredBlock`（初值空气）；
  - 方块变化且新方块非空气 → `TriggerEnterBlock(*this, CurrentBlock)`。
- 条件匹配：复用 `MatchBlockName`（`ItemToString(cItem(BlockType))`，两边去掉 `minecraft:` 前缀）。
- demo：已移除（原 cuberite 测试数据由 1.12.2 原版数据取代，见 vanilla-1.12.2-advancement-delivery.md）。

## 3. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 启动日志 Loaded 22 advancement(s)；无 unsupported 条件 warning |
| 触发授予 | bot 脚下（100,64,-20）SetBlock 水(9) → 下一 tick `HasAdvancement("cuberite:m5/enter_block")` = true（实测 true） |
| 客户端 | 公告/树更新〔待验〕（toast 在 M1 已记录为客户端侧未观测到） |

## 4. 已知偏差 / 后续

- 只比较玩家脚下方块；头部/其他位置进入方块未覆盖。
- 方块名用 `ItemToString(cItem(BlockType))`（meta 0）；带 state 的方块变体未区分。
- 只在 `cPlayer::Tick` 的 playing/非 frozen 路径检测；与原版 enter_block 的边界差异〔待核〕。
- 水/熔岩的流动可能让方块 ID 在 8/9 之间切换，导致重复触发；criterion 达成后会被跳过。
