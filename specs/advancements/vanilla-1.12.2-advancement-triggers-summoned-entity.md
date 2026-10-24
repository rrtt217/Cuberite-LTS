# 1.12.2 进度触发器：summoned_entity spec

> **状态**：实现中（world-level）。
> **来源**：[Minecraft Wiki — Advancement](https://minecraft.wiki/w/Advancement) 的 summoned_entity 小节（1.12.2 语义）。

## 1. 原版行为（文档）

- 铁傀儡 / 雪傀儡被玩家**搭建结构**召唤时，对**实体位置**切比雪夫距离 **5** 格内所有玩家触发；
- 凋灵被玩家**搭建结构**召唤时，对**实体位置**切比雪夫距离 **50** 格内所有玩家触发；
- 末影龙被**复活**时，对**末地中距 (0,0,0) 不超过 192 格**的所有玩家触发。

注意：`/summon` 命令**不是**触发源（不属于以上三条）。

条件：

- `entity`：被召唤实体的谓词（命名空间 ID 或内联谓词）；
- `player`：获得进度的玩家谓词。

## 2. 实现

- `cAdvancementManager::TriggerSummonedEntity(const cEntity & a_Entity)` 为世界级：取实体所在世界，按实体类型选择半径（傀儡 5、凋灵 50、默认 5），遍历世界玩家，按切比雪夫距离过滤，再匹配 `entity` 与 `player` 谓词后授予。
- 调用点（仅结构性召唤）：
  - 凋灵搭建：`cItemMobHead` 的 `TrySpawnWitherFromImage`；
  - 铁傀儡 / 雪傀儡搭建：`cItemPumpkin`。
- `/summon` 命令（Core `cmd_summon.lua`）已移除原先的 `Player:TriggerSummonedEntity` 调用——命令不是原版触发源。
- 末影龙复活：本仓库尚无龙复活生成器（`cEndDragonFight` 未实现），192/末地规则待该子系统落地后接入。

## 3. 验证

| 项 | 结果 |
|---|---|
| 构建 / 风格 | 通过 / 0 violations |
| 玩家搭建凋灵（50 格内） | 待实机验证（本轮仅验证 `/summon` 会触发，已确认该行为非原版并移除）〔待核〕 |
| 玩家搭建铁/雪傀儡（5 格内） | 待实机验证〔待核〕 |
| 末影龙复活 | 未实现（依赖龙复活生成器） |

## 4. 已知偏差 / 后续

- 末影龙复活尚未接入；`cEndDragonFight` 未实现。
- 半径内玩家判定使用切比雪夫距离（与 wiki 一致）。
- `player` 谓词复用实体谓词匹配器。
