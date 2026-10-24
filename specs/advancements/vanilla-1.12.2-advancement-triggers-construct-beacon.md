# 1.12.2 进度触发器：construct_beacon spec

> **状态**：实现中（feature/advancements-triggers-construct-beacon，叠在 feature/advancements-triggers-brewed-potion 之上）。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 `minecraft:construct_beacon` 小节。

## 1. 原版行为（文档）

玩家改变信标结构后触发（信标自我更新时）。条件：

- `level`：更新后的信标结构层级（整数或 `{min,max}`）。

## 2. 实现

- trigger 白名单加入 `minecraft:construct_beacon`；加载期只接受 `level`，其他键 warning 并跳过；`level` 用 `MatchRange`。
- 调用点：`cBeaconEntity::UsedBy()`（玩家右键使用信标）：
  - 先调用 `UpdateBeacon()` 重新计算金字塔层级（防止结构变化后层级还是旧值）；
  - 再调用 `TriggerConstructBeacon(*a_Player, GetBeaconLevel())`。
- demo：已移除（原 cuberite 测试数据由 1.12.2 原版数据取代，见 vanilla-1.12.2-advancement-delivery.md）。

## 3. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 启动日志 Loaded 30 advancement(s)；无 unsupported 条件 warning |
| 触发授予 | 在 (99,64,-24) 用 3×3 铁块基座（y=63）+ 信标搭建，bot `mcc_activate_block` 右键 → `HasAdvancement("cuberite:m5/construct_beacon")` = true（实测 true） |
| 客户端 | 公告/树更新〔待验〕（toast 在 M1 已记录为客户端侧未观测到） |

## 4. 已知偏差 / 后续

- 触发时机是**玩家右键使用信标**（打开界面），不是 wiki 描述的「玩家改变结构、信标自我更新」；玩家归因因此绕开了「谁放置了金字塔方块」的追踪。若要精确对齐，需要在方块放置/破坏路径找到受影响的信标并记录玩家。
- `level` 条件已支持；其他条件未实现。
- 控制台/命令方块信标更新没有玩家，不会触发（与原版玩家归因一致〔待核〕）。
