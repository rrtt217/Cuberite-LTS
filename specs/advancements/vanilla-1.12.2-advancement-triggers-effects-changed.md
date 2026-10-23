# 1.12.2 进度触发器：effects_changed spec

> **状态**：实现中（feature/advancements-triggers-effects-changed，叠在 feature/advancements-triggers-recipe-unlocked 之上）。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 `minecraft:effects_changed` 小节。

## 1. 原版行为（文档）

玩家的状态效果列表变化后触发。条件：

- `effects`：效果 predicate 列表（effect / amplifier / duration / is_ambient / hide_particles 等）。

## 2. 实现

- trigger 白名单加入 `minecraft:effects_changed`。
- 调用点：`cPawn::AddEntityEffect()` 的两条成功路径：
  - 已有同名效果、决定延长持续时间（`EffectCombine::ExtendDuration`）广播之后；
  - 新建/替换效果、`Effect->OnActivate()` 之后。
  - `Ignore`（更弱且更短）路径不触发；只有 `IsPlayer()` 时触发。
- 条件：本簇先只支持**无条件** criterion；带 `effects` 谓词的 criterion 在加载期 warning 并跳过（fail-closed），避免把任意效果变化误判为匹配。
- demo：`Server/advancements/cuberite/m5/effects_changed.json`（无条件）。

## 3. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 启动日志 Loaded 19 advancement(s)；无 unsupported 条件 warning |
| 触发授予 | console `effect TestBot 1 30 1`（速度，ID 1）→ `HasAdvancement("cuberite:m5/effects_changed")` = true（用名字 `speed` 会被 Core 命令拒绝，需用数字 ID） |
| 客户端 | 公告/树更新〔待验〕（toast 在 M1 已记录为客户端侧未观测到） |

## 4. 已知偏差 / 后续

- 只覆盖**添加/延长**；效果到期移除（`RemoveEntityEffect`）尚未触发（原版 effects_changed 对移除也会触发〔待核〕）。
- `effects` 谓词（effect/amplifier/duration/…）未实现；含该键的 criterion 会被跳过。
- 仅玩家；怪物获得效果不触发（原版触发器只对玩家）。
