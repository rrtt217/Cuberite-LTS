# 1.12.2 进度触发器：tame_animal spec

> **状态**：实现中（feature/advancements-triggers-tame-animal，叠在 feature/advancements-fix-consume-item 之上）。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 `minecraft:tame_animal` 小节。

## 1. 原版行为（文档）

玩家驯服动物后触发。条件：

- `entity`：被驯服动物的 entity predicate（type / player 等）。

## 2. 实现

- trigger 白名单加入 `minecraft:tame_animal`；加载期只接受 `entity`，其他键 warning 并跳过；匹配复用 `MatchEntityPredicate`（type/player）。
- 调用点：`cWolf::OnRightClicked()` 的驯服成功分支——`SetIsTame(true)` / `SetOwner()` / `WolfTamingSucceeds` 动画之后 → `TriggerTameAnimal(a_Player, *this)`。
- demo：`Server/advancements/cuberite/m5/tame_animal.json`（entity.type=minecraft:wolf）。
- 只有成功驯服才触发；失败的骨头尝试不触发。

## 3. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 启动日志 Loaded 26 advancement(s)；无 unsupported 条件 warning |
| 触发授予 | 生成狼（客户端实体 id 122）+ 20 根骨头，`mcc_entity_interact` 直到驯服 → `HasAdvancement("cuberite:m5/tame_animal")` = true（实测 true） |
| 客户端 | 公告/树更新〔待验〕（toast 在 M1 已记录为客户端侧未观测到） |

## 4. 已知偏差 / 后续

- 只接入了 **狼** 的驯服路径；豹猫/马的驯服入口尚未接入（它们的 `SetIsTame` 位置不同）。
- entity 谓词只支持 type/player（与战斗簇一致）。
- 驯服概率沿用服务端既有 0.125/骨头；测试需要多次交互。
