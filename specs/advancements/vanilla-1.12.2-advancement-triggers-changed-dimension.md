# 1.12.2 进度触发器：changed_dimension spec

> **状态**：实现中（feature/advancements-triggers-changed-dimension，叠在 feature/advancements-triggers-effects-changed 之上）。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 `minecraft:changed_dimension` 小节。

## 1. 原版行为（文档）

玩家改变维度后触发。条件：

- `from`：原维度 ID（overworld / the_nether / the_end）；
- `to`：目标维度 ID。

## 2. 实现

- trigger 白名单加入 `minecraft:changed_dimension`；加载期只接受 `from` / `to`，其他键 warning 并跳过。
- 调用点：`cEntity::DoMoveToWorld()` 在 `SetWorld(新世界)` 之后、`AddEntity()` 之前：
  - 记录旧世界维度（`OldWorld->GetDimension()`）和新世界维度；
  - 仅 `IsPlayer()` 且维度确实变化时调用 `TriggerChangedDimension(*this, Old, New)`。
- 条件匹配：维度名映射（`dimOverworld` → `overworld`，`dimNether` → `the_nether`，`dimEnd` → `the_end`），条件里的 `minecraft:` 前缀会被去掉。
- demo：已移除（原 cuberite 测试数据由 1.12.2 原版数据取代，见 vanilla-1.12.2-advancement-delivery.md）。

## 3. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 启动日志 Loaded 20 advancement(s)；无 unsupported 条件 warning |
| 触发授予 | execute_lua `p:MoveToWorld("world_nether", false)` → `HasAdvancement("cuberite:m5/changed_dimension")` = true，world=world_nether；再 MoveToWorld("world") 返回 overworld |
| 客户端 | 公告/树更新〔待验〕（toast 在 M1 已记录为客户端侧未观测到） |

## 4. 已知偏差 / 后续

- 触发在 `SetWorld` 之后、`AddEntity` 之前；此时玩家客户端正处在维度切换中，进度包与 respawn 包的相对顺序需要用真客户端观察〔待核〕。
- 只对玩家触发（原版触发器只对玩家）；same-dimension 传送不触发。
- 维度名只映射三个原版维度；自定义维度/数字 ID 未支持。
