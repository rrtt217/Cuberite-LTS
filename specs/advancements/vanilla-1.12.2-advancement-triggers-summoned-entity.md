# 1.12.2 进度触发器：summoned_entity spec

> **状态**：实现中（feature/advancements-triggers-summoned-entity，叠在 feature/advancements-triggers-slept-in-bed 之上；Core 对应分支 feature/core-summoned-entity-trigger）。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 `minecraft:summoned_entity` 小节。

## 1. 原版行为（文档）

玩家召唤一个实体后触发。条件：

- `entity`：被召唤实体的 entity predicate（type / player 等）。

## 2. 实现

- 本仓库的召唤入口是 Core 的 `/summon` 命令：
  - `SpawnEntity()` 现在返回 `(Success, EntityId)`：mob / minecart / projectile 返回 API 的实体 ID；
  - `HandleSummonCommand` 在成功后用 `World:DoWithEntityByID(EntityId, ...)` 找到实体，并调用 `Player:TriggerSummonedEntity(entity)`（仅当有 summoning player）。
- C++：`cPlayer::TriggerSummonedEntity(cEntity &)` → `cAdvancementManager::TriggerSummonedEntity()`；加载期只接受 `entity`，其他键 warning 并跳过；匹配复用 `MatchEntityPredicate`（type/player）。
- demo：`Server/advancements/cuberite/m5/summoned_entity.json`（entity.type=minecraft:zombie）。

## 3. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 启动日志 Loaded 25 advancement(s)；无 unsupported 条件 warning |
| 触发授予 | `op TestBot`（rank Admin）后 bot 发送 `/summon zombie` → `HasAdvancement("cuberite:m5/summoned_entity")` = true（实测 true） |
| 客户端 | 公告/树更新〔待验〕（toast 在 M1 已记录为客户端侧未观测到） |

## 4. 已知偏差 / 后续

- 只覆盖 Core `/summon` 命令；刷怪蛋、结构生成等召唤路径未触发。
- `SpawnEntity` 对 boat / falling_block / lightning / tnt / xp_orb / ender_crystal 仍返回 nil ID（这些分支保持原行为，只是不通知触发器）；mob / minecart / projectile 已返回 ID。
- 控制台 `/summon` 没有 summoning player，不触发（原版命令方块/控制台同样没有玩家归属〔待核〕）。
- entity 谓词只支持 type/player（与战斗簇一致）。
