# 1.12.2 进度触发器：used_ender_eye spec

> **状态**：实现中（feature/advancements-triggers-used-ender-eye，叠在 feature/advancements-triggers-placed-block 之上）。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 `minecraft:used_ender_eye` 小节。

## 1. 原版行为（文档）

玩家使用末影之眼（在有要塞生成的世界里）后触发。条件：

- `distance`：玩家与要塞之间的水平距离（整数或 `{min,max}`）。

## 2. 实现

- trigger 白名单加入 `minecraft:used_ender_eye`；加载期只接受 `distance`，其他键 warning 并跳过（fail-closed）。
- 调用点：`cItemEyeOfEnderHandler::OnItemUse()` 的**投掷**路径（`CreateProjectile()` 成功后、扣物品前）：
  - 距离 = `hypot(玩家X - 要塞目标X, 玩家Z - 要塞目标Z)`，四舍五入为整数；
  - 要塞目标来自 `a_World->GetGenerator().GetNearestStructureTarget("Stronghold", ...)`（该路径本来就要用它来定飞行方向）；
  - 命中 `MatchRange(distance)` 后走 M1 的 `GrantAdvancementCriterion`。
- demo：已移除（原 cuberite 测试数据由 1.12.2 原版数据取代，见 vanilla-1.12.2-advancement-delivery.md）。

## 3. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 启动日志 Loaded 16 advancement(s)；无 unsupported 条件 warning |
| 触发授予（bot 路径） | bot 持末影之眼(381) → `mcc_use_item` 投掷 → `HasAdvancement("cuberite:m5/use_ender_eye")` = true |
| 客户端 | 公告/树更新〔待验〕（toast 在 M1 已记录为客户端侧未观测到） |

## 4. 已知偏差 / 后续

- 只覆盖**投掷**路径；往末地传送门框架里填眼的路径不触发（原版是否触发〔待核〕）。
- 如果当前世界生成器没有 Stronghold（`GetNearestStructureTarget` 失败），投掷本身会被拒绝，触发器也不会触发；与原版「总有要塞」的差异取决于生成器配置。
- `distance` 用四舍五入后的整型，与原版的双精度比较可能有 ±1 的边界差异〔待核〕。
