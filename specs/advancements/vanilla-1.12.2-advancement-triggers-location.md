# 1.12.2 进度触发器：location spec

> **状态**：实现中（feature/advancements-triggers-location，叠在 feature/advancements-triggers-construct-beacon 之上）。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 `minecraft:location` 小节。

## 1. 原版行为（文档）

每 20 tick 检查一次玩家位置；条件是一个 location predicate：

- `dimension`：维度；
- `position`：`x` / `y` / `z`（数值或 `{min,max}`）；
- 以及 biome / feature / light / block 等（本实现暂不支持）。

## 2. 实现

- trigger 白名单加入 `minecraft:location`；加载期只接受 `dimension` 与 `position`（position 内只接受 x/y/z），其他键 warning 并跳过（fail-closed）。
- 调用点：`cPlayer::Tick` 在 `TriggerTick` 之后调用 `TriggerLocation(*this)`；用玩家当前维度 + 脚下方块坐标（floor）匹配。
- 条件匹配：维度名复用 `DimensionName`（overworld/the_nether/the_end），`position` 各轴用 `MatchRange`（整数或 min/max）。
- demo：`Server/advancements/cuberite/m5/location.json`（dimension=overworld）。

## 3. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 启动日志 Loaded 31 advancement(s)；无 unsupported 条件 warning |
| 触发授予 | bot 在 overworld 上线后一个 tick → `HasAdvancement("cuberite:m5/location")` = true（实测 true） |
| 客户端 | 公告/树更新〔待验〕（toast 在 M1 已记录为客户端侧未观测到） |

## 4. 已知偏差 / 后续

- wiki 是每 20 tick 检查一次；本实现每 tick 检查（静态条件的进度仍只完成一次，差异在性能/时机）〔待核：是否需要在 tick 计数器上取模 20〕。
- biome / feature / light / block 等 location predicate 字段未实现；含这些字段的 criterion 加载期被跳过。
- `position` 用脚下方块坐标（floor），与原版的精确坐标边界可能有 ±1 差异〔待核〕。
