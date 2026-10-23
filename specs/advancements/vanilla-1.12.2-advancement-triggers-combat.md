# 1.12.2 进度触发器：击杀簇（M4 第一簇）spec

> **状态**：实现中（feature/advancements-triggers-combat，叠在 feature/advancements-triggers-inventory 之上）。
> **范围**：`minecraft:player_killed_entity` 与 `minecraft:entity_killed_player` 两个击杀触发器 + entity 谓词子集；`player_hurt_entity` / `entity_hurt_player` 留给下一簇。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的对应 trigger 小节；[同步实测记录](vanilla-1.12.2-advancements-observation.md)。

## 1. 原版行为（文档）

- `minecraft:player_killed_entity`：玩家击杀一个实体后触发。条件：
  - `entity`：被击杀实体的 entity predicate；
  - `killing_blow`：伤害来源 predicate。
- `minecraft:entity_killed_player`：玩家被一个实体杀死后触发。条件：
  - `entity`：杀死玩家的实体 predicate；
  - `killing_blow`：伤害来源 predicate。

## 2. 实现

- trigger 索引沿用 M3 的 `m_Triggers`（按 trigger 名）。
- 加载期只索引**已实现**的 trigger（inventory_changed / player_killed_entity / entity_killed_player）；其他 trigger 的 criterion 暂时不索引。
- 已实现 trigger 的条件做支持性检查：击杀簇只接受 `entity`，entity 谓词只接受 `type`（字符串）与 `player`（bool）；出现 `killing_blow` 或其他键时加载期 warning 并跳过该 criterion（fail-closed，避免误触发）。
- entity 类型名：
  - 玩家 → `minecraft:player`；
  - 怪物 → `minecraft:` + NamespaceSerializer::From(GetMobType())（1.12.2 的实体 id 字符串，如 `minecraft:zombie`）；
  - 其他实体类型（弹射物、载具等）暂不生成名字，带 `type` 的条件不会命中（fail-closed）。
- 调用点：
  - `cPlayer::Killed(a_Victim, a_DamageType)` 末尾（击杀者）→ TriggerPlayerKilledEntity；
  - `cPlayer::KilledBy(a_TDI)` 死亡处理末尾（被击杀者）→ TriggerEntityKilledPlayer(a_TDI.Attacker)，attacker 为空时不触发。
- 命中后调用 M1 的 `GrantAdvancementCriterion`，授予/公告/奖励/可见性更新全部复用。

## 3. demo 数据

Server/advancements/cuberite/m4/：
- `kill_zombie.json`：player_killed_entity + entity.type=minecraft:zombie；
- `kill_skeleton.json`：entity.type=minecraft:skeleton；
- `killed_by_zombie.json`：entity_killed_player + entity.type=minecraft:zombie。

## 4. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 启动日志 Loaded 10 advancement(s)（M1 4 + M3 3 + M4 3）；无 unsupported 条件 warning |
| 击杀触发 | 在世界出生点 SpawnMob(mtZombie)（ID 128），等 2 秒让其进入 ticking，再用 DoWithEntityByID → TakeDamage(dtAttack, TestBot, 100, 0)，随后 `HasAdvancement("cuberite:m4/kill_zombie")` = true；`kill_skeleton` = false |
| 客户端 | 公告/树更新〔待验〕（toast 在 M1 已记录为客户端侧未观测到） |

## 5. 已知偏差 / 后续

- `killing_blow` 与 damage predicate 未实现；含这些键的 criterion 加载期被跳过。
- `player_hurt_entity` / `entity_hurt_player` 未实现（下一簇）。
- entity 谓词只支持 type/player；NBT、装备、效果、距离等谓词未实现。
- 非怪物/玩家的实体类型（arrow、boat 等）暂不映射。
- 击杀触发只在 `cPlayer::Killed/KilledBy` 调用；如果玩家击杀由其他路径（如爆炸、环境）引发，仍会走这两个方法（Attacker 为玩家时）。
- 测试注意：在未加载区块（如 y=100 的虚空位置）SpawnMob 后立刻 Lua TakeDamage，会触发 Monster.cpp:1283 的 (a_NewTarget == nullptr) || IsTicking() 断言（预存在的 Lua/生成时序问题，与本触发器无关）。验证时应生成在世界出生点等已加载区块，并等实体进入 ticking 再伤害。
