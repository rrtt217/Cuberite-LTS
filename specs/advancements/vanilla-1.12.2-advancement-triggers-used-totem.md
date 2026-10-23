# 1.12.2 进度触发器：used_totem spec

> **状态**：实现中（feature/advancements-triggers-used-totem，叠在 feature/advancements-triggers-used-ender-eye 之上）。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 `minecraft:used_totem` 小节。

## 1. 原版行为（文档）

玩家使用不死图腾后触发。条件：

- `item`：图腾物品 predicate（只对图腾物品有效）。

## 2. 实现

- trigger 白名单加入 `minecraft:used_totem`；加载期只接受 `item`，其他键 warning 并跳过（fail-closed）。
- 调用点：`cPawn::KilledBy()` 的图腾复活分支——`DeductTotem()` 成功、播放 `PawnTotemActivates`、加吸收/再生/抗火并设 `m_Health = 1` 之后，若 `IsPlayer()` 则 `TriggerUsedTotem(*this, cItem(E_ITEM_TOTEM_OF_UNDYING))`。
- 存活判定/消耗图腾仍由既有 `DeductTotem()` 负责（主手或副手），触发器只观察成功复活。
- demo：`Server/advancements/cuberite/m5/use_totem.json`（item=minecraft:totem_of_undying）。

## 3. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 启动日志 Loaded 17 advancement(s)；无 unsupported 条件 warning |
| 触发授予（bot 路径） | bot 持不死图腾（slot 37，mcc_select_item）→ Lua `TakeDamage(dtAttack, nil, 100, 0)` → health=1、`HasAdvancement("cuberite:m5/use_totem")` = true |
| 客户端 | 公告/树更新〔待验〕（toast 在 M1 已记录为客户端侧未观测到） |

## 4. 已知偏差 / 后续

- 只覆盖死亡复活路径；不存在其他「使用图腾」路径（原版同）。
- `item` 谓词的 nbt/enchantments 未实现（与 M3/M5 一致）。
- `dtAdmin` / `dtInVoid` 不触发（既有 `DeductTotem` 拒绝，符合原版「虚空/管理员伤害不救」）。
