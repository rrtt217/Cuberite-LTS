# 1.12.2 进度触发器：bred_animals spec

> **状态**：实现中（feature/advancements-triggers-bred-animals，叠在 feature/advancements-triggers-tame-animal 之上）。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 `minecraft:bred_animals` 小节。

## 1. 原版行为（文档）

玩家让两只动物繁殖后触发。条件（都是 entity predicate）：

- `child`：繁殖出的幼崽；
- `parent`：父/母之一；
- `partner`：与 parent 繁殖的另一只（用于马+驴=骡之类）。

## 2. 实现

- trigger 白名单加入 `minecraft:bred_animals`；加载期只接受 `child` / `parent` / `partner`，每个都是 entity predicate（type/player），其他键 warning 并跳过。
- 调用点：`cMonster::LoveTick()` 的繁殖完成分支——生成幼崽、继承、经验球之后，在 `DoWithPlayerByUUID(m_Feeder, ...)` 的 lambda 里（与 `AnimalsBred` 统计同一处）调用 `TriggerBredAnimals(a_Player, *this, *m_LovePartner, Baby)`。
- 实体谓词匹配复用 `MatchEntityPredicateValue`（type/player）；`child` 条件在幼崽为空时视为不匹配。
- demo：`Server/advancements/cuberite/m5/bred_animals.json`（parent/partner 都是 minecraft:cow）。

## 3. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 启动日志 Loaded 27 advancement(s)；无 unsupported 条件 warning |
| 触发授予 | 脚本 bot 生成两只牛（ID 201/202），距离 1.6 时各喂一次小麦 → 约 3 秒后幼崽出生 → `HasAdvancement("cuberite:m5/bred_animals")` = true（实测 true） |
| 客户端 | 公告/树更新〔待验〕（toast 在 M1 已记录为客户端侧未观测到） |

## 4. 已知偏差 / 后续

- 只有真正喂食的玩家（`m_Feeder`）获得触发；与 vanilla 一致。
- `child` 条件在幼崽生成失败时不会匹配；正常路径幼崽存在。
- entity 谓词只支持 type/player；location/effects 等谓词未实现。
