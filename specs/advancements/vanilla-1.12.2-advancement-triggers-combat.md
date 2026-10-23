# 1.12.2 进度触发器：战斗簇（M4）spec

> **状态**：实现中（feature/advancements-triggers-combat，叠在 feature/advancements-triggers-inventory 之上）。
> **范围**：四个战斗触发器 `player_killed_entity`、`entity_killed_player`、`player_hurt_entity`、`entity_hurt_player` + entity/damage 谓词子集。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的对应 trigger 小节；[同步实测记录](vanilla-1.12.2-advancements-observation.md)。

## 1. 原版行为（文档）

- `minecraft:player_killed_entity`：玩家击杀实体后触发；条件 `entity`（被击杀实体）、`killing_blow`（伤害来源）。
- `minecraft:entity_killed_player`：玩家被实体杀死后触发；条件 `entity`（击杀者）、`killing_blow`。
- `minecraft:player_hurt_entity`：玩家伤害实体后触发；条件 `entity`（被伤害实体）、`damage`（伤害来源）。
- `minecraft:entity_hurt_player`：玩家受到伤害后触发；条件 `damage`（伤害来源）。

## 2. 实现

- trigger 索引沿用 M3 的 `m_Triggers`；加载期只索引已实现的 trigger（inventory_changed + 上述四个），其余 trigger 的 criterion 暂不索引。
- 条件支持性检查（fail-closed）：
  - 击杀簇与 `player_hurt_entity`：只接受 `entity`（谓词只接受 `type` / `player`）；出现 `killing_blow` / `damage` 等键时加载期 warning 并跳过该 criterion。
  - `entity_hurt_player`：接受空条件，或 `damage.source_entity`（谓词只接受 `type` / `player`）；出现 `damage.type`、`is_projectile` 等键时加载期 warning 并跳过。
- entity 类型名：
  - 玩家 → `minecraft:player`；
  - 怪物 → `minecraft:` + NamespaceSerializer::From(GetMobType())（如 `minecraft:zombie`）；
  - 其他实体类型暂不映射（带 `type` 的条件不命中）。
- 调用点：
  - `cPlayer::Killed(a_Victim, a_DamageType)` 末尾（击杀者）→ TriggerPlayerKilledEntity；
  - `cPlayer::KilledBy(a_TDI)` 死亡处理末尾（被击杀者）→ TriggerEntityKilledPlayer(a_TDI.Attacker)；
  - `cEntity::DoTakeDamage(TDI)` 在 `m_Health` 扣减之后、死亡判定之前（`TDI.FinalDamage > 0`）：
    - attacker 是玩家且不是自身 → TriggerPlayerHurtEntity(attacker, *this)；
    - `this` 是玩家 → TriggerEntityHurtPlayer(*this, TDI.Attacker)（attacker 可为空，环境伤害也能匹配无 source_entity 的条件）。
- 命中后调用 M1 的 `GrantAdvancementCriterion`；hurt 先于 kill 触发（伤害落地后先 hurt，再进入死亡判定）。

## 3. demo 数据

Server/advancements/cuberite/m4/：
- `kill_zombie.json`：player_killed_entity + entity.type=minecraft:zombie；
- `kill_skeleton.json`：entity.type=minecraft:skeleton；
- `killed_by_zombie.json`：entity_killed_player + entity.type=minecraft:zombie；
- `hurt_zombie.json`：player_hurt_entity + entity.type=minecraft:zombie；
- `hurt_by_zombie.json`：entity_hurt_player + damage.source_entity.type=minecraft:zombie。

## 4. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 启动日志 Loaded 12 advancement(s)；加载期无 unsupported 条件 warning |
| 击杀触发 | 世界出生点 SpawnMob(mtZombie)、等 ticking，TakeDamage(dtAttack, TestBot, 100, 0) → `kill_zombie` = true、`kill_skeleton` = false |
| 伤害触发 | 对同一僵尸 TakeDamage(dtAttack, TestBot, 1, 0)（非致命）→ `hurt_zombie` = true；再对 TestBot TakeDamage(dtAttack, zombie, 1, 0) → `hurt_by_zombie` = true（实测均为 true，无断言/错误） |
| 客户端 | 公告/树更新〔待验〕（toast 在 M1 已记录为客户端侧未观测到） |

## 5. 已知偏差 / 后续

- `killing_blow`、`damage.type`、`is_projectile`、实体/物品/位置等谓词未实现；含这些键的 criterion 加载期被跳过。
- entity 谓词只支持 type/player；NBT、装备、效果、距离等未实现。
- 非怪物/玩家的实体类型（arrow、boat 等）暂不映射。
- hurt 触发只在 `cEntity::DoTakeDamage` 的 `FinalDamage > 0` 路径；0 伤害事件不触发。
- 玩家对自己造成伤害（attacker == this）不触发 player_hurt_entity。
- 测试注意：在未加载区块 SpawnMob 后立刻 Lua TakeDamage 会触发 Monster.cpp:1283 的 ticking 断言（预存在时序问题）；应生成在世界出生点并等实体 ticking。
