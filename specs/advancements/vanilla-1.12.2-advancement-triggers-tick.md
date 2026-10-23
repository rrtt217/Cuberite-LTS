# 1.12.2 进度触发器：tick spec

> **状态**：实现中（feature/advancements-triggers-tick，叠在 feature/advancements-triggers-changed-dimension 之上）。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 `minecraft:tick` 小节。

## 1. 原版行为（文档）

每个游戏刻为玩家触发一次。

## 2. 实现

- trigger 白名单加入 `minecraft:tick`；加载期只接受空条件；带条件的 criterion 会 warning 并跳过（当前 wiki 的 tick 没有条件，保留这个门以防未来数据）。
- 调用点：`cPlayer::Tick()` 在 `Super::Tick()` 之后（已经排除未进入游戏/冻结/维度切换中的情况）→ `TriggerTick(*this)`。
- 条件匹配：无条件 criterion 每 tick 检查一次，未达成则授予；达成后 `HasAdvancementCriterion()` 提前跳过，索引为空时只是一次 `equal_range`。
- demo：`Server/advancements/cuberite/m5/tick.json`（无条件）。

## 3. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 启动日志 Loaded 21 advancement(s)；无 unsupported 条件 warning |
| 触发授予 | bot 在线后一个 tick 内 `HasAdvancement("cuberite:m5/tick")` = true（实测 true） |
| 客户端 | 公告/树更新〔待验〕（toast 在 M1 已记录为客户端侧未观测到） |

## 4. 已知偏差 / 后续

- tick 的 conditions 未实现（wiki 当前无 conditions；若有数据会加载期跳过）。
- 每个玩家每 tick 做一次 trigger 索引查找；当前数据只有 1 条 tick criterion，后续可按 trigger 存在性快速跳过（索引为空时 `equal_range` 已很便宜）。
- 只在 `cPlayer::Tick` 的「playing 且未 frozen/未切换世界」路径触发；与原版 tick 的边界差异〔待核〕。
