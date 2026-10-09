# S5 状态效果施加语义（重复施加合并 + DistanceModifier 误用）

分支：`feature/effects-application-semantics` ｜ 级别：P1 ｜ 前置：无 ｜ 编排见 [roadmap](vanilla-1.12.2-status-effects-roadmap.md)

## 1. 两个确定性缺陷 + 一个待核语义

### 1.1 `DistanceModifier` 把时长乘成 0（确定）

`cPawn::AddEntityEffect` 用 `a_DistanceModifier` **缩放时长**（[Pawn.cpp:200](../src/Entities/Pawn.cpp)）：

```
a_Duration = static_cast<int>(a_Duration * a_DistanceModifier);
```

而 [Entity.cpp:1872](../src/Entities/Entity.cpp)（呼吸附魔头盔给水下玩家夜视）传的是 `a_DistanceModifier = 0`：

```
AddEntityEffect(cEntityEffect::effNightVision, 200, 5, 0);   // 时长 200 * 0 = 0
```

→ 该效果时长恒为 0，下一个 tick 即被移除（[Pawn.cpp:47](../src/Entities/Pawn.cpp)），**整条附魔夜视是死代码**。
同时 `cEntityEffect::OnActivate` 又用同一个 `m_DistanceModifier` 缩放**即时效果效力**（[EntityEffect.cpp:329](../src/Entities/EntityEffect.cpp)）——一个参数承担两种语义。

### 1.2 重复施加 = 整条替换 + 计时归零（与规格不符，见 §2）

[Pawn.cpp:202-212](../src/Entities/Pawn.cpp)：已存在同类型效果时先 `OnDeactivate`、再整条重建（`m_Ticks` 归 0）。后果：站在信标旁 / 区域效果云里，每 4 秒 / 每 10 tick 被重新施加一次 → 时长永不衰减；等级更低的药水会**顶掉**等级更高的现有效果。

### 1.3 立即广播的时机

`AddEntityEffect` 先 `BroadcastEntityEffect` 再 `OnActivate`（[Pawn.cpp:210-212](../src/Entities/Pawn.cpp)），而 `OnActivate` 里可能立刻造成伤害/回血 → 客户端先看到图标、后收到血条。顺序本身可接受，但合并语义改动后需要重新确认「广播的是最终生效值」。

## 2. 行为规格（1.12.2）

**DistanceModifier**：vanilla 只在**喷溅/云**场景按距离衰减，衰减同时作用于时长与即时效力，且取值区间为 `[0, 1]`；**不存在「传 0 表示不衰减」的用法**。
来源：[Potion](https://minecraft.wiki/w/Potion)（splash/lingering 的距离衰减与 1/4、1/2 规则）；本仓库对云的实现与调用方约定见 [specs/vanilla-1.12.2-area-effect-cloud.md §2/§7](vanilla-1.12.2-area-effect-cloud.md)。

**重复施加合并（已由允许来源证实）**：[Commands/effect](https://minecraft.wiki/w/Commands/effect) 的 Usage 段原文：

> if a target already has a status effect with the same id, a new effect **only with a longer duration or a higher amplifier** can be added.
> If the new effect has a higher amplifier and a shorter duration, the original effect is hidden.
> If the new effect has a lower amplifier and a longer duration, the new effect is hidden.
> Otherwise, the original active effect is replaced by the new effect, without changing hidden effects.

据此推导（本分支的判定表，`level` 指 amplifier）：

| 旧 vs 新 | 结果 |
|---|---|
| 新 amplifier **更高** | 替换（新时长；即使新时长更短也替换） |
| 新 amplifier **相同**，新时长 **>** 旧剩余时长 | 只把时长延长到新值，**不重置已进行 tick** |
| 新 amplifier **相同**，新时长 **≤** 旧剩余时长 | 保持不变 |
| 新 amplifier **更低** | 保持不变（更低的药水顶不掉更高的效果） |

「hidden / 不改变隐藏效果」属 1.20.5+ 的 `hideParticles` 语义，与 1.12.2 无关，本分支**不实现隐藏**（1.12.2 的 `hideParticles` 只是不画粒子，落在 [S14](vanilla-1.12.2-status-effect-potion-metadata.md)）。
**版本注记**：该页描述当前版本；上述「只有更长或更强才能加入」的规则在 1.9–1.12 的行为序列中一致，但 1.12.2 专属页面待在动手时补链接（`[needs-check]` 仅限「1.12.2 页面链接」这一点，规则本身按上表实现）。

## 3. 改动

1. **拆开两个语义**：`cPawn::AddEntityEffect` 的参数改名/拆分为 `a_DurationMultiplier`（默认 1.0，只作用时长）与即时效力系数，或直接删掉「乘时长」改为调用方自己算好时长（**推荐**：与云 spec 已有的「由调用方给出最终值」一致，见 [area-effect-cloud §6.2](vanilla-1.12.2-area-effect-cloud.md)）。
   - 受影响调用点：[SplashPotionEntity.cpp:72-96](../src/Entities/SplashPotionEntity.cpp)（真正的距离衰减处）、[Entity.cpp:1872](../src/Entities/Entity.cpp)、[Pawn.cpp:186-216](../src/Entities/Pawn.cpp)、[EntityEffect.h:60-96](../src/Entities/EntityEffect.h) 与绑定/APIDump。
2. **修呼吸夜视调用**：改为传「不衰减」的正确形式；该效果是否**应当**存在属 `[needs-check]`（允许来源未证实 1.12.2 呼吸附魔给夜视）→ 本分支**保留行为但让它真的生效**，并加 `// TODO(needs-check)` 指向 oracle 复核。
3. **把合并决策抽成纯函数** `src/Entities/EffectCombineRules.h`：
   ```
   enum class EffectCombine { Replace, Ignore, ExtendDuration };"
   EffectCombine DecideCombine(int a_OldAmp, int a_OldRemainingTicks, int a_OldTicksElapsed,"
                               int a_NewAmp, int a_NewDurationTicks);
   ```
   `cPawn::AddEntityEffect` 只负责执行决策。**单测锁的是「现状行为 + 决策点唯一化」**，日后改判定只需改这一处。
4. `cEntityEffect` 增加 `AddTicks`/`SetRemainingTicks` 语义整理（现有 `SetTicks`，[EntityEffect.h:98-101](../src/Entities/EntityEffect.h)），为「不重置已进行时间」预留接口。

## 4. 测试

`tests/Entities/EffectCombineRulesTest.cpp`：
- 覆盖 §2 判定表四行全分支（更高 amp、同 amp 更长、同 amp 不长、更低 amp），并断言「同 amp 延长时长」时 \`Ticks\` 保持原值；
- 不变量：**任何决策都不产生负剩余时长**；`Ignore` 决策下现有 `Ticks` 不被清零；
- `DurationMultiplier` 拆分后的边界：乘子 0 → 时长 0（调用方必须自觉），乘子 1 → 时长不变，乘子 > 1 → 拒绝（`ASSERT`）。

**若改动导出签名**（`AddEntityEffect` 参数变化）→ AGENTS 门 4/门 6 全部适用：
`src/Bindings/AllToLua.pkg` + 手写绑定 + [APIDump](../Server/Plugins/APIDump/) 三处同步，并跑 `CheckBindingsDependencies.lua` 与 APIDump 自检。
为压缩风险，**优先考虑不改签名的做法**：新增重载 `AddEntityEffectNoFalloff(...)`，保留旧签名并注明其距离衰减语义。

## 5. 已知偏差（本分支明确不解决）

- 合并规则本身仍为「替换 + 计时归零」（§2 推测项）。
- 区域效果云/喷溅药水的距离公式（`1 − 0.25d`）未在允许来源逐字核实 → 保持现状。
- 云内「每次施加后 DurationOnUse/RadiusOnUse」已实现（[AreaEffectCloud.cpp:227-231](../src/Entities/AreaEffectCloud.cpp)）✓ 无偏差。
