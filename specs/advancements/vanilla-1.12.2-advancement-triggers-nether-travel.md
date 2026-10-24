# 1.12.2 进度触发器：nether_travel spec

> **状态**：实现中（feature/advancements-triggers-nether-travel，叠在 feature/advancements-triggers-location 之上）。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 `minecraft:nether_travel` 小节。

## 1. 原版行为（文档）

玩家进入下界后返回主世界时触发。条件：

- `distance`：进入下界与返回主世界两个主世界位置之间的距离：
  - `absolute`：3D 距离；
  - `horizontal`：水平（XZ）距离；
  - `x` / `y` / `z`：各轴绝对差；
  - 每项是 `{min,max}` 浮点范围。

## 2. 实现

- `cPlayer` 增加 `m_NetherEntry` / `m_HasNetherEntry` 与 `SetNetherEntry` / `GetNetherEntry` / `ClearNetherEntry`（tolua 块外，内部 API）。
- `cEntity::DoMoveToWorld()` 记录 `OldPos`（移动前位置）：
  - 主世界 → 下界：`Player.SetNetherEntry(OldPos)`；
  - 下界 → 主世界：取记录的 entry，调用 `TriggerNetherTravel(Player, entry, 新位置)`，然后清除 entry。
- manager：白名单加入 `minecraft:nether_travel`；只接受 `distance`（内部只允许 absolute/horizontal/x/y/z）；用 `MatchDoubleRange` 匹配。
- demo：`Server/advancements/cuberite/m5/nether_travel.json`（无条件）。

## 3. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 启动日志 Loaded 32 advancement(s)；无 unsupported 条件 warning |
| 触发授予 | bot `MoveToWorld("world_nether", false)` → 再 `MoveToWorld("world", false)` → `HasAdvancement("cuberite:m5/nether_travel")` = true（实测 true，world=world） |
| 客户端 | 公告/树更新〔待验〕（toast 在 M1 已记录为客户端侧未观测到） |

## 4. 已知偏差 / 后续

- entry 位置只在内存里记录；玩家在**下界退出/重登**会丢失 entry，返回时不触发（vanilla 是否持久化〔待核〕）。
- 距离用主世界 entry 与返回后的主世界位置计算；与原版传送门坐标缩放的细节可能有差异〔待核〕。
- `distance` 各分量已支持；其他条件未实现。
