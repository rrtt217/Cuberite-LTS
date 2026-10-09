# S2 状态效果：玩家防火（Fire Resistance 对玩家无效）

分支：`feature/effects-fire-resistance` ｜ 级别：**P0** ｜ 前置：无 ｜ 编排见 [roadmap](vanilla-1.12.2-status-effects-roadmap.md)

## 1. 问题

虚函数被遮蔽：

- `cPawn::IsFireproof()` 返回 `m_IsFireproof || HasEntityEffect(effFireResistance)`（[Pawn.cpp:153-156](../src/Entities/Pawn.cpp)）
- `cPlayer::IsFireproof()` **再次 override**，返回 `m_IsFireproof || IsGameModeCreative() || IsGameModeSpectator()`（[Player.h:228-232](../src/Entities/Player.h)）——**丢掉了效果查询**

火焰/熔岩/燃烧伤害全部经 `IsFireproof()` 虚派发（[Entity.cpp:1277 / 1294 / 1318 / 1339](../src/Entities/Entity.cpp)），因此：

> 玩家喝下火焰抵抗药水 / 吃附魔金苹果 / 被图腾给予 40 秒防火 → **照常烧伤**。

唯一侥幸正确处是岩浆块：那里手写了一遍效果判断（[Entity.cpp:995](../src/Entities/Entity.cpp)），恰好掩盖了主路径的错误。怪物侧正常（走 `cPawn` 版本）。

## 2. 行为规格（1.12.2）

来源：[Potion — status effect table](https://minecraft.wiki/w/Potion)（Fire Resistance：「Immunity to all heat-related damage」）、[Golden Apple](https://minecraft.wiki/w/Golden_Apple)/[Enchanted_Golden_Apple](https://minecraft.wiki/w/Enchanted_Golden_Apple)（附魔金苹果给 5:00 Fire Resistance）。

- 携带 Fire Resistance 的生物：不受火焰/熔岩/燃烧伤害，且**已点燃的状态立即终止**；
- 岩浆块仍会造成伤害（该伤害不是「热伤害」，而是接触伤害）——本仓库现状即如此（[Entity.cpp:991-996](../src/Entities/Entity.cpp)），保持；
- 创造/旁观玩家不烧伤，这是游戏模式规则、与效果无关。

## 3. 改动

1. 删除 `cPlayer::IsFireproof()` 覆写（[Player.h:228-232](../src/Entities/Player.h)），把「游戏模式」条件并入基类语义。落地二选一，取 A：
   - **A（推荐）**：在 `cPawn::IsFireproof()` 内改为 `Super::IsFireproof() || HasEntityEffect(effFireResistance) || (IsPlayer() && 创造/旁观)`，并删掉 `cPlayer` 的覆写。理由：防火判定的真源重新回到一处，`cPlayer` 不再有机会遮蔽。
   - B：`cPlayer::IsFireproof()` 改为 `return Super::IsFireproof() || 创造 || 旁观;`。改动更小，但「覆写链」这一坑仍在。
2. 给该函数补注释：**任何子类覆写都必须查询效果**，否则本缺陷复现。
3. 顺带删除 [Entity.cpp:995](../src/Entities/Entity.cpp) 的重复效果判断（改完 A 后 `IsFireproof()` 已含效果；岩浆块的「仍然受伤」语义要保持 → 该处应改为**只**判 `SetIsFireproof`/游戏模式，或保留现注释说明岩浆块不受防火保护）。**注意**：不要顺手让岩浆块跳过伤害。

## 4. 测试

防火判定是「实体状态 × 游戏模式 × 效果」的纯决策，抽成规则头：

- 新增 `src/Entities/FireproofRules.h`：`bool IsFireproofFor(bool a_HardcodedFireproof, bool a_HasFireResistanceEffect, bool a_IsCreativeOrSpectator)`；`cPawn::IsFireproof()` 与 `cPlayer` 分支都调它。
- `tests/Entities/FireproofRulesTest.cpp`（并人 S1 建的 `tests/Entities/`；若 S1 未合并则本分支自建目录与 `add_subdirectory`）关键用例：
  - **仅有 Fire Resistance 效果的玩家 → true**（本缺陷的回归锁）
  - 仅有效果的非玩家 Pawn → true
  - 生存模式、无效果、未硬编码防火 → false
  - 创造/旁观、无效果 → true
  - 硬编码防火（岩浆怪/烈焰人配置） → true

## 5. 已知偏差

- 岩浆块伤害的归类（是否属于「热伤害」）以本仓库现状为准（`dtMagma`），允许来源未逐条区分；`[needs-check]`。
- 防火生物被火焰弹命中的「不点燃」行为（[FireChargeEntity.cpp:51](../src/Entities/FireChargeEntity.cpp) 已用 `IsFireproof`）在本分支后自动对玩家生效，属修复的连带收益，需在交付说明中列出。
