# 1.12.2 进度触发器：slept_in_bed spec

> **状态**：实现中（feature/advancements-triggers-slept-in-bed，叠在 feature/advancements-triggers-enchanted-item 之上）。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 `minecraft:slept_in_bed` 小节。

## 1. 原版行为（文档）

玩家成功在床上睡觉后触发。

## 2. 实现

- trigger 白名单加入 `minecraft:slept_in_bed`；加载期只接受空条件；带条件的 criterion warning 并跳过。
- 调用点：`cBlockBedHandler::OnUse()` 在 `SetIsInBed(true)` 成功、占用床并被统计 `SleepInBed` 之后 → `TriggerSleptInBed(a_Player)`。
- 睡眠的前置检查（主世界、夜晚/雷暴、床未被占用、附近无敌对生物）仍由既有 `OnUse` 负责；只有真正进入床才触发。
- demo：已移除（原 cuberite 测试数据由 1.12.2 原版数据取代，见 vanilla-1.12.2-advancement-delivery.md）。

## 3. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 启动日志 Loaded 24 advancement(s)；无 unsupported 条件 warning |
| 触发授予 | time set night + difficulty peaceful，床放在脚下 (97,64,-24) → `mcc_activate_block` → `HasAdvancement("cuberite:m5/slept_in_bed")` = true（睡完 IsInBed=false，时间跳过） |
| 客户端 | 公告/树更新〔待验〕（toast 在 M1 已记录为客户端侧未观测到） |

## 4. 已知偏差 / 后续

- 只覆盖成功入睡；被拒绝的睡眠（白天/有怪/床被占）不触发（与原版一致）。
- `slept_in_bed` 的 conditions 未实现（wiki 当前无 conditions；未来数据会加载期跳过）。
- 床的位置/朝向依赖 `MetaDataToDirection`；测试用 foot meta 0 + head meta 8 的两块床。
