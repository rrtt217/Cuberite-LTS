# S5 状态效果施加语义（重复施加合并 + DistanceModifier 误用）

分支：`feature/effects-application-semantics` ｜ 级别：P1 ｜ 前置：无 ｜ 编排见 [roadmap](vanilla-1.12.2-status-effects-roadmap.md)

## 1. 两个确定性缺陷 + 一个待核语义

### 1.1 `DistanceModifier` 把时长乘成 0（确定）

`cPawn::AddEntityEffect` 用 `a_DistanceModifier` **缩放时长**（修复前，该行已删除；[Pawn.cpp](../src/Entities/Pawn.cpp)）：

```
a_Duration = static_cast<int>(a_Duration * a_DistanceModifier);
```

而 [Entity.cpp:1875](../src/Entities/Entity.cpp)（呼吸附魔头盔给水下玩家夜视）传的是 `a_DistanceModifier = 0`：

```
AddEntityEffect(cEntityEffect::effNightVision, 200, 5, 0);   // 时长 200 * 0 = 0
```

→ 该效果时长恒为 0，下一个 tick 即被移除（[Pawn.cpp:50](../src/Entities/Pawn.cpp)），**整条附魔夜视是死代码**。
同时 `cEntityEffect::OnActivate` 又用同一个 `m_DistanceModifier` 缩放**即时效果效力**（[EntityEffect.cpp:332](../src/Entities/EntityEffect.cpp)）——一个参数承担两种语义。

### 1.2 重复施加被静默丢弃（与规格不符，见 §2）

修复前的 [Pawn.cpp](../src/Entities/Pawn.cpp) `AddEntityEffect` 的写法是「先 `OnDeactivate` 旧效果，再 `emplace` 新效果」；但 `std::map::emplace` 在键已存在时**不会插入**，返回的是旧元素（新构造的效果对象随即被析构）。因此实际行为是：旧效果被 `OnDeactivate` 后又被 `OnActivate`（净效果为零），**新效果的等级与时长被完全丢弃**。后果：站在信标旁 / 区域效果云里，每 4 秒 / 每 10 tick 被重新施加一次不产生任何变化（剩余时长不延长）；**等级更高的药水顶不掉等级更低的现有效果**——与 §2 判定表恰好相反。

> 本 spec 初稿曾写「整条重建、`m_Ticks` 归 0、低等级顶掉高等级」。动手时按代码路径实测复核，结论为上述「`emplace` 未插入」；初稿描述作废，判定表（§2）不变。

### 1.3 立即广播的时机

`AddEntityEffect` 先 `BroadcastEntityEffect` 再 `OnActivate`（[Pawn.cpp:262](../src/Entities/Pawn.cpp)），而 `OnActivate` 里可能立刻造成伤害/回血 → 客户端先看到图标、后收到血条。顺序本身可接受，但合并语义改动后需要重新确认「广播的是最终生效值」。

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
   - 受影响调用点：[SplashPotionEntity.cpp:59-88](../src/Entities/SplashPotionEntity.cpp)（真正的距离衰减处）、[Entity.cpp:1875](../src/Entities/Entity.cpp)、[Pawn.cpp:196-259](../src/Entities/Pawn.cpp)、[EntityEffect.h:60-101](../src/Entities/EntityEffect.h) 与绑定/APIDump。
2. **修呼吸夜视调用**：改为传「不衰减」的正确形式，让这条效果真的生效。
   - **该行为已由允许来源证实**：Respiration 在 1.12.2 确实「提高水下可见度」——[minecraft.wiki — Respiration](https://minecraft.wiki/w/Respiration) 的 History 段记载 Java 版 **1.13 / 18w08b**「Respiration no longer grants enhanced vision underwater」，即 1.13 之前一直生效。
   - **仍待 oracle 的部分（`TODO(needs-check)`）**：vanilla 把它做成客户端的水下雾/亮度变化，而不是 Night Vision **药水效果**（后者会给玩家挂上夜视图标）；因此「用夜视近似」这一做法与现有的 `200 tick / amplifier 5` 取值留待实机对照。
3. **把合并决策抽成纯函数** `src/Entities/EffectCombineRules.h`：
   ```
   enum class EffectCombine { Replace, Ignore, ExtendDuration };"
   EffectCombine DecideCombine(int a_OldAmp, int a_OldRemainingTicks, int a_OldTicksElapsed,"
                               int a_NewAmp, int a_NewDurationTicks);
   ```
   `cPawn::AddEntityEffect` 只负责执行决策。**单测锁的是「现状行为 + 决策点唯一化」**，日后改判定只需改这一处。
4. `cEntityEffect` 增加 `AddTicks`/`SetRemainingTicks` 语义整理（现有 `SetTicks`，[EntityEffect.h:98-106](../src/Entities/EntityEffect.h)），为「不重置已进行时间」预留接口。
   - 落地时：`AddTicks` 加在 `cEntityEffect` 并用于 `OnTick`；「不重置已进行时间」的算式放进 `EffectCombineRules.h` 的纯函数 `ExtendedTotalDuration`，由 `cPawn::AddEntityEffect` 调用——这样它可被单测，而 `SetRemainingTicks` 不必新增一个（未被测试覆盖的）成员。`AddTicks` 位于 `// tolua_end` 之后，不影响导出 API。
5. **瞬时效果不参与合并**：`IsInstantEffect`（瞬间治疗 / 瞬间伤害 / 饱和）在 `cPawn::AddEntityEffect` 里直接走 `Replace` 分支，保证每次施加都真的执行 `OnActivate`。这类效果本就以 0 时长入 map、下一 tick 被移除，若按 §2 表把「同等级、不长于剩余」判成 `Ignore`，同 tick 内的第二次施加会丢失治疗/伤害。该判定从 [SplashPotionEntity.cpp](../src/Entities/SplashPotionEntity.cpp) 的同名静态函数移入 `EffectCombineRules.h`，两处共用且可单测。

## 4. 测试

`tests/Entities/EffectCombineRulesTest.cpp`：
- 覆盖 §2 判定表四行全分支（更高 amp、同 amp 更长、同 amp 不长、更低 amp），并断言「同 amp 延长时长」时 \`Ticks\` 保持原值；
- 不变量：**任何决策都不产生负剩余时长**；`Ignore` 决策下现有 `Ticks` 不被清零；
- `DurationMultiplier` 拆分后的边界：乘子 0 → 时长 0（调用方必须自觉），乘子 1 → 时长不变，乘子 > 1 → 拒绝（`ASSERT`）。

**若改动导出签名**（`AddEntityEffect` 参数变化）→ AGENTS 门 4/门 6 全部适用：
`src/Bindings/AllToLua.pkg` + 手写绑定 + [APIDump](../Server/Plugins/APIDump/) 三处同步，并跑 `CheckBindingsDependencies.lua` 与 APIDump 自检。
为压缩风险，**优先考虑不改签名的做法**：新增重载 `AddEntityEffectNoFalloff(...)`，保留旧签名并注明其距离衰减语义。

## 5. 已知偏差（本分支明确不解决）

- §2 判定表的来源是 wiki 的当前 `Commands/effect` 页面；「1.9–1.12 行为序列一致」这一点未能在 1.12.2 专属页面逐字核对（§2 的 `[needs-check]` 仅指该链接），判定表本身按 §2 实现。
- 区域效果云/喷溅药水的距离公式（`1 − 0.25d`）未在允许来源逐字核实 → 保持现状。
- 瞬时效果走 `Replace` 是本分支为保住「每次施加都生效」而加的实现约定（§3 第 5 条），不是 §2 判定表的直接推论。
- 云内「每次施加后 DurationOnUse/RadiusOnUse」已实现（[AreaEffectCloud.cpp:227-231](../src/Entities/AreaEffectCloud.cpp)）✓ 无偏差。
