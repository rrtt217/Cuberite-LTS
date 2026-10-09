# S3 状态效果：Saturation（永不过期 + 语义/数值偏差）

分支：`feature/effects-saturation` ｜ 级别：**P0** ｜ 前置：无 ｜ 编排见 [roadmap](vanilla-1.12.2-status-effects-roadmap.md)

## 1. 问题

`cEntityEffectSaturation::OnTick`（[EntityEffect.cpp:504-511](../src/Entities/EntityEffect.cpp)）**没有调用 `Super::OnTick`**：

```
void cEntityEffectSaturation::OnTick(cPawn & a_Target)
{
    if (a_Target.IsPlayer()) { ... SetFoodSaturationLevel(+1+amp); }   // 缺 Super::OnTick(a_Target);
}
```

`m_Ticks` 因此恒为 0，而到期判定是 `Duration - Ticks <= 0`（[Pawn.cpp:47](../src/Entities/Pawn.cpp)）→ **效果永不过期**，只能靠喝牛奶（[ItemMilk.h:26](../src/Items/ItemMilk.h)）或死亡（[Pawn.cpp:131](../src/Entities/Pawn.cpp)）清除；期间每 tick 加饱和 → **无限自然回血**。

其余语义/数值偏差（同函数）：

| 项 | 现状 | 规格 |
|---|---|---|
| 恢复饥饿 | 不恢复 | 每 tick 恢复 1 × level 点饥饿 |
| 恢复饱和 | `1 + amplifier` | 2 × level 点 |
| 饱和上限 | 无上限 | 不超过当前饥饿值 |
| 非玩家 | 跳过 | 非生物无饥饿（等价正确） |

## 2. 行为规格（1.12.2）

来源：[Potion](https://minecraft.wiki/w/Potion)（「Saturation … Restores 1 hunger point × level and 2 × level points of saturation」）与 [Saturation (effect)](https://minecraft.wiki/w/Saturation_(effect))（「instantly replenishes 1 hunger × level and 2 × level saturation … continues each tick」）。
饥饿/饱和交互语义（饱和不超过饥饿值）：[Food#Saturation](https://minecraft.wiki/w/Food#Saturation)。

规格化描述（本分支按此实现）：
1. 施加瞬间（`OnActivate`）即结算一次，之后每 tick 结算一次；
2. `hunger += 1 × level`、`saturation += 2 × level`，其中 `level = amplifier + 1`；
3. `saturation` 结算后夹紧到 `[0, hunger]`；`hunger` 夹紧到 `[0, 20]`；
4. 时长正常递减，到期由 `cPawn::Tick` 正常移除并广播。

## 3. 改动

1. `cEntityEffectSaturation::OnTick` 补 `Super::OnTick(a_Target);`（计时）——**一行修掉永不过期**。
2. 结算逻辑改调新规则头 `src/Entities/HungerRules.h`：
   ```
   struct EffectHungerDelta { int Hunger; double Saturation; };
   EffectHungerDelta SaturationEffectDelta(int a_Amplifier);          // {+1*level, +2*level}
   void ClampFoodState(int & a_Hunger, double & a_Saturation);        // 饱和 ≤ 饥饿，饥饿 ≤ 20
   ```
   同头文件后续可被 S4（饥饿效果）复用，避免二次建基设。
3. `cPlayer` 侧用现成的 `SetFoodLevel` / `SetFoodSaturationLevel`（[Player.h](../src/Entities/Player.h)），不新增导出。

## 4. 测试

`tests/Entities/HungerRulesTest.cpp`：
- amplifier 0/1/4 → 饥饿增量 1/2/5、饱和增量 2/4/10；
- 夹紧不变量：饥饿 6 时结算后饱和 ≤ 6；饥饿 20 时饱和可达上限；饥饿 0 时效果把饥饿抬到 ≥1；
- 「Delta 为正的有害/有益方向」不随 amplifier 变号；
- 纯函数即可覆盖，无需 world harness（同 S1 的理由）。

**协议/客户端侧无需改动**：饱和值本就随 `Update Health`（1.8 `0x06`：Health/Food/Food Saturation，见 [wikivg 镜像 rev 1064](https://wikivg.booky.dev/index.php?title=Protocol&oldid=1064)）下发，与效果列表无关。

## 5. 已知偏差

- **[needs-check]** 「每 tick 同时恢复饥饿」在 1.12.2 是否等价于「先即时结算、之后每 tick 结算」——两者在时长 1 tick 的瞬间类场景（喷溅/滞留药水，见 [SplashPotionEntity.cpp:142-166](../src/Entities/SplashPotionEntity.cpp) 与 [specs/vanilla-1.12.2-area-effect-cloud.md §7](vanilla-1.12.2-area-effect-cloud.md) 的 1/2 效力规则）会有可测差异。本分支采用「OnActivate 结算一次 + 之后每 tick 结算」，并在代码注释标注 `[needs-check]`。
- 负面 amplifier（「Negative levels decrease hunger and saturation」）无法经现有 API 表达（`short` 传得进但 `AddEntityEffect` 不校验），本分支不实现。
