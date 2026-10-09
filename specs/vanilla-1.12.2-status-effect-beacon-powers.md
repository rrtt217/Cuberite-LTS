# S7 信标效果时长与等级（Beacon powers）

分支：`feature/beacon-power-duration` ｜ 级别：P1 ｜ 前置：无 ｜ 编排见 [roadmap](vanilla-1.12.2-status-effects-roadmap.md)

## 1. 问题

`cBeaconEntity::GiveEffects`（[BeaconEntity.cpp:221-253](../src/BlockEntities/BeaconEntity.cpp)）把效果时长**写死 180 tick**：

```
Player.AddEntityEffect(m_PrimaryEffect, 180, EffectLevel);        // 行 245
if (HasSecondaryEffect) { Player.AddEntityEffect(m_SecondaryEffect, 180, 0); }   // 行 249
```

金字塔等级只影响半径与「能否升到 II」，**不影响时长** → 1 级与 4 级信标的效果时长相同，且都比 vanilla 短。

## 2. 行为规格（1.12.2）

来源：[Beacon](https://minecraft.wiki/w/Beacon)（已核对该页原文）：

- 「**Every 4 seconds**, if the beacon beam is currently active, the selected powers are applied with a duration of **9 seconds, plus 2 seconds per pyramid level**, to all players in range.」
- 时长表（JE）：1→11 s、2→13 s、3→15 s、4→17 s；半径（不含信标本身）：20/30/40/50；
- 主要能力：Speed / Haste 任意级，Resistance / Jump Boost 需 2 级，Strength 需 3 级；次要能力（仅 4 级）：Regeneration I，或把主要能力升到 II；
- 效果只给玩家。

## 3. 现状核对结论

| 项 | 现状 | 判定 |
|---|---|---|
| 4 秒周期（`(WorldTickAge % 4s) == 0s`） | [BeaconEntity.cpp:297-308](../src/BlockEntities/BeaconEntity.cpp) | ✅ |
| 半径 `level*10 + 10` | 同文件 :228 | ✅ |
| 能力等级门控 | 同文件 :72-90 | ✅ 与规格逐项一致 |
| 4 级「主效果升 II」= 主==次 时 `EffectLevel = 1` | 同文件 :230-233 | ✅（协议上「二级槽填同一效果 id」正是 vanilla 的做法） |
| 只给玩家 | 同文件 :240 | ✅ |
| **时长** | 固定 180 | ❌ 应为 `(9 + 2 × level) × 20` |
| 遮挡/金字塔探测 | :33-66 / :140-151 | 本分支不动（属信标结构规格） |

## 4. 改动

1. 新增纯规则头 `src/BlockEntities/BeaconPowerRules.h`：
   ```
   int BeaconEffectDurationTicks(int a_PyramidLevel);   // (9 + 2*level) * 20，level∈[1,4]，越界钳制
   int BeaconEffectRadius(int a_PyramidLevel);          // 10 + 10*level
   ```
   并把 `IsValidEffect` 的门控表搬进来（使门控可测）。
2. `GiveEffects` 用 `BeaconEffectDurationTicks(m_BeaconLevel)` 取代常量 180；
   注意与 S5 的交互：**同一信标每 4 秒重施**，若 S5 已实现「同 amp 取更长剩余时长」则行为更贴近 vanilla；若 S5 未合并，本分支的时长修复独立成立（现行为是整条替换，仍是「持续刷新」，与 vanilla 表现一致）。
3. `SetPrimaryEffect`/`SetSecondaryEffect` 的窗口属性同步（:109/:131）保持不变。

## 5. 测试

`tests/BlockEntities/BeaconPowerRulesTest.cpp`（新目录，或并入 `tests/Entities/`——取前者更贴合 `src/BlockEntities/`）：
- 时长序列 220/260/300/340 tick（= 11/13/15/17 s）；
- 半径序列 20/30/40/50；
- 门控表：Speed/Haste 在 level 1 可；Resistance/Jump Boost 需 ≥2；Strength 需 ≥3；Regeneration 仅 4；其余 id 全部拒；
- 越界钳制：level 0/5/−1 → 不崩且落在合法区间。
CMake 做法同 [S1 §5](vanilla-1.12.2-status-effect-amplifier.md)。

## 6. 已知偏差

- 信标效果的 **ambient 标记**（HUD 蓝框）无法表达：1.9+ 的 flags 位域被恒写 0（[Protocol_1_8.cpp:515](../src/Protocol/Protocol_1_8.cpp)）。要修需连带改 `SendEntityEffect` 签名 → 归 [S14](vanilla-1.12.2-status-effect-potion-metadata.md)。
- 金字塔层数探测（`CalculatePyramidLevel`）的正确性不在本分支（属信标结构规格）。
