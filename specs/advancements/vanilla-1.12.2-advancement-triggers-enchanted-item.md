# 1.12.2 进度触发器：enchanted_item spec

> **状态**：实现中（feature/advancements-triggers-enchanted-item，叠在 feature/advancements-triggers-enter-block 之上；Core 对应分支 feature/core-enchanted-item-trigger）。
> **来源**：[Advancement（2017-09-27 修订）](https://minecraft.wiki/w/Advancement?oldid=1151757) 的 `minecraft:enchanted_item` 小节。

## 1. 原版行为（文档）

玩家附魔物品后触发（附魔台、铁砧或命令）。条件：

- `item`：附魔后的物品 predicate；
- `levels`：附魔消耗的经验等级（整数或 `{min,max}`）。

## 2. 实现

- 本仓库目前没有附魔台/铁砧的 C++ 交互路径，因此首个入口是 **Core 的 /enchant 命令**：
  - Core `enchant.lua` 在 `EnchantItem()` 成功后，对目标玩家调用 `a_Target:TriggerEnchantedItem(a_Target:GetEquippedItem(), 0)`；
  - `levels` 传 0：命令本身不消耗经验等级（原版命令路径的等级消耗语义〔待核〕）。
- C++：`cPlayer::TriggerEnchantedItem(cItem, Levels)` → `cAdvancementManager::TriggerEnchantedItem()`；加载期只接受 `item` / `levels`，其他键 warning 并跳过；`item` 用 `MatchItemPredicate`，`levels` 用 `MatchRange`。
- demo：已移除（原 cuberite 测试数据由 1.12.2 原版数据取代，见 vanilla-1.12.2-advancement-delivery.md）。

## 3. 验证

| 项 | 结果 |
|---|---|
| 风格/构建/ctest | 0 violations / 通过 / 49/49 |
| 数据加载 | 启动日志 Loaded 23 advancement(s)；无 unsupported 条件 warning |
| 触发授予 | bot 持钻石剑（slot 37）→ console `enchant TestBot sharpness 1` → `HasAdvancement("cuberite:m5/enchanted_item")` = true（实测 true） |
| 客户端 | 公告/树更新〔待验〕（toast 在 M1 已记录为客户端侧未观测到） |

## 4. 已知偏差 / 后续

- 只覆盖 Core `/enchant` 的单目标路径；`@` 选择器路径（`EnchantSelectorTargets`）暂未通知触发器。
- 附魔台/铁砧的 C++ 路径尚未实现，无法从这两个入口触发；实现后应复用同一个 cPlayer 方法。
- `levels` 固定 0（命令不消耗 XP 等级）；原版铁砧/附魔台的等级消耗语义待接入时再对齐。
- `item` 谓词的 nbt/enchantments 未实现（与其他触发器一致）。
